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

#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Components.hpp"
#include "Form/RowFormWidget.hpp"
#include "TaskDefaultsConfigPanel.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  StartType,
  StartRadius,
  spacer_1,
  FinishType,
  FinishRadius,
  spacer_2,
  TurnpointType,
  TurnpointRadius,
  spacer_3,
  TaskType,
  AATMinTime,
  AATTimeMargin
};

class TaskDefaultsConfigPanel : public RowFormWidget {
public:
  TaskDefaultsConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(125)) {}

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  void SetStartLabel();
  void SetFinishLabel();
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static TaskDefaultsConfigPanel *instance;

static void
OnStartType(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  instance->SetStartLabel();
}

static void
OnFinishType(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  instance->SetFinishLabel();
}

static const TCHAR *Caption_GateWidth = N_("Gate width");
static const TCHAR *Caption_Radius = N_("Radius");
void
TaskDefaultsConfigPanel::SetStartLabel()
{
  WndProperty &wp = GetControl(StartRadius);

  if (GetValueInteger(StartType) == AbstractTaskFactory::START_LINE)
    wp.SetCaption(gettext(Caption_GateWidth));
  else
    wp.SetCaption(gettext(Caption_Radius));
}

void
TaskDefaultsConfigPanel::SetFinishLabel()
{
  WndProperty &wp = GetControl(FinishRadius);

  if (GetValueInteger(FinishType) == AbstractTaskFactory::FINISH_LINE)
    wp.SetCaption(gettext(Caption_GateWidth));
  else
    wp.SetCaption(gettext(Caption_Radius));
}


void
TaskDefaultsConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  WndProperty *wp;
  const ComputerSettings &settings_computer = XCSoarInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings_computer.task;
  OrderedTask* temptask = protected_task_manager->TaskBlank();
  temptask->SetFactory(TaskBehaviour::FACTORY_RT);

  instance = this;
  RowFormWidget::Prepare(parent, rc);

  wp = AddEnum(_("Start point"), _("Default start type for new tasks you create."), OnStartType);
  if (wp) {
    const auto point_types = temptask->GetFactory().getValidStartTypes();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (auto i = point_types.begin(), end = point_types.end();
         i != end; ++i) {
      const AbstractTaskFactory::LegalPointType_t type = *i;
      dfe->addEnumText(OrderedTaskPointName(type), (unsigned)type,
                       OrderedTaskPointDescription(type));
      if (type == task_behaviour.sector_defaults.start_type)
        dfe->Set((unsigned)type);
    }
    wp->RefreshDisplay();
  }

  AddFloat(Caption_GateWidth, _("Default radius or gate width of the start zone for new tasks."),
           _T("%.1f %s"), _T("%.1f"), fixed(0.1), fixed(100), fixed(0.5), true, ugDistance,
           task_behaviour.sector_defaults.start_radius);

  AddSpacer();

  wp = AddEnum(_("Finish point"), _("Default finish type for new tasks you create."), OnFinishType);
  if (wp) {
    const auto point_types = temptask->GetFactory().getValidFinishTypes();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (auto i = point_types.begin(), end = point_types.end();
         i != end; ++i) {
      const AbstractTaskFactory::LegalPointType_t type = *i;
      dfe->addEnumText(OrderedTaskPointName(type), (unsigned)type,
                       OrderedTaskPointDescription(type));
      if (type == task_behaviour.sector_defaults.finish_type)
        dfe->Set((unsigned)type);
    }
    wp->RefreshDisplay();
  }

  AddFloat(Caption_GateWidth, _("Default radius or gate width of the finish zone in new tasks."),
           _T("%.1f %s"), _T("%.1f"), fixed(0.1), fixed(100), fixed(0.5), true, ugDistance,
           task_behaviour.sector_defaults.finish_radius);

  AddSpacer();

  wp = AddEnum(_("Turn point"), _("Default turn point type for new tasks you create."));
  if (wp) {
    const auto point_types = temptask->GetFactory().getValidIntermediateTypes();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (auto i = point_types.begin(), end = point_types.end();
         i != end; ++i) {
      const AbstractTaskFactory::LegalPointType_t type = *i;
      dfe->addEnumText(OrderedTaskPointName(type), (unsigned)type,
                       OrderedTaskPointDescription(type));
      if (type == task_behaviour.sector_defaults.turnpoint_type) {
        dfe->Set((unsigned)type);
      }
    }
    wp->RefreshDisplay();
  }

  AddFloat(Caption_Radius, _("Default radius of turnpoint cylinders and sectors in new tasks."),
           _T("%.1f %s"), _T("%.1f"), fixed(0.1), fixed(100), fixed(0.5), true, ugDistance,
           task_behaviour.sector_defaults.turnpoint_radius);

  AddSpacer();

  wp = AddEnum(_("Task"), _("Default task type for new tasks you create."));
  if (wp) {
    const std::vector<TaskBehaviour::FactoryType> factory_types =
        temptask->GetFactoryTypes();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (unsigned i = 0; i < factory_types.size(); i++) {
      dfe->addEnumText(OrderedTaskFactoryName(factory_types[i]),
          (unsigned)factory_types[i], OrderedTaskFactoryDescription(
              factory_types[i]));
      if (factory_types[i] == task_behaviour.task_type_default)
        dfe->Set((unsigned)factory_types[i]);
    }
    wp->RefreshDisplay();
  }

  AddInteger(_("AAT min. time"), _("Default AAT min. time for new AAT tasks."),
             _T("%u min"), _T("%u"), 1, 500, 1,
             (unsigned)(task_behaviour.ordered_defaults.aat_min_time / 60));

  AddInteger(_("Optimisation margin"),
             _("Safety margin for AAT task optimisation.  Optimisation "
                 "seeks to complete the task at the minimum time plus this margin time."),
             _T("%u min"), _T("%u"), 0, 30, 1,
             (unsigned)(task_behaviour.optimise_targets_margin / 60));

  SetStartLabel();
  SetFinishLabel();

  delete temptask;
}

bool
TaskDefaultsConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;

  changed |= SaveValueEnum(StartType, szProfileStartType, task_behaviour.sector_defaults.start_type);

  changed |= SaveValue(StartRadius, ugDistance, szProfileStartRadius,
                       task_behaviour.sector_defaults.start_radius);

  changed |= SaveValueEnum(TurnpointType, szProfileTurnpointType,
                           task_behaviour.sector_defaults.turnpoint_type);

  changed |= SaveValue(TurnpointRadius, ugDistance, szProfileTurnpointRadius,
                       task_behaviour.sector_defaults.turnpoint_radius);

  changed |= SaveValueEnum(FinishType, szProfileFinishType,
                           task_behaviour.sector_defaults.finish_type);

  changed |= SaveValue(FinishRadius, ugDistance, szProfileFinishRadius,
                       task_behaviour.sector_defaults.finish_radius);

  changed |= SaveValueEnum(TaskType, szProfileTaskType, task_behaviour.task_type_default);

  unsigned aatminutes = unsigned(task_behaviour.ordered_defaults.aat_min_time) / 60;
  if (SaveValue(AATMinTime, aatminutes)) {
    task_behaviour.ordered_defaults.aat_min_time = fixed(aatminutes * 60);
    Profile::Set(szProfileAATMinTime, aatminutes * 60);
    changed = true;
  }

  unsigned aatmargin = task_behaviour.optimise_targets_margin / 60;
  if (SaveValue(AATTimeMargin, aatmargin)) {
    task_behaviour.optimise_targets_margin = aatmargin * 60;
    Profile::Set(szProfileAATTimeMargin, aatmargin * 60);
    changed = true;
  }

  _changed |= changed;
  _require_restart |= require_restart;
  return true;
}

Widget *
CreateTaskDefaultsConfigPanel()
{
  return new TaskDefaultsConfigPanel();
}
