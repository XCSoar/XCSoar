// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindSettingsPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/ProfileMap.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Float.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Units/Units.hpp"

WindSettingsPanel::WindSettingsPanel(bool _edit_manual_wind,
                                     bool _clear_manual_button,
                                     bool _edit_trail_drift) noexcept
  :RowFormWidget(UIGlobals::GetDialogLook()),
   edit_manual_wind(_edit_manual_wind),
   clear_manual_button(_clear_manual_button),
   edit_trail_drift(_edit_trail_drift),
   clear_manual_window(nullptr) {}

void
WindSettingsPanel::ClearManual() noexcept
{
  CommonInterface::SetComputerSettings().wind.manual_wind_available.Clear();
  manual_modified = false;
  UpdateVector();
}

void
WindSettingsPanel::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const WindSettings &settings = CommonInterface::GetComputerSettings().wind;
  const MapSettings &map_settings = CommonInterface::GetMapSettings();

  AddBoolean(_("Circling wind"),
             _("Estimate the wind vector while circling. Requires only a GPS."),
             settings.circling_wind);

  AddBoolean(_("ZigZag wind"),
             _("Estimate the wind vector during glides. Requires an airspeed sensor."),
             settings.zig_zag_wind);

  AddBoolean(_("External wind"),
             _("Should XCSoar accept wind estimates from other instruments?"),
             settings.external_wind);

  if (edit_trail_drift)
    AddBoolean(_("Trail drift"),
               _("Determines whether the snail trail is drifted with the wind "
                 "when displayed in circling mode. Switched Off, "
                 "the snail trail stays uncompensated for wind drift."),
               map_settings.trail.wind_drift_enabled);
  else
    AddDummy();

  if (edit_manual_wind) {
    SpeedVector manual_wind = CommonInterface::Calculated().GetWindOrZero();

    AddReadOnly(_("Source"));

    WndProperty *wp =
      AddFloat(_("Speed"), _("Manual adjustment of wind speed."),
               "%.0f %s", "%.0f",
               0,
               Units::ToUserWindSpeed(Units::ToSysUnit(200,
                                                       Unit::KILOMETER_PER_HOUR)),
               1, false,
               Units::ToUserWindSpeed(manual_wind.norm),
               this);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetWindSpeedName());
    wp->RefreshDisplay();

    wp = AddAngle(_("Direction"), _("Manual adjustment of wind direction."),
                  manual_wind.bearing, 5u, false,
                  this);

    manual_modified = false;
  }

  if (clear_manual_button)
    AddButton(_("Clear"), [this](){ ClearManual(); });

  UpdateVector();
}

void
WindSettingsPanel::Show(const PixelRect &rc) noexcept
{
  if (edit_manual_wind) {
    UpdateVector();
    CommonInterface::GetLiveBlackboard().AddListener(*this);
  }

  RowFormWidget::Show(rc);
}

void
WindSettingsPanel::Hide() noexcept
{
  RowFormWidget::Hide();

  if (edit_manual_wind)
    CommonInterface::GetLiveBlackboard().RemoveListener(*this);
}

bool
WindSettingsPanel::Save(bool &_changed) noexcept
{
  WindSettings &settings = CommonInterface::SetComputerSettings().wind;
  MapSettings &map_settings = CommonInterface::SetMapSettings();

  bool changed = false;

  bool auto_wind_changed = SaveValue(CIRCLING_WIND, settings.circling_wind);
  auto_wind_changed |= SaveValue(ZIG_ZAG_WIND, settings.zig_zag_wind);

  if (auto_wind_changed) {
    changed = true;
    Profile::Set(ProfileKeys::AutoWind, settings.GetLegacyAutoWindMode());
  }

  changed |= SaveValue(EXTERNAL_WIND, ProfileKeys::ExternalWind,
                       settings.external_wind);

  if (edit_trail_drift)
    changed |= SaveValue(TrailDrift, ProfileKeys::TrailDrift,
                         map_settings.trail.wind_drift_enabled);

  _changed |= changed;
  return true;
}

void
WindSettingsPanel::OnModified(DataField &df) noexcept
{
  if (!edit_manual_wind)
    return;

  const NMEAInfo &basic = CommonInterface::Basic();

  if (&df == &GetDataField(Speed) || &df == &GetDataField(Direction)) {
    WindSettings &settings = CommonInterface::SetComputerSettings().wind;
    settings.manual_wind.norm = Units::ToSysWindSpeed(GetValueFloat(Speed));
    settings.manual_wind.bearing = GetValueAngle(Direction);
    settings.manual_wind_available.Update(basic.clock);
    manual_modified = true;
  }

  UpdateVector();
}

void
WindSettingsPanel::UpdateVector() noexcept
{
  if (!edit_manual_wind)
    return;

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const WindSettings &settings = CommonInterface::SetComputerSettings().wind;

  const char *source = nullptr;
  switch (manual_modified
          ? DerivedInfo::WindSource::MANUAL
          : calculated.wind_source) {
  case DerivedInfo::WindSource::NONE:
    source = _("None");
    break;

  case DerivedInfo::WindSource::MANUAL:
    source = _("Manual");
    break;

  case DerivedInfo::WindSource::CIRCLING:
    source = _("Circling");
    break;

  case DerivedInfo::WindSource::EKF:
    source = _("ZigZag");
    break;

  case DerivedInfo::WindSource::EXTERNAL:
    source = _("External");
    break;
  }

  SetText(SOURCE, source);

  if (!manual_modified && !settings.manual_wind_available) {
    SpeedVector wind = CommonInterface::Calculated().GetWindOrZero();
    LoadValue(Speed, Units::ToUserWindSpeed(wind.norm));
    LoadValue(Direction, wind.bearing);
  }

  const bool visible = settings.manual_wind_available;
  if (clear_manual_button)
    SetRowEnabled(CLEAR_MANUAL_BUTTON, visible);
  else if (clear_manual_window != nullptr)
    clear_manual_window->SetEnabled(visible);
}

void
WindSettingsPanel::OnCalculatedUpdate([[maybe_unused]] const MoreData &basic,
                                      [[maybe_unused]] const DerivedInfo &calculated)
{
  UpdateVector();
}
