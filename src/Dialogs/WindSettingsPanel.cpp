/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"

WindSettingsPanel::WindSettingsPanel()
  :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(150)) {}

void
WindSettingsPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const WindSettings &settings = XCSoarInterface::GetComputerSettings();

  static gcc_constexpr_data StaticEnumChoice auto_wind_list[] = {
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

  AddBoolean(_("External wind"),
             _("If enabled, then the wind vector received from external devices overrides "
                 "XCSoar's internal wind calculation."),
             settings.use_external_wind);
}

bool
WindSettingsPanel::Save(bool &_changed, bool &_require_restart)
{
  WindSettings &settings = XCSoarInterface::SetComputerSettings();

  bool changed = false;

  unsigned auto_wind_mode = settings.GetLegacyAutoWindMode();
  if (SaveValueEnum(AutoWind, szProfileAutoWind, auto_wind_mode)) {
    settings.SetLegacyAutoWindMode(auto_wind_mode);
    changed = true;
  }

  changed |= SaveValue(ExternalWind, szProfileExternalWind,
                       settings.use_external_wind);

  _changed |= changed;
  return true;
}
