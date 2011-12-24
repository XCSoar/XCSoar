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
#include "Form/Form.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"

class GlideComputerConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

void
GlideComputerConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
GlideComputerConfigPanel::Hide()
{
  XMLWidget::Hide();
}

void
GlideComputerConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent,
             Layout::landscape ? _T("IDR_XML_GLIDECOMPUTERCONFIGPANEL") :
                               _T("IDR_XML_GLIDECOMPUTERCONFIGPANEL_L"));


  const SETTINGS_COMPUTER &settings_computer = XCSoarInterface::GetSettingsComputer();

  static gcc_constexpr_data StaticEnumChoice auto_wind_list[] = {
    { AUTOWIND_NONE, N_("Manual") },
    { AUTOWIND_CIRCLING, N_("Circling") },
    { AUTOWIND_ZIGZAG, N_("ZigZag") },
    { AUTOWIND_CIRCLING | AUTOWIND_ZIGZAG, N_("Both") },
    { 0 }
  };

  LoadFormProperty(form, _T("prpAutoWind"), auto_wind_list,
                   settings_computer.auto_wind_mode);

  LoadFormProperty(form, _T("prpExternalWind"), settings_computer.use_external_wind);

  static gcc_constexpr_data StaticEnumChoice auto_mc_list[] = {
    { TaskBehaviour::AUTOMC_FINALGLIDE, N_("Final glide") },
    { TaskBehaviour::AUTOMC_CLIMBAVERAGE, N_("Trending average climb") },
    { TaskBehaviour::AUTOMC_BOTH, N_("Both") },
    { 0 }
  };

  LoadFormProperty(form, _T("prpAutoMcMode"), auto_mc_list,
                   settings_computer.task.auto_mc_mode);

  LoadFormProperty(form, _T("prpBlockSTF"),
                   settings_computer.block_stf_enabled);

  LoadFormProperty(form, _T("prpEnableNavBaroAltitude"),
                   settings_computer.nav_baro_altitude_enabled);

  LoadFormProperty(form, _T("prpEnableExternalTriggerCruise"),
                   settings_computer.external_trigger_cruise_enabled);

  static gcc_constexpr_data StaticEnumChoice aver_eff_list[] = {
    { ae15seconds, _T("15 s") },
    { ae30seconds, _T("30 s") },
    { ae60seconds, _T("60 s") },
    { ae90seconds, _T("90 s") },
    { ae2minutes, _T("2 min") },
    { ae3minutes, _T("3 min") },
    { 0 }
  };

  LoadFormProperty(form, _T("prpAverEffTime"), aver_eff_list,
                   settings_computer.average_eff_time);
}

bool
GlideComputerConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();

  changed |= SaveFormProperty(form, _T("prpAutoWind"), szProfileAutoWind,
                              settings_computer.auto_wind_mode);

  changed |= SaveFormProperty(form, _T("prpExternalWind"),
                              szProfileExternalWind,
                              settings_computer.use_external_wind);

  changed |= SaveFormPropertyEnum(form, _T("prpAutoMcMode"),
                                  szProfileAutoMcMode,
                                  settings_computer.task.auto_mc_mode);

  changed |= SaveFormProperty(form, _T("prpBlockSTF"),
                              szProfileBlockSTF,
                              settings_computer.block_stf_enabled);

  changed |= SaveFormProperty(form, _T("prpEnableNavBaroAltitude"),
                              szProfileEnableNavBaroAltitude,
                              settings_computer.nav_baro_altitude_enabled);

  changed |= SaveFormProperty(form, _T("prpEnableExternalTriggerCruise"),
                              szProfileEnableExternalTriggerCruise,
                              settings_computer.external_trigger_cruise_enabled);

  changed |= require_restart |=
    SaveFormPropertyEnum(form, _T("prpAverEffTime"),
                         szProfileAverEffTime, settings_computer.average_eff_time);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateGlideComputerConfigPanel()
{
  return new GlideComputerConfigPanel();
}
