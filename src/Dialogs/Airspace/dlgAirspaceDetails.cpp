// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Airspace/AirspaceClass.hpp"
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
#include "NOTAM/NOTAM.hpp"
#include "ActionInterface.hpp"
#include <Message.hpp>
#include "Geo/AltitudeReference.hpp"
#include "Language/Language.hpp"
#include "TransponderMode.hpp"
#include "LogFile.hpp"
#include "util/StaticString.hxx"

#include <cstring>
#include <cassert>
#include <exception>

namespace {


void
FormatAltitudeWithReference(char *buffer, size_t buffer_size,
                            const AirspaceAltitude &altitude) noexcept
{
  if (buffer == nullptr || buffer_size == 0)
    return;

  AirspaceFormatter::FormatAltitudeShort(buffer, altitude, true);

  const size_t current_len = std::strlen(buffer);
  const size_t remaining = buffer_size > current_len
    ? buffer_size - current_len - 1
    : 0;

  if (remaining == 0)
    return;

  const char *suffix = nullptr;
  if (altitude.reference == AltitudeReference::MSL)
    suffix = " MSL";
  else if (altitude.reference == AltitudeReference::AGL)
    suffix = " AGL";

  if (suffix != nullptr) {
    const size_t suffix_len = std::strlen(suffix);
    if (suffix_len <= remaining)
      std::memcpy(buffer + current_len, suffix, suffix_len + 1);
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
  char buffer2[5];

  transponderCode.Format(buffer2, sizeof(buffer2));

  if (transponderCode.IsDefined()) {
    AddReadOnly(_("Squawk code"), nullptr, buffer2);
    AddButton(_("Set Squawk Code"), [transponderCode]() {
      ActionInterface::SetTransponderCode(transponderCode);
    });
  }

  if (airspace->GetRadioFrequency().IsDefined()) {
    if (airspace->GetRadioFrequency().Format(buffer.data(), buffer.capacity()) !=
        nullptr) {
      buffer += " MHz";
      AddReadOnly(_("Radio"), nullptr, buffer);
    }

    const char *frequencyName = airspace->GetName();
    const char *stationName = airspace->GetStationName();

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

  AddReadOnly(_("Class"), nullptr,
              AirspaceFormatter::GetClassShort(*airspace));
  AddReadOnly(_("Type"), nullptr, AirspaceFormatter::GetType(*airspace));

  AirspaceFormatter::FormatAltitude(buffer.data(), airspace->GetTop());
  AddReadOnly(_("Top"), nullptr, buffer);

  AirspaceFormatter::FormatAltitude(buffer.data(), airspace->GetBase());
  AddReadOnly(_("Base"), nullptr, buffer);

  if (warnings != nullptr && basic.location_available &&
      basic.location.IsValid()) {
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

  try {
    const bool acked = warnings->GetAckDay(*airspace);
    warnings->AcknowledgeDay(airspace, !acked);
  } catch (...) {
    LogError(std::current_exception(),
             "Failed to update airspace day acknowledgement");
    Message::AddMessage(_("Failed to update airspace acknowledgement"));
    return;
  }

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
  void AddNOTAMIdentifiers(const char *notam_number,
                           const std::optional<struct NOTAM> &notam_opt);
  void AddNOTAMValidity(const std::optional<struct NOTAM> &notam_opt,
                        StaticString<128> &buffer);
  void AddNOTAMAltitudes(StaticString<128> &buffer);
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
  
  const char *title = is_notam ? _("NOTAM Details") : _("Airspace Details");
  
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      title, widget);

  if (warnings != nullptr) {
    widget->dialog = &dialog;
    const char *label = _("Ack Day");
    try {
      label = warnings->GetAckDay(*airspace) ? _("Enable") : _("Ack Day");
    } catch (const std::exception &e) {
      LogFmt("Failed to query airspace day acknowledgement: {}", e.what());
    } catch (...) {
      LogError(std::current_exception(),
               "Failed to query airspace day acknowledgement");
    }

    dialog.AddButton(label, [widget](){ widget->AckDayOrEnable(); });
  }
  dialog.AddButton(_("Close"), mrOK);

  dialog.ShowModal();
}

void
NOTAMDetailsWidget::AddNOTAMIdentifiers(const char *notam_number,
                                        const std::optional<struct NOTAM> &notam_opt)
{
  // Display NOTAM number
  if (notam_number && notam_number[0] != '\0') {
    const auto safe_notam_number = SafeString(std::string{notam_number});
    AddReadOnly(_("NOTAM"), nullptr, safe_notam_number.c_str());
  }

  // Display ICAO location if we found the NOTAM
  if (notam_opt && !notam_opt->location.empty()) {
    AddReadOnly(_("Location"), nullptr, notam_opt->location.c_str());
  }

  // Display Q-code (feature type)
  if (notam_opt && !notam_opt->feature_type.empty()) {
    AddReadOnly(_("Q-Code"), nullptr, notam_opt->feature_type.c_str());
  }

  // NOTAM Series (if available)
  if (notam_opt && !notam_opt->series.empty()) {
    AddReadOnly(_("Series"), nullptr, notam_opt->series.c_str());
  }
}

void
NOTAMDetailsWidget::AddNOTAMValidity(
    const std::optional<struct NOTAM> &notam_opt,
    StaticString<128> &buffer)
{
  if (!notam_opt)
    return;

  char time_buffer[64];

  // Effective start - format as friendly date/time
  BrokenDateTime start_dt(notam_opt->start_time);
  FormatISO8601(time_buffer, start_dt);
  AddReadOnly(_("Valid From"), nullptr, time_buffer);

  // Effective end
  BrokenDateTime end_dt(notam_opt->end_time);
  // Check if it's a far future date (PERM = permanent)
  if (notam_opt->end_time >= NOTAMTime::PermanentEndTime()) {
    AddReadOnly(_("Valid Until"), nullptr, "PERM");
  } else {
    FormatISO8601(time_buffer, end_dt);
    AddReadOnly(_("Valid Until"), nullptr, time_buffer);
  }

  // Status indicator
  const NMEAInfo &basic = CommonInterface::Basic();
  const auto now = basic.time_available && basic.date_time_utc.IsDatePlausible()
    ? basic.date_time_utc.ToTimePoint()
    : std::chrono::system_clock::now();
  StaticString<32> time_str;
  if (now < notam_opt->start_time) {
    // Not yet active
    const auto starts_in =
      std::chrono::duration_cast<std::chrono::minutes>(
        notam_opt->start_time - now);
    if (starts_in.count() < 60) {
      // Translators: %d is number of minutes, keep format short.
      time_str.Format(_("%dm"), static_cast<int>(starts_in.count()));
    } else {
      const auto starts_in_hours = (starts_in.count() + 59) / 60;
      if (starts_in_hours < 48) {
        // Translators: %d is number of hours, keep format short.
        time_str.Format(_("%dh"), static_cast<int>(starts_in_hours));
      } else {
        const auto days = starts_in_hours / 24;
        // Translators: %d is number of days, keep format short.
        time_str.Format(_("%dd"), static_cast<int>(days));
      }
    }
    buffer.Format(_("Starts in %s"), time_str.c_str());
  } else if (now > notam_opt->end_time) {
    // Expired
    const auto expired_ago =
      std::chrono::duration_cast<std::chrono::minutes>(
        now - notam_opt->end_time);
    if (expired_ago.count() < 60) {
      // Translators: %d is number of minutes, keep format short.
      time_str.Format(_("%dm"), static_cast<int>(expired_ago.count()));
    } else {
      const auto expired_minutes = expired_ago.count();
      const auto expired_hours = (expired_minutes + 59) / 60;
      if (expired_hours < 48) {
        // Translators: %d is number of hours, keep format short.
        time_str.Format(_("%dh"), static_cast<int>(expired_hours));
      } else {
        const auto days = (expired_minutes + 1439) / 1440;
        // Translators: %d is number of days, keep format short.
        time_str.Format(_("%dd"), static_cast<int>(days));
      }
    }
    buffer.Format(_("Expired %s ago"), time_str.c_str());
  } else {
    // Currently active
    buffer = _("Active");
  }
  AddReadOnly(_("Now"), nullptr, buffer);
}

void
NOTAMDetailsWidget::AddNOTAMAltitudes(StaticString<128> &buffer)
{
  FormatAltitudeWithReference(buffer.data(), buffer.capacity(),
                              airspace->GetTop());
  AddReadOnly(_("Top"), nullptr, buffer);

  FormatAltitudeWithReference(buffer.data(), buffer.capacity(),
                              airspace->GetBase());
  AddReadOnly(_("Base"), nullptr, buffer);
}

void
NOTAMDetailsWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                            [[maybe_unused]] const PixelRect &rc) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  StaticString<128> buffer;
  
  // Look up the NOTAM data using the stable key stored in station_name.
  const char *notam_number = airspace->GetStationName();
  std::optional<struct NOTAM> notam_opt;
  
#ifdef HAVE_HTTP
  if (net_components && net_components->notam && notam_number &&
      notam_number[0] != '\0') {
    try {
      notam_opt =
        net_components->notam->FindNOTAMByNumber(notam_number);
    } catch (...) {
      LogError(std::current_exception(), "Failed to lookup NOTAM");
    }
  }
#endif
  
  AddNOTAMIdentifiers(notam_number, notam_opt);
  
  // NOTAM text (stored in name field)
  AddMultiLine(notam_opt && !notam_opt->text.empty()
               ? notam_opt->text.c_str()
               : airspace->GetName());

  AddNOTAMValidity(notam_opt, buffer);
  AddNOTAMAltitudes(buffer);
  
  // Distance calculation
  if (warnings != nullptr && basic.location_available &&
      basic.location.IsValid()) {
    const GeoPoint closest =
      airspace->ClosestPoint(basic.location, warnings->GetProjection());
    const auto distance = closest.Distance(basic.location);
    AddReadOnly(_("Distance"), nullptr, FormatUserDistance(distance));
  }
}
