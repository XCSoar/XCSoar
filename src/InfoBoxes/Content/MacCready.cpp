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

#include "InfoBoxes/Content/MacCready.hpp"
#include "InfoBoxes/Data.hpp"
#include "InfoBoxes/Panel/MacCreadyEdit.hpp"
#include "InfoBoxes/Panel/MacCreadySetup.hpp"
#include "Interface.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Language/Language.hpp"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Screen/Layout.hpp"
#include "Profile/Profile.hpp"
#include "Util/Macros.hpp"

#include <tchar.h>
#include <stdio.h>

static void
SetVSpeed(InfoBoxData &data, fixed value)
{
  TCHAR buffer[32];
  FormatUserVerticalSpeed(value, buffer, false);
  data.SetValue(buffer[0] == _T('+') ? buffer + 1 : buffer);
  data.SetValueUnit(Units::current.vertical_speed_unit);
}

/*
 * Subpart callback function pointers
 */

static constexpr InfoBoxContentMacCready::PanelContent panels[] = {
  InfoBoxContentMacCready::PanelContent (
    N_("Edit"),
    LoadMacCreadyEditPanel),

  InfoBoxContentMacCready::PanelContent (
    N_("Setup"),
    LoadMacCreadySetupPanel),
};

static constexpr InfoBoxContentMacCready::DialogContent dlgContent = {
  ARRAY_SIZE(panels), &panels[0],
};

const InfoBoxContentMacCready::DialogContent*
InfoBoxContentMacCready::GetDialogContent() {
  return &dlgContent;
}

/*
 * Subpart normal operations
 */

void
InfoBoxContentMacCready::Update(InfoBoxData &data)
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  data.SetTitle(settings_computer.task.auto_mc ? _("MC AUTO") : _("MC MANUAL"));

  SetVSpeed(data, settings_computer.polar.glide_polar_task.GetMC());

  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;
  data.SetCommentFromSpeed(common_stats.V_block, false);
}

bool
InfoBoxContentMacCready::HandleKey(const InfoBoxKeyCodes keycode)
{
  if (protected_task_manager == NULL)
    return false;

  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();
  const GlidePolar &polar = settings_computer.polar.glide_polar_task;
  TaskBehaviour &task_behaviour = CommonInterface::SetComputerSettings().task;
  fixed mc = polar.GetMC();

  switch (keycode) {
  case ibkUp:
    mc = std::min(mc + Units::ToSysVSpeed(GetUserVerticalSpeedStep()),
                  fixed(5));
    ActionInterface::SetManualMacCready(mc);
    return true;

  case ibkDown:
    mc = std::max(mc - Units::ToSysVSpeed(GetUserVerticalSpeedStep()),
                  fixed_zero);
    ActionInterface::SetManualMacCready(mc);
    return true;

  case ibkLeft:
    task_behaviour.auto_mc = false;
    Profile::Set(ProfileKeys::AutoMc, false);
    return true;

  case ibkRight:
    task_behaviour.auto_mc = true;
    Profile::Set(ProfileKeys::AutoMc, true);
    return true;

  case ibkEnter:
    task_behaviour.auto_mc = !task_behaviour.auto_mc;
    Profile::Set(ProfileKeys::AutoMc, task_behaviour.auto_mc);
    return true;
  }
  return false;
}

bool
InfoBoxContentMacCready::HandleQuickAccess(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return false;

  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();
  const GlidePolar &polar = settings_computer.polar.glide_polar_task;
  fixed mc = polar.GetMC();

  if (_tcscmp(misc, _T("+0.1")) == 0) {
    mc = std::min(mc + Units::ToSysVSpeed(GetUserVerticalSpeedStep()),
                  fixed(5));
    ActionInterface::SetManualMacCready(mc);
    return true;

  } else if (_tcscmp(misc, _T("+0.5")) == 0) {
    mc = std::min(mc + Units::ToSysVSpeed(GetUserVerticalSpeedStep() * 5),
                  fixed(5));
    ActionInterface::SetManualMacCready(mc);
    return true;

  } else if (_tcscmp(misc, _T("-0.1")) == 0) {
    mc = std::max(mc - Units::ToSysVSpeed(GetUserVerticalSpeedStep()),
                  fixed_zero);
    ActionInterface::SetManualMacCready(mc);
    return true;

  } else if (_tcscmp(misc, _T("-0.5")) == 0) {
    mc = std::max(mc - Units::ToSysVSpeed(GetUserVerticalSpeedStep() * 5),
                  fixed_zero);
    ActionInterface::SetManualMacCready(mc);
    return true;

  } else if (_tcscmp(misc, _T("mode")) == 0) {
    return HandleKey(ibkEnter);
  }

  return false;
}
