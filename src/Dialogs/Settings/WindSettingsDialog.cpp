// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/PickList.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Computer/Settings.hpp"
#include "MapSettings.hpp"
#include "NMEA/Derived.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Profile/Profile.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Form/Button.hpp"
#include "Screen/Layout.hpp"
#include "util/StaticString.hxx"

#include <algorithm>
#include <cmath>

namespace {

static unsigned
PreferredClientWidth(const DialogLook &look) noexcept
{
  const unsigned help = look.list.font->TextSize(
    "Estimate the wind vector during glides.").width;
  const unsigned value =
    look.list.font->TextSize("Direction   000°").width;
  const unsigned text = std::max(help, value);
  return text + 2 * Layout::VptScale(10) +
    2 * Layout::GetTextPadding();
}

/**
 * Wind Settings as one group.  The four switches are kept here until
 * Close.  Speed and direction are written as soon as a value is
 * chosen, which is the manual wind.  The source row follows the
 * wind in use.  Clear drops that manual wind.
 *
 * The dialog floats over the map, sized to this list, and is fitted
 * again when the list or the screen layout changes.
 */
class WindSetup final : private NullBlackboardListener {
  GroupedListWidget *list = nullptr;
  WidgetDialog *dialog = nullptr;
  Button *clear_button = nullptr;

  bool circling_wind;
  bool zig_zag_wind;
  bool external_wind;
  bool trail_drift;

  /** the pilot has chosen a speed or a direction in this dialog */
  bool manual_modified = false;

  /** a value list is open, so a wind update must not rebuild this one */
  bool picking = false;

  struct Drawn {
    bool circling = false;
    bool zigzag = false;
    bool external = false;
    bool trail = false;
    DerivedInfo::WindSource source = DerivedInfo::WindSource::NONE;
    int speed_user = 0;
    int bearing_deg = 0;
    bool manual_available = false;

    bool operator==(const Drawn &) const noexcept = default;
  };

  Drawn drawn;
  bool have_drawn = false;

public:
  WindSetup() noexcept
    :circling_wind(CommonInterface::GetComputerSettings().wind.circling_wind),
     zig_zag_wind(CommonInterface::GetComputerSettings().wind.zig_zag_wind),
     external_wind(CommonInterface::GetComputerSettings().wind.external_wind),
     trail_drift(CommonInterface::GetMapSettings().trail.wind_drift_enabled)
  {}

  void SetList(GroupedListWidget &_list) noexcept {
    list = &_list;
  }

  void SetDialog(WidgetDialog &_dialog) noexcept {
    dialog = &_dialog;
  }

  void SetClearButton(Button *button) noexcept {
    clear_button = button;
  }

  void Refresh() noexcept;
  void SetButtons() noexcept;
  void Start() noexcept;
  void Stop() noexcept;
  void Refit() noexcept;
  void Commit() noexcept;
  void ClearManual() noexcept;

private:
  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;

  void EditSpeed();
  void EditDirection();

  [[nodiscard]]
  SpeedVector Shown() const noexcept;

  [[nodiscard]]
  Drawn Collect() const noexcept;

  void AddSwitch(const char *caption, const char *help,
                 bool checked, bool &field) noexcept;

  void AddValue(const char *caption, const char *help,
                const char *text, GroupedListWidget::Callback edit) noexcept;

  void SetManual(SpeedVector wind) noexcept;
};

void
WindSetup::AddSwitch(const char *caption, const char *help,
                        bool checked, bool &field) noexcept
{
  /* a tap on the label selects the row.  A tap on the switch flips
     it.  Close is what keeps the four switches. */
  list->AddItem(caption, [this, &field]{
    field = !field;
    Refresh();
  }, {.toggle = true, .checked = checked, .help = help});
}

void
WindSetup::AddValue(const char *caption, const char *help,
                       const char *text,
                       GroupedListWidget::Callback edit) noexcept
{
  GroupedListWidget::ItemOptions options;
  options.help = help;
  options.value = text;
  options.chevron = true;
  options.label_selects = true;
  list->AddItem(caption, std::move(edit), options);
}

static void
FormatWindSpeed(double user, StaticString<64> &text)
{
  FormatUserWindSpeed(Units::ToSysWindSpeed(user), text.data(),
                      true, false);
}

static void
FormatWindDirection(double degrees, StaticString<64> &text)
{
  FormatBearing(text.data(), text.capacity(),
                (unsigned)std::lround(degrees));
}

const char *
WindSourceText(DerivedInfo::WindSource source) noexcept
{
  switch (source) {
  case DerivedInfo::WindSource::NONE:
    return _("None");

  case DerivedInfo::WindSource::MANUAL:
    return C_("Status", "Manual");

  case DerivedInfo::WindSource::CIRCLING:
    return _("Circling");

  case DerivedInfo::WindSource::EKF:
    return _("ZigZag");

  case DerivedInfo::WindSource::EXTERNAL:
    return _("External");
  }

  return _("None");
}

SpeedVector
WindSetup::Shown() const noexcept
{
  if (manual_modified)
    return CommonInterface::GetComputerSettings().wind.manual_wind;

  return CommonInterface::Calculated().GetWindOrZero();
}

WindSetup::Drawn
WindSetup::Collect() const noexcept
{
  const SpeedVector shown = Shown();
  const WindSettings &live =
    CommonInterface::GetComputerSettings().wind;

  Drawn next;
  next.circling = circling_wind;
  next.zigzag = zig_zag_wind;
  next.external = external_wind;
  next.trail = trail_drift;
  next.source = manual_modified
    ? DerivedInfo::WindSource::MANUAL
    : CommonInterface::Calculated().wind_source;
  next.speed_user = (int)std::lround(Units::ToUserWindSpeed(shown.norm));
  next.bearing_deg =
    (int)std::lround(shown.bearing.AsBearing().Degrees());
  next.manual_available = live.manual_wind_available;
  return next;
}

void
WindSetup::Refresh() noexcept
{
  const Drawn next = Collect();
  const SpeedVector shown = Shown();

  list->Clear();
  list->AddGroup(nullptr);

  /* the wind in use, then the ways it is estimated, then the map.
     Source names where the two rows under it came from. */
  list->AddItem(C_("Wind source", "Source"),
                {.value = WindSourceText(next.source),
                 .disabled = true});

  StaticString<64> text;
  FormatUserWindSpeed(shown.norm, text.data(), true, false);
  AddValue(_("Speed"), _("Manual adjustment of wind speed."),
           text.c_str(), [this]{ EditSpeed(); });

  FormatBearing(text.data(), text.capacity(), shown.bearing);
  AddValue(_("Direction"), _("Manual adjustment of wind direction."),
           text.c_str(), [this]{ EditDirection(); });

  AddSwitch(_("Circling wind"),
            _("Estimate the wind vector while circling. Requires only a GPS."),
            next.circling, circling_wind);
  AddSwitch(_("ZigZag wind"),
            _("Estimate the wind vector during glides. "
              "Requires an airspeed sensor."),
            next.zigzag, zig_zag_wind);
  AddSwitch(_("External wind"),
            _("Should XCSoar accept wind estimates from other instruments?"),
            next.external, external_wind);
  AddSwitch(_("Trail drift"),
            _("Determines whether the snail trail is drifted with the wind "
              "when displayed in circling mode at near map scales. Switched "
              "Off, the snail trail stays uncompensated for wind drift."),
            next.trail, trail_drift);

  drawn = next;
  have_drawn = true;

  list->UpdateLayout();
  SetButtons();
  Refit();
}

void
WindSetup::SetButtons() noexcept
{
  if (clear_button == nullptr)
    return;

  clear_button->SetEnabled(
    CommonInterface::GetComputerSettings().wind.manual_wind_available);
}

void
WindSetup::Start() noexcept
{
  CommonInterface::GetLiveBlackboard().AddListener(*this);
}

void
WindSetup::Stop() noexcept
{
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
}

void
WindSetup::Refit() noexcept
{
  if (dialog != nullptr)
    dialog->FitToList(dialog->GetParentClientRect(),
                      PreferredClientWidth(dialog->GetLook()));
}

void
WindSetup::Commit() noexcept
{
  WindSettings &settings = CommonInterface::SetComputerSettings().wind;

  const bool auto_changed = settings.circling_wind != circling_wind ||
    settings.zig_zag_wind != zig_zag_wind;
  settings.circling_wind = circling_wind;
  settings.zig_zag_wind = zig_zag_wind;
  if (auto_changed)
    Profile::Set(ProfileKeys::AutoWind, settings.GetLegacyAutoWindMode());

  if (settings.external_wind != external_wind) {
    settings.external_wind = external_wind;
    Profile::Set(ProfileKeys::ExternalWind, settings.external_wind);
  }

  MapSettings &map = CommonInterface::SetMapSettings();
  if (map.trail.wind_drift_enabled != trail_drift) {
    map.trail.wind_drift_enabled = trail_drift;
    Profile::Set(ProfileKeys::TrailDrift, trail_drift);
  }
}

void
WindSetup::ClearManual() noexcept
{
  CommonInterface::SetComputerSettings().wind.manual_wind_available.Clear();
  manual_modified = false;
  Refresh();
}

void
WindSetup::OnCalculatedUpdate(const MoreData &,
                                 const DerivedInfo &)
{
  if (picking)
    return;

  if (!have_drawn || Collect() != drawn)
    Refresh();
  else
    SetButtons();
}

void
WindSetup::SetManual(SpeedVector wind) noexcept
{
  /* a speed of 0 is no wind, so the row must not stay Manual */
  if (!(wind.norm > 0)) {
    CommonInterface::SetComputerSettings().wind.manual_wind_available.Clear();
    manual_modified = false;
    return;
  }

  WindSettings &settings = CommonInterface::SetComputerSettings().wind;
  settings.manual_wind = wind;
  settings.manual_wind_available.Update(CommonInterface::Basic().clock);
  manual_modified = true;
}

void
WindSetup::EditSpeed()
{
  const SpeedVector shown = Shown();
  double value = Units::ToUserWindSpeed(shown.norm);
  double max_value = Units::ToUserWindSpeed(Units::ToSysUnit(
    200, Unit::KILOMETER_PER_HOUR));
  if (value > max_value)
    max_value = value;

  picking = true;
  const bool ok = PickList(_("Speed"), value, 0, max_value, 1,
                           FormatWindSpeed);
  picking = false;
  if (!ok)
    return;

  SetManual(SpeedVector(shown.bearing, Units::ToSysWindSpeed(value)));
  Refresh();
}

void
WindSetup::EditDirection()
{
  const SpeedVector shown = Shown();
  double value = shown.bearing.AsBearing().Degrees();

  picking = true;
  const bool ok = PickList(_("Direction"), value, 0, 355, 5,
                           FormatWindDirection);
  picking = false;
  if (!ok)
    return;

  SetManual(SpeedVector(Angle::Degrees(value), shown.norm));
  Refresh();
}

} // namespace

void
ShowWindSettingsDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  auto *list = new GroupedListWidget(look);
  WindSetup setup;
  setup.SetList(*list);

  const PixelRect rc{Layout::Scale(PixelSize{220u, 220u})};
  WidgetDialog dialog(UIGlobals::GetMainWindow(), look, rc,
                            _("Wind Settings"), list);
  setup.SetDialog(dialog);
  setup.SetClearButton(dialog.AddButton(_("Clear"), [&setup]{
    setup.ClearManual();
  }));
  dialog.AddButton(_("Close"), mrOK);

  dialog.EnableCursorSelection();
  dialog.ResyncButtonPanelSelection();
  list->SetActionBar(dialog.GetButtonPanel());
  list->SetCursorCallback([&setup](int){
    setup.Refit();
  });

  dialog.PrepareWidget();
  setup.Refresh();
  setup.Start();
  const int result = dialog.ShowModal();
  setup.Stop();

  if (result == mrOK)
    setup.Commit();
}
