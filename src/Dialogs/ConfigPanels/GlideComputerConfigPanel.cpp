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

#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "GlideComputerConfigPanel.hpp"
#include "Language/Language.hpp"

static WndForm* wf = NULL;


void
GlideComputerConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  const SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SettingsComputer();

  static const StaticEnumChoice auto_wind_list[] = {
    { D_AUTOWIND_NONE, N_("Manual") },
    { D_AUTOWIND_CIRCLING, N_("Circling") },
    { D_AUTOWIND_ZIGZAG, N_("ZigZag") },
    { D_AUTOWIND_CIRCLING|D_AUTOWIND_ZIGZAG, N_("Both") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpAutoWind"), auto_wind_list,
                   settings_computer.AutoWindMode);

  LoadFormProperty(*wf, _T("prpExternalWind"), settings_computer.ExternalWind);

  static const StaticEnumChoice auto_mc_list[] = {
    { TaskBehaviour::AUTOMC_FINALGLIDE, N_("Final glide") },
    { TaskBehaviour::AUTOMC_CLIMBAVERAGE, N_("Trending average climb") },
    { TaskBehaviour::AUTOMC_BOTH, N_("Both") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpAutoMcMode"), auto_mc_list,
                   settings_computer.task.auto_mc_mode);

  LoadFormProperty(*wf, _T("prpBlockSTF"),
                   settings_computer.EnableBlockSTF);

  LoadFormProperty(*wf, _T("prpEnableNavBaroAltitude"),
                   settings_computer.EnableNavBaroAltitude);

  LoadFormProperty(*wf, _T("prpEnableExternalTriggerCruise"),
                   settings_computer.EnableExternalTriggerCruise);

  static const StaticEnumChoice aver_eff_list[] = {
    { ae15seconds, _T("15 s") },
    { ae30seconds, _T("30 s") },
    { ae60seconds, _T("60 s") },
    { ae90seconds, _T("90 s") },
    { ae2minutes, _T("2 min") },
    { ae3minutes, _T("3 min") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpAverEffTime"), aver_eff_list,
                   settings_computer.AverEffTime);
}


bool
GlideComputerConfigPanel::Save(bool &requirerestart)
{
  bool changed = false;
  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();

  changed |= SaveFormProperty(*wf, _T("prpAutoWind"), szProfileAutoWind,
                              settings_computer.AutoWindMode);

  changed |= SaveFormProperty(*wf, _T("prpExternalWind"),
                              szProfileExternalWind,
                              settings_computer.ExternalWind);

  changed |= SaveFormPropertyEnum(*wf, _T("prpAutoMcMode"),
                                  szProfileAutoMcMode,
                                  settings_computer.task.auto_mc_mode);

  changed |= SaveFormProperty(*wf, _T("prpBlockSTF"),
                              szProfileBlockSTF,
                              settings_computer.EnableBlockSTF);

  changed |= SaveFormProperty(*wf, _T("prpEnableNavBaroAltitude"),
                              szProfileEnableNavBaroAltitude,
                              settings_computer.EnableNavBaroAltitude);

  changed |= SaveFormProperty(*wf, _T("prpEnableExternalTriggerCruise"),
                              szProfileEnableExternalTriggerCruise,
                              settings_computer.EnableExternalTriggerCruise);

  changed |= requirerestart |=
    SaveFormPropertyEnum(*wf, _T("prpAverEffTime"),
                         szProfileAverEffTime, settings_computer.AverEffTime);

  return changed;
}
