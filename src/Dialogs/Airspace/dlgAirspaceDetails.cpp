// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "time/BrokenDateTime.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "NOTAM/NOTAMGlue.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"
#include "TransponderMode.hpp"
#include "util/StaticString.hxx"
#include "util/ConvertString.hpp"
#include "Geo/AltitudeReference.hpp"

#include <cassert>

namespace {

constexpr std::chrono::seconds PERMANENT_NOTAM_THRESHOLD{2147483647};

static void
FormatAltitudeWithReference(TCHAR *buffer, size_t buffer_size,
                             const AirspaceAltitude &altitude,
                             const std::optional<struct NOTAM> &notam_opt,
                             bool is_base) noexcept
{
  AirspaceFormatter::FormatAltitudeShort(buffer, altitude, true);
  
  if (notam_opt) {
    const auto &ref_alt = is_base ? notam_opt->lower_altitude : notam_opt->upper_altitude;
    const size_t current_len = _tcslen(buffer);
    const size_t remaining = buffer_size > current_len ? buffer_size - current_len - 1 : 0;
    
    if (remaining > 0) {
      const TCHAR *suffix = nullptr;
      
      if (!is_base || !ref_alt.IsTerrain()) {
        if (ref_alt.reference == AltitudeReference::MSL) {
          suffix = _T(" MSL");
        }
      }
      
      if (suffix != nullptr) {
        const size_t suffix_len = _tcslen(suffix);
        if (suffix_len <= remaining) {
          // Safe to copy - we have enough space
          for (size_t i = 0; i <= suffix_len && (current_len + i) < buffer_size; ++i) {
            buffer[current_len + i] = suffix[i];
          }
          buffer[buffer_size - 1] = _T('\0'); // Ensure null termination
        }
      }
    }
  }
}

} // namespace

class AirspaceDetailsWidget
  : public RowFormWidget {
protected:
  ConstAirspacePtr airspace;
  ProtectedAirspaceWarningManager *warnings;

public:
  /**
   * Hack to allow the widget to close its surrounding dialog.
   */
  WndForm *dialog;

  AirspaceDetailsWidget(ConstAirspacePtr _airspace,
                        ProtectedAirspaceWarningManager *_warnings)
    :RowFormWidget(UIGlobals::GetDialogLook()),
     airspace(std::move(_airspace)), warnings(_warnings) {}

  void AckDayOrEnable() noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};

void
AirspaceDetailsWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                               [[maybe_unused]] const PixelRect &rc) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();

  StaticString<64> buffer;

  AddMultiLine(airspace->GetName());

  const TransponderCode transponderCode = airspace->GetTransponderCode();
  TCHAR buffer2[5];

  transponderCode.Format(buffer2, sizeof(buffer2));

  if (transponderCode.IsDefined()) {
    AddReadOnly(_("Squawk code"), nullptr, buffer2);
    AddButton(_("Set Squawk Code"), [transponderCode]() {
      ActionInterface::SetTransponderCode(transponderCode);
    });
  }

  if (airspace->GetRadioFrequency().Format(buffer.data(), buffer.capacity()) !=
      nullptr) {
    buffer += _T(" MHz");
    AddReadOnly(_("Radio"), nullptr, buffer);

    const TCHAR *frequencyName = airspace->GetName();
    const TCHAR *stationName = airspace->GetStationName();

    if (stationName != nullptr && stationName[0] != '\0') {
      AddReadOnly(_("Station"), nullptr, stationName);
      frequencyName = stationName;
    }

    AddButton(_("Set Active Frequency"), [this, frequencyName]() {
      ActionInterface::SetActiveFrequency(airspace->GetRadioFrequency(),
                                          frequencyName);
    });

    AddButton(_("Set Standby Frequency"), [this, frequencyName]() {
      ActionInterface::SetStandbyFrequency(airspace->GetRadioFrequency(),
                                           frequencyName);
    });
  }

  AddReadOnly(_("Class"), nullptr, AirspaceFormatter::GetClassShort(*airspace));
  AddReadOnly(_("Type"), nullptr, AirspaceFormatter::GetType(*airspace));

  AirspaceFormatter::FormatAltitude(buffer.data(), airspace->GetTop());
  AddReadOnly(_("Top"), nullptr, buffer);

  AirspaceFormatter::FormatAltitude(buffer.data(), airspace->GetBase());
  AddReadOnly(_("Base"), nullptr, buffer);

  if (warnings != nullptr) {
    const GeoPoint closest =
      airspace->ClosestPoint(basic.location, warnings->GetProjection());
    const auto distance = closest.Distance(basic.location);

    AddReadOnly(_("Distance"), nullptr, FormatUserDistance(distance));
  }
}

void
AirspaceDetailsWidget::AckDayOrEnable() noexcept
{
  assert(warnings != nullptr);

  const bool acked = warnings->GetAckDay(*airspace);
  warnings->AcknowledgeDay(airspace, !acked);

  dialog->SetModalResult(mrOK);
}

/**
 * Extended widget for displaying NOTAM-specific information
 */
class NOTAMDetailsWidget final : public AirspaceDetailsWidget {
public:
  NOTAMDetailsWidget(ConstAirspacePtr _airspace,
                     ProtectedAirspaceWarningManager *_warnings)
    : AirspaceDetailsWidget(std::move(_airspace), _warnings) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

private:
  void AddNOTAMIdentifiers(const TCHAR *notam_number,
                           const std::optional<struct NOTAM> &notam_opt,
                           StaticString<128> &buffer);
  void AddNOTAMValidity(const std::optional<struct NOTAM> &notam_opt,
                        StaticString<128> &buffer);
  void AddNOTAMAltitudes(const std::optional<struct NOTAM> &notam_opt,
                         StaticString<128> &buffer);
};

void
dlgAirspaceDetails(ConstAirspacePtr airspace,
                   ProtectedAirspaceWarningManager *warnings)
{
  // Use specialized widget for NOTAMs
  const bool is_notam = airspace->GetType() == AirspaceClass::NOTAM;
  
  AirspaceDetailsWidget *widget = is_notam
    ? new NOTAMDetailsWidget(airspace, warnings)
    : new AirspaceDetailsWidget(airspace, warnings);
  
  const TCHAR *title = is_notam ? _("NOTAM Details") : _("Airspace Details");
  
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      title, widget);

  if (warnings != nullptr) {
    widget->dialog = &dialog;
    dialog.AddButton(warnings->GetAckDay(*airspace)
                     ? _("Enable") : _("Ack Day"),
                     [widget](){ widget->AckDayOrEnable(); });
  }
  dialog.AddButton(_("Close"), mrOK);

  dialog.ShowModal();
}

void
NOTAMDetailsWidget::AddNOTAMIdentifiers(const TCHAR *notam_number,
                                        const std::optional<struct NOTAM> &notam_opt,
                                        StaticString<128> &buffer)
{
  // Display NOTAM number
  if (notam_number && notam_number[0] != '\0') {
    AddReadOnly(_("NOTAM"), nullptr, notam_number);
  }

  // Display ICAO location if we found the NOTAM
  if (notam_opt && !notam_opt->location.empty()) {
    UTF8ToWideConverter location_conv(notam_opt->location.c_str());
    if (location_conv.IsValid()) {
      AddReadOnly(_("Location"), nullptr, location_conv.c_str());
    }
  }

  // Display Q-code (feature type)
  if (notam_opt && !notam_opt->feature_type.empty()) {
    UTF8ToWideConverter qcode_conv(notam_opt->feature_type.c_str());
    if (qcode_conv.IsValid()) {
      AddReadOnly(_("Q-Code"), nullptr, qcode_conv.c_str());
    }
  }

  // NOTAM Series (if available)
  if (notam_opt && !notam_opt->series.empty()) {
    UTF8ToWideConverter series_conv(notam_opt->series.c_str());
    if (series_conv.IsValid()) {
      buffer.Format(_T("Series %s"), series_conv.c_str());
      AddReadOnly(_("Type"), nullptr, buffer);
    }
  }
}

void
NOTAMDetailsWidget::AddNOTAMValidity(
    const std::optional<struct NOTAM> &notam_opt,
    StaticString<128> &buffer)
{
  if (!notam_opt)
    return;

  TCHAR time_buffer[64];

  // Effective start - format as friendly date/time
  BrokenDateTime start_dt(notam_opt->start_time);
  _stprintf(time_buffer, _T("%04u-%02u-%02u %02u:%02u"),
            start_dt.year, start_dt.month, start_dt.day,
            start_dt.hour, start_dt.minute);
  AddReadOnly(_("Valid From"), nullptr, time_buffer);

  // Effective end
  BrokenDateTime end_dt(notam_opt->end_time);
  // Check if it's a far future date (PERM = permanent)
  auto end_seconds = std::chrono::duration_cast<std::chrono::seconds>(
    notam_opt->end_time.time_since_epoch()).count();
  if (end_seconds > PERMANENT_NOTAM_THRESHOLD.count()) {
    AddReadOnly(_("Valid Until"), nullptr, _T("PERM"));
  } else {
    _stprintf(time_buffer, _T("%04u-%02u-%02u %02u:%02u"),
              end_dt.year, end_dt.month, end_dt.day,
              end_dt.hour, end_dt.minute);
    AddReadOnly(_("Valid Until"), nullptr, time_buffer);
  }

  // Status indicator
  auto now = std::chrono::system_clock::now();
  StaticString<32> time_str;
  if (now < notam_opt->start_time) {
    // Not yet active
    auto starts_in = std::chrono::duration_cast<std::chrono::hours>(
      notam_opt->start_time - now);
    if (starts_in.count() < 48) {
      time_str.Format(_T("%dh"), static_cast<int>(starts_in.count()));
    } else {
      auto days = starts_in.count() / 24;
      time_str.Format(_T("%dd"), static_cast<int>(days));
    }
    buffer = _("Starts in");
    buffer += _T(" ");
    buffer += time_str;
  } else if (now > notam_opt->end_time) {
    // Expired
    auto expired_ago = std::chrono::duration_cast<std::chrono::hours>(
      now - notam_opt->end_time);
    if (expired_ago.count() < 48) {
      time_str.Format(_T("%dh"), static_cast<int>(expired_ago.count()));
    } else {
      auto days = expired_ago.count() / 24;
      time_str.Format(_T("%dd"), static_cast<int>(days));
    }
    buffer = _("Expired");
    buffer += _T(" ");
    buffer += time_str;
    buffer += _T(" ");
    buffer += _("ago");
  } else {
    // Currently active
    buffer = _("Active");
  }
  AddReadOnly(_("Now"), nullptr, buffer);
}

void
NOTAMDetailsWidget::AddNOTAMAltitudes(
    const std::optional<struct NOTAM> &notam_opt,
    StaticString<128> &buffer)
{
  FormatAltitudeWithReference(buffer.data(), buffer.capacity(),
                              airspace->GetTop(), notam_opt, false);
  AddReadOnly(_("Top"), nullptr, buffer);

  FormatAltitudeWithReference(buffer.data(), buffer.capacity(),
                              airspace->GetBase(), notam_opt, true);
  AddReadOnly(_("Base"), nullptr, buffer);
}

void
NOTAMDetailsWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                            [[maybe_unused]] const PixelRect &rc) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  StaticString<128> buffer;
  
  // Look up the NOTAM data using the number stored in station_name
  const TCHAR *notam_number = airspace->GetStationName();
  std::optional<struct NOTAM> notam_opt;
  
#ifdef HAVE_HTTP
  if (net_components && net_components->notam && notam_number && notam_number[0] != '\0') {
    // Convert TCHAR to UTF-8 std::string
    WideToUTF8Converter number_conv(notam_number);
    if (number_conv.IsValid()) {
      notam_opt = net_components->notam->FindNOTAMByNumber(number_conv.c_str());
    }
  }
#endif
  
  AddNOTAMIdentifiers(notam_number, notam_opt, buffer);
  
  // NOTAM text (stored in name field)
  AddMultiLine(airspace->GetName());

  AddNOTAMValidity(notam_opt, buffer);
  AddNOTAMAltitudes(notam_opt, buffer);
  
  // Distance calculation
  if (warnings != nullptr) {
    const GeoPoint closest =
      airspace->ClosestPoint(basic.location, warnings->GetProjection());
    const auto distance = closest.Distance(basic.location);
    AddReadOnly(_("Distance"), nullptr, FormatUserDistance(distance));
  }
}
