/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "GaugesConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Form/Util.hpp"
#include "Interface.hpp"

static WndForm* wf = NULL;


void
GaugesConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  const UISettings &ui_settings = CommonInterface::GetUISettings();

  LoadFormProperty(*wf, _T("prpEnableFLARMGauge"),
                   ui_settings.enable_flarm_gauge);

  LoadFormProperty(*wf, _T("prpAutoCloseFlarmDialog"),
                   ui_settings.auto_close_flarm_dialog);

  LoadFormProperty(*wf, _T("prpEnableTAGauge"),
                   ui_settings.enable_thermal_assistant_gauge);

  LoadFormProperty(*wf, _T("prpEnableThermalProfile"),
                   XCSoarInterface::SettingsMap().show_thermal_profile);
}


bool
GaugesConfigPanel::Save()
{
  UISettings &ui_settings = CommonInterface::SetUISettings();

  bool changed = false;

  changed |= SaveFormProperty(*wf, _T("prpEnableFLARMGauge"),
                              szProfileEnableFLARMGauge,
                              ui_settings.enable_flarm_gauge);

  changed |= SaveFormProperty(*wf, _T("prpAutoCloseFlarmDialog"),
                              szProfileAutoCloseFlarmDialog,
                              ui_settings.auto_close_flarm_dialog);

  changed |= SaveFormProperty(*wf, _T("prpEnableTAGauge"),
                              szProfileEnableTAGauge,
                              ui_settings.enable_thermal_assistant_gauge);

  changed |= SaveFormProperty(*wf, _T("prpEnableThermalProfile"),
                              szProfileEnableThermalProfile,
                              XCSoarInterface::SetSettingsMap().show_thermal_profile);

  return changed;
}
