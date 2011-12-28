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
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "GlideComputerConfigPanel.hpp"
#include "Language/Language.hpp"
#include "Form/Form.hpp"
#include "Form/RowFormWidget.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  AutoMcMode,
  BlockSTF,
  EnableNavBaroAltitude,
  EnableExternalTriggerCruise,
  AverEffTime
};

class GlideComputerConfigPanel : public RowFormWidget {
public:
  GlideComputerConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(150)) {}

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
};

void
GlideComputerConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const ComputerSettings &settings_computer = XCSoarInterface::GetComputerSettings();

  RowFormWidget::Prepare(parent, rc);

  static gcc_constexpr_data StaticEnumChoice auto_mc_list[] = {
    { TaskBehaviour::AUTOMC_FINALGLIDE, N_("Final glide"),
      N_("Adjusts MC for fastest arrival.  For OLC sprint tasks, the MacCready is adjusted in "
          "order to cover the greatest distance in the remaining time and reach the finish height.") },
    { TaskBehaviour::AUTOMC_CLIMBAVERAGE, N_("Trending average climb"),
      N_("Sets MC to the trending average climb rate based on all climbs.") },
    { TaskBehaviour::AUTOMC_BOTH, N_("Both"),
      N_("Uses trending average during task, then fastest arrival when in final glide mode.") },
    { 0 }
  };

  AddEnum(_("Auto MC mode"),
          _("This option defines which auto MacCready algorithm is used."),
          auto_mc_list, settings_computer.task.auto_mc_mode);

  // TODO All below is for the Expert
  AddBoolean(_("Block speed to fly"),
             _("If enabled, the command speed in cruise is set to the MacCready speed to fly in "
                 "no vertical air-mass movement. If disabled, the command speed in cruise is set "
                 "to the dolphin speed to fly, equivalent to the MacCready speed with vertical "
                 "air-mass movement."),
             settings_computer.block_stf_enabled);

  AddBoolean(_("Nav. by baro altitude"),
             _("When enabled and if connected to a barometric altimeter, barometric altitude is "
                 "used for all navigation functions. Otherwise GPS altitude is used."),
             settings_computer.nav_baro_altitude_enabled);

  AddBoolean(_("Flap forces cruise"),
             _("When Vega variometer is connected and this option is true, the positive flap "
                 "setting switches the flight mode between circling and cruise."),
             settings_computer.external_trigger_cruise_enabled);

  static gcc_constexpr_data StaticEnumChoice aver_eff_list[] = {
    { ae15seconds, _T("15 s"), N_("Preferred period for paragliders.") },
    { ae30seconds, _T("30 s") },
    { ae60seconds, _T("60 s") },
    { ae90seconds, _T("90 s"), N_("Preferred period for gliders.") },
    { ae2minutes, _T("2 min") },
    { ae3minutes, _T("3 min") },
    { 0 }
  };

  AddEnum(_("L/D average period"),
          _("Here you can decide on how many seconds of flight this calculation must be done. "
              "Normally for gliders a good value is 90-120 seconds, and for paragliders 15 seconds."),
          aver_eff_list, settings_computer.average_eff_time);
}

bool
GlideComputerConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();

  changed |= SaveValueEnum(AutoMcMode, szProfileAutoMcMode, settings_computer.task.auto_mc_mode);

  changed |= SaveValue(BlockSTF, szProfileBlockSTF, settings_computer.block_stf_enabled);

  changed |= SaveValue(EnableNavBaroAltitude, szProfileEnableNavBaroAltitude,
                       settings_computer.nav_baro_altitude_enabled);

  changed |= SaveValue(EnableExternalTriggerCruise, szProfileEnableExternalTriggerCruise,
                       settings_computer.external_trigger_cruise_enabled);

  changed |= require_restart |=
      SaveValueEnum(AverEffTime, szProfileAverEffTime, settings_computer.average_eff_time);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateGlideComputerConfigPanel()
{
  return new GlideComputerConfigPanel();
}
