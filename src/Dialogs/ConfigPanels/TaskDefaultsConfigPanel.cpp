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
#include "Task/ProtectedTaskManager.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Components.hpp"
#include "Form/XMLWidget.hpp"
#include "TaskDefaultsConfigPanel.hpp"
#include "Screen/Layout.hpp"

class TaskDefaultsConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
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

void
TaskDefaultsConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}


void
TaskDefaultsConfigPanel::Hide()
{
  XMLWidget::Hide();
}


void
TaskDefaultsConfigPanel::SetStartLabel()
{
  WndProperty *wp = NULL;
  wp = (WndProperty*)form.FindByName(_T("prpStartRadius"));
  assert(wp);
  if (GetFormValueInteger(form, _T("prpStartType")) == AbstractTaskFactory::START_LINE)
    wp->SetCaption(_T("Gate Width"));
  else
    wp->SetCaption(_T("Radius"));
}

void
TaskDefaultsConfigPanel::SetFinishLabel()
{
  WndProperty *wp = NULL;

  wp = (WndProperty*)form.FindByName(_T("prpFinishRadius"));
  assert(wp);
  if (GetFormValueInteger(form, _T("prpFinishType")) == AbstractTaskFactory::FINISH_LINE)
    wp->SetCaption(_T("Gate Width"));
  else
    wp->SetCaption(_T("Radius"));
}


static gcc_constexpr_data CallBackTableEntry task_defaults_config_panel_callbacks[] = {
  DeclareCallBackEntry(OnStartType),
  DeclareCallBackEntry(OnFinishType),

  DeclareCallBackEntry(NULL)
};

void
TaskDefaultsConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{

  instance = this;
  LoadWindow(task_defaults_config_panel_callbacks, parent,
             Layout::landscape ? _T("IDR_XML_TASKDEFAULTSCONFIGPANEL_L") :
                               _T("IDR_XML_TASKDEFAULTSCONFIGPANEL"));

  WndProperty *wp;
  const ComputerSettings &settings_computer = XCSoarInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings_computer.task;
  OrderedTask* temptask = protected_task_manager->TaskBlank();
  temptask->SetFactory(TaskBehaviour::FACTORY_RT);

  wp = (WndProperty*)form.FindByName(_T("prpStartType"));
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

  LoadFormProperty(form, _T("prpStartRadius"), ugDistance,
                   task_behaviour.sector_defaults.start_radius);

  wp = (WndProperty*)form.FindByName(_T("prpFinishType"));
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

  LoadFormProperty(form, _T("prpFinishRadius"), ugDistance,
                   task_behaviour.sector_defaults.finish_radius);

  wp = (WndProperty*)form.FindByName(_T("prpTurnpointType"));
  if (wp) {
    const auto point_types =
      temptask->GetFactory().getValidIntermediateTypes();
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

  LoadFormProperty(form, _T("prpTurnpointRadius"), ugDistance,
                   task_behaviour.sector_defaults.turnpoint_radius);

  wp = (WndProperty*)form.FindByName(_T("prpTaskType"));
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

  LoadFormProperty(form, _T("prpAATMinTime"),
                   (unsigned)(task_behaviour.ordered_defaults.aat_min_time / 60));

  LoadFormProperty(form, _T("prpAATTimeMargin"),
                   (unsigned)(task_behaviour.optimise_targets_margin / 60));

  instance->SetStartLabel();
  instance->SetFinishLabel();

  delete temptask;
}

bool
TaskDefaultsConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;;

  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;
  WndProperty *wp;

  changed |= SaveFormPropertyEnum(form, _T("prpStartType"),
                                  szProfileStartType,
                                  task_behaviour.sector_defaults.start_type);

  changed |= SaveFormProperty(form, _T("prpStartRadius"),
                              ugDistance,
                              task_behaviour.sector_defaults.start_radius,
                              szProfileStartRadius);

  changed |= SaveFormPropertyEnum(form, _T("prpTurnpointType"),
                                  szProfileTurnpointType,
                                  task_behaviour.sector_defaults.turnpoint_type);

  changed |= SaveFormProperty(form, _T("prpTurnpointRadius"),
                              ugDistance,
                              task_behaviour.sector_defaults.turnpoint_radius,
                              szProfileTurnpointRadius);

  changed |= SaveFormPropertyEnum(form, _T("prpFinishType"),
                                  szProfileFinishType,
                                  task_behaviour.sector_defaults.finish_type);

  changed |= SaveFormProperty(form, _T("prpFinishRadius"),
                              ugDistance,
                              task_behaviour.sector_defaults.finish_radius,
                              szProfileFinishRadius);

  changed |= SaveFormPropertyEnum(form, _T("prpTaskType"),
                                  szProfileTaskType,
                                  task_behaviour.task_type_default);

  unsigned aatminutes = unsigned(task_behaviour.ordered_defaults.aat_min_time) / 60;
  wp = (WndProperty*)form.FindByName(_T("prpAATMinTime"));
  if (aatminutes != (unsigned)wp->GetDataField()->GetAsInteger()) {
    aatminutes = wp->GetDataField()->GetAsInteger();
    task_behaviour.ordered_defaults.aat_min_time = fixed(aatminutes * 60);
    Profile::Set(szProfileAATMinTime, aatminutes * 60);
    changed = true;
  }

  unsigned aatmargin = task_behaviour.optimise_targets_margin/60;
  wp = (WndProperty*)form.FindByName(_T("prpAATTimeMargin"));
  if (aatmargin != (unsigned)wp->GetDataField()->GetAsInteger()) {
    aatmargin = wp->GetDataField()->GetAsInteger();
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
