// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/GroupedListWidget.hpp"
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
#include "Language/FormatText.hpp"
#include "Radio/TransponderMode.hpp"
#include "LogFile.hpp"
#include "util/StaticString.hxx"
#include "util/UTF8.hpp"

#include <cstring>
#include <exception>
#include <optional>
#include <string>

namespace {

[[nodiscard]]
static std::string
SafeString(const std::string &input)
{
  if (input.empty())
    return {};

  return ValidateUTF8(input)
    ? input
    : std::string(_("[Invalid text]"));
}

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

/**
 * Acknowledge the airspace for the day, or take that back, and close
 * the dialog.
 */
static void
AckDayOrEnable(ProtectedAirspaceWarningManager &warnings,
               const ConstAirspacePtr &airspace, WndForm &dialog) noexcept
{
  try {
    const bool acked = warnings.GetAckDay(*airspace);
    warnings.AcknowledgeDay(airspace, !acked);
  } catch (...) {
    LogError(std::current_exception(),
             "Failed to update airspace day acknowledgement");
    Message::AddMessage(_("Failed to update airspace acknowledgement"));
    return;
  }

  dialog.SetModalResult(mrOK);
}

} // namespace

class AirspaceDetailsWidget final
  : public RowFormWidget {
  ConstAirspacePtr airspace;
  ProtectedAirspaceWarningManager *warnings;

public:
  AirspaceDetailsWidget(ConstAirspacePtr _airspace,
                        ProtectedAirspaceWarningManager *_warnings)
    :RowFormWidget(UIGlobals::GetDialogLook()),
     airspace(std::move(_airspace)), warnings(_warnings) {}

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

/**
 * The details of a NOTAM: its text, what identifies it, when it is
 * in effect, and where it is.
 */
class NOTAMDetailsWidget final : public GroupedListWidget {
  const ConstAirspacePtr airspace;
  ProtectedAirspaceWarningManager *const warnings;

public:
  NOTAMDetailsWidget(ConstAirspacePtr _airspace,
                     ProtectedAirspaceWarningManager *_warnings) noexcept
    :GroupedListWidget(UIGlobals::GetDialogLook()),
     airspace(std::move(_airspace)), warnings(_warnings) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

private:
  void AddValidity(const struct NOTAM &notam) noexcept;
};

/**
 * A short "2h" or "3d": how far a point in time is away.
 */
static void
FormatRelativeTime(StaticString<32> &buffer,
                   std::chrono::system_clock::duration duration) noexcept
{
  const auto minutes =
    std::chrono::duration_cast<std::chrono::minutes>(duration).count();

  if (minutes < 60)
    // Translators: %d is number of minutes, keep format short.
    buffer.Format(_("%dm"), static_cast<int>(minutes));
  else if (const auto hours = (minutes + 59) / 60; hours < 48)
    // Translators: %d is number of hours, keep format short.
    buffer.Format(_("%dh"), static_cast<int>(hours));
  else
    // Translators: %d is number of days, keep format short.
    buffer.Format(_("%dd"), static_cast<int>(minutes / 1440));
}

void
NOTAMDetailsWidget::AddValidity(const struct NOTAM &notam) noexcept
{
  char time_buffer[64];

  BrokenDateTime start_dt(notam.start_time);
  FormatISO8601(time_buffer, start_dt);
  AddItem(C_("Setting", "Valid From"), {.value = time_buffer});

  if (notam.end_time_permanent)
    AddItem(C_("Setting", "Valid Until"), {.value = "PERM"});
  else {
    BrokenDateTime end_dt(notam.end_time);
    FormatISO8601(time_buffer, end_dt);
    AddItem(C_("Setting", "Valid Until"), {.value = time_buffer});
  }

  /* the badge says whether the NOTAM is in effect */
  const NMEAInfo &basic = CommonInterface::Basic();
  const auto now = basic.time_available &&
    basic.date_time_utc.IsDatePlausible()
    ? basic.date_time_utc.ToTimePoint()
    : std::chrono::system_clock::now();

  StaticString<32> time_str;
  StaticString<64> badge;
  BadgeStyle badge_style;

  if (now < notam.start_time) {
    FormatRelativeTime(time_str, notam.start_time - now);
    FormatStartsIn(badge, time_str.c_str());
    badge_style = BadgeStyle::PRIMARY;
  } else if (!notam.end_time_permanent && now > notam.end_time) {
    FormatRelativeTime(time_str, now - notam.end_time);
    badge.Format(_("Expired %s ago"), time_str.c_str());
    badge_style = BadgeStyle::DANGER;
  } else {
    badge = _("Active");
    badge_style = BadgeStyle::SUCCESS;
  }

  AddItem(_("Status"), {.badge = badge.c_str(), .badge_style = badge_style});
}

void
NOTAMDetailsWidget::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  StaticString<128> buffer;

  /* the NOTAM itself, found by the stable key stored in the station
     name */
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

  /* the number heads the group of the text, which is the item: it
     has no caption */
  const auto safe_number =
    SafeString(std::string{notam_number != nullptr ? notam_number : ""});

  const char *const airspace_name = airspace->GetName();
  const auto text =
    notam_opt && !notam_opt->text.empty()
    ? SafeString(notam_opt->text)
    : SafeString(airspace_name != nullptr ? airspace_name : "");

  if (!text.empty()) {
    /* the text is not a target for a finger: it keeps the room of a
       one-line row around it */
    AddGroup(safe_number.empty() ? nullptr : safe_number.c_str(),
             {.shrink_vertical_padding = false});
    AddItem(nullptr, {.description = text.c_str(),
                      .description_font = TextFont::MONO});
  }

  if (notam_opt) {
    const auto location = SafeString(notam_opt->location);
    const auto feature_type = SafeString(notam_opt->feature_type);
    const auto series = SafeString(notam_opt->series);

    if (!location.empty() || !feature_type.empty() || !series.empty()) {
      AddGroup();

      if (!location.empty())
        AddItem(_("Location"), {.value = location.c_str(),
                                .value_font = TextFont::MONO});

      if (!feature_type.empty())
        AddItem(C_("Setting", "Q-Code"), {.value = feature_type.c_str(),
                                          .value_font = TextFont::MONO});

      if (!series.empty())
        AddItem(C_("Setting", "Series"), {.value = series.c_str()});
    }

    AddGroup();
    AddValidity(*notam_opt);
  }

  AddGroup();

  FormatAltitudeWithReference(buffer.data(), buffer.capacity(),
                              airspace->GetTop());
  AddItem(_("Top"), {.value = buffer.c_str()});

  FormatAltitudeWithReference(buffer.data(), buffer.capacity(),
                              airspace->GetBase());
  AddItem(_("Base"), {.value = buffer.c_str()});

  if (warnings != nullptr && basic.location_available &&
      basic.location.IsValid()) {
    const GeoPoint closest =
      airspace->ClosestPoint(basic.location, warnings->GetProjection());
    const auto distance = closest.Distance(basic.location);
    AddItem(_("Distance"), {.value = FormatUserDistance(distance).c_str()});
  }

  UpdateLayout();

  GroupedListWidget::Prepare(parent, rc);
}

static bool
dlgAirspaceDetailsModal(ConstAirspacePtr airspace,
                        ProtectedAirspaceWarningManager *warnings,
                        bool browse_parent) noexcept
{
  const bool is_notam = airspace->GetType() == AirspaceClass::NOTAM;

  UI::SingleWindow &parent = UIGlobals::GetMainWindow();
  const DialogLook &look = UIGlobals::GetDialogLook();

  /* the details of an airspace are a few rows which fit into a small
     dialog; a NOTAM brings its text and fills the screen */
  std::optional<WidgetDialog> dialog;
  if (is_notam) {
    dialog.emplace(WidgetDialog::Full{}, parent, look, _("NOTAM Details"));
    dialog->FinishPreliminary(
      std::make_unique<NOTAMDetailsWidget>(airspace, warnings));
  } else {
    dialog.emplace(WidgetDialog::Auto{}, parent, look,
                   _("Airspace Details"));
    dialog->FinishPreliminary(
      std::make_unique<AirspaceDetailsWidget>(airspace, warnings));
  }

  if (warnings != nullptr) {
    const char *label = _("Ack Day");
    try {
      label = warnings->GetAckDay(*airspace) ? _("Enable") : _("Ack Day");
    } catch (const std::exception &e) {
      LogFmt("Failed to query airspace day acknowledgement: {}", e.what());
    } catch (...) {
      LogError(std::current_exception(),
               "Failed to query airspace day acknowledgement");
    }

    dialog->AddButton(label, [warnings, &airspace, &dialog](){
      AckDayOrEnable(*warnings, airspace, *dialog);
    });
  }

  dialog->AddButton(_("Close"), browse_parent ? mrCancel : mrOK);

  if (!browse_parent) {
    dialog->ShowModal();
    return false;
  }

  return dialog->ShowModal() == mrOK;
}

void
dlgAirspaceDetails(ConstAirspacePtr airspace,
                   ProtectedAirspaceWarningManager *warnings)
{
  dlgAirspaceDetailsModal(std::move(airspace), warnings, false);
}

bool
dlgAirspaceDetailsForBrowseParent(
  ConstAirspacePtr airspace,
  ProtectedAirspaceWarningManager *warnings) noexcept
{
  return dlgAirspaceDetailsModal(std::move(airspace), warnings, true);
}
