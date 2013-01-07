/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "WindSettingsPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Float.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Units/Units.hpp"

WindSettingsPanel::WindSettingsPanel(bool _edit_manual_wind,
                                     bool _edit_trail_drift)
  :RowFormWidget(UIGlobals::GetDialogLook()),
   edit_manual_wind(_edit_manual_wind),
   edit_trail_drift(_edit_trail_drift) {}

void
WindSettingsPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const NMEAInfo &basic = CommonInterface::Basic();
  const WindSettings &settings = CommonInterface::GetComputerSettings().wind;
  const MapSettings &map_settings = CommonInterface::GetMapSettings();

  static constexpr StaticEnumChoice auto_wind_list[] = {
    { AUTOWIND_NONE, N_("Manual"),
      N_("When the algorithm is switched off, the pilot is responsible for setting the wind estimate.") },
    { AUTOWIND_CIRCLING, N_("Circling"),
      N_("Requires only a GPS source.") },
    { AUTOWIND_ZIGZAG, N_("ZigZag"),
      N_("Requires GPS and an intelligent vario with airspeed output.") },
    { AUTOWIND_CIRCLING | AUTOWIND_ZIGZAG, N_("Both"),
      N_("Use ZigZag and circling.") },
    { 0 }
  };

  AddEnum(_("Auto wind"),
          _("This allows switching on or off the automatic wind algorithm."),
          auto_wind_list, settings.GetLegacyAutoWindMode());

  AddBoolean(_("Prefer external wind"),
             _("If enabled, then the wind vector received from external devices overrides "
                 "XCSoar's internal wind calculation."),
             settings.use_external_wind);

  if (edit_trail_drift)
    AddBoolean(_("Trail drift"),
               _("Determines whether the snail trail is drifted with the wind "
                 "when displayed in circling mode."),
               map_settings.trail.wind_drift_enabled);
  else
    AddDummy();

  if (edit_manual_wind) {
    external_wind = settings.use_external_wind &&
      basic.external_wind_available;

    SpeedVector manual_wind = CommonInterface::Calculated().GetWindOrZero();

    WndProperty *wp =
      AddFloat(_("Speed"), _("Manual adjustment of wind speed."),
               _T("%.0f %s"), _T("%.0f"),
               fixed(0),
               Units::ToUserWindSpeed(Units::ToSysUnit(fixed(200),
                                                       Unit::KILOMETER_PER_HOUR)),
               fixed(1), false,
               Units::ToUserWindSpeed(manual_wind.norm));
    wp->SetEnabled(!external_wind);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetWindSpeedName());
    wp->RefreshDisplay();

    wp = AddAngle(_("Direction"), _("Manual adjustment of wind direction."),
                  manual_wind.bearing, 5u, false);
    wp->SetEnabled(!external_wind);
  }
}

bool
WindSettingsPanel::Save(bool &_changed, bool &_require_restart)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  WindSettings &settings = CommonInterface::SetComputerSettings().wind;
  MapSettings &map_settings = CommonInterface::SetMapSettings();

  bool changed = false;

  unsigned auto_wind_mode = settings.GetLegacyAutoWindMode();
  if (SaveValueEnum(AutoWind, ProfileKeys::AutoWind, auto_wind_mode)) {
    settings.SetLegacyAutoWindMode(auto_wind_mode);
    changed = true;
  }

  changed |= SaveValue(ExternalWind, ProfileKeys::ExternalWind,
                       settings.use_external_wind);

  if (edit_trail_drift)
    changed |= SaveValue(TrailDrift, ProfileKeys::TrailDrift,
                         map_settings.trail.wind_drift_enabled);

  if (edit_manual_wind && !external_wind) {
    settings.manual_wind.norm = Units::ToSysWindSpeed(GetValueFloat(Speed));
    settings.manual_wind.bearing = GetValueAngle(Direction);
    settings.manual_wind_available.Update(basic.clock);
  }

  _changed |= changed;
  return true;
}
