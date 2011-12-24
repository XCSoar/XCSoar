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

#include "TaskRulesConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"


class TaskRulesConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

void
TaskRulesConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
TaskRulesConfigPanel::Hide()
{
  XMLWidget::Hide();
}

void
TaskRulesConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent,
             Layout::landscape ? _T("IDR_XML_TASKRULESCONFIGPANEL_L") :
                               _T("IDR_XML_TASKRULESCONFIGPANEL"));

  const ComputerSettings &settings_computer = XCSoarInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings_computer.task;

  LoadFormProperty(form, _T("prpStartMaxSpeed"), ugHorizontalSpeed,
                   task_behaviour.ordered_defaults.start_max_speed);

  LoadFormProperty(form, _T("prpStartMaxSpeedMargin"), ugHorizontalSpeed,
                   task_behaviour.start_max_speed_margin);

  static gcc_constexpr_data StaticEnumChoice start_max_height_ref_list[] = {
    { hrAGL, N_("AGL"), N_("Reference AGL for start maximum height rule (above start point)") },
    { hrMSL, N_("MSL"), N_("Reference MSL for start maximum height rule (above sea level)") },
    { 0 }
  };
  LoadFormProperty(form, _T("prpStartHeightRef"), start_max_height_ref_list,
                   task_behaviour.ordered_defaults.start_max_height_ref);

  LoadFormProperty(form, _T("prpStartMaxHeight"), ugAltitude,
                   task_behaviour.ordered_defaults.start_max_height);

  LoadFormProperty(form, _T("prpStartMaxHeightMargin"), ugAltitude,
                   task_behaviour.start_max_height_margin);

  static gcc_constexpr_data StaticEnumChoice finish_min_height_ref_list[] = {
    { hrAGL, N_("AGL"), N_("Reference AGL for finish minimum height rule (above finish point)") },
    { hrMSL, N_("MSL"), N_("Reference MSL for finish minimum height rule (above sea level)") },
    { 0 }
  };
  LoadFormProperty(form, _T("prpFinishHeightRef"), finish_min_height_ref_list,
                   task_behaviour.ordered_defaults.finish_min_height_ref);

  LoadFormProperty(form, _T("prpFinishMinHeight"), ugAltitude,
                   task_behaviour.ordered_defaults.finish_min_height);

  const StaticEnumChoice contests_list[] = {
    { OLC_FAI, ContestToString(OLC_FAI) },
    { OLC_Classic, ContestToString(OLC_Classic) },
    { OLC_League, ContestToString(OLC_League) },
    { OLC_Plus, ContestToString(OLC_Plus) },
    { OLC_XContest, ContestToString(OLC_XContest) },
    { OLC_DHVXC, ContestToString(OLC_DHVXC) },
    { OLC_SISAT, ContestToString(OLC_SISAT) },
    { 0 }
  };
  LoadFormProperty(form, _T("prpContests"), contests_list,
                   task_behaviour.contest);
}


bool
TaskRulesConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;

  OrderedTaskBehaviour &otb = task_behaviour.ordered_defaults;
  changed |= SaveFormProperty(form, _T("prpStartMaxSpeed"),
                              ugHorizontalSpeed,
                              otb.start_max_speed,
                              szProfileStartMaxSpeed);

  changed |= SaveFormProperty(form, _T("prpStartMaxSpeedMargin"),
                              ugHorizontalSpeed,
                              task_behaviour.start_max_speed_margin,
                              szProfileStartMaxSpeedMargin);

  changed |= SaveFormProperty(form, _T("prpStartMaxHeight"), ugAltitude,
                              otb.start_max_height,
                              szProfileStartMaxHeight);

  changed |= SaveFormProperty(form, _T("prpStartMaxHeightMargin"), ugAltitude,
                              task_behaviour.start_max_height_margin,
                              szProfileStartMaxHeightMargin);

  changed |= SaveFormPropertyEnum(form, _T("prpStartHeightRef"),
                                  szProfileStartHeightRef,
                                  otb.start_max_height_ref);

  changed |= SaveFormProperty(form, _T("prpFinishMinHeight"), ugAltitude,
                              otb.finish_min_height,
                              szProfileFinishMinHeight);

  changed |= SaveFormPropertyEnum(form, _T("prpFinishHeightRef"),
                                  szProfileFinishHeightRef,
                                  otb.finish_min_height_ref);

  changed |= SaveFormPropertyEnum(form, _T("prpContests"), szProfileOLCRules,
                                  task_behaviour.contest);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateTaskRulesConfigPanel()
{
  return new TaskRulesConfigPanel();
}
