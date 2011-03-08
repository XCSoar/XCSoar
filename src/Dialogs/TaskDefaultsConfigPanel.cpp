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
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Components.hpp"
#include "TaskDefaultsConfigPanel.hpp"

static WndForm* wf = NULL;


void
TaskDefaultsConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  WndProperty *wp;
  const SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SettingsComputer();
  OrderedTask* temptask = protected_task_manager->task_blank();
  temptask->set_factory(TaskBehaviour::FACTORY_RT);


  wp = (WndProperty*)wf->FindByName(_T("prpStartType"));
  if (wp) {
    const AbstractTaskFactory::LegalPointVector point_types =
        temptask->get_factory().getValidStartTypes();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (unsigned i = 0; i < point_types.size(); i++) {
      dfe->addEnumText(OrderedTaskPointName(point_types[i]), (unsigned)point_types[i],
          OrderedTaskPointDescription(point_types[i]));
      if (point_types[i] == settings_computer.sector_defaults.start_type)
        dfe->Set((unsigned)point_types[i]);
    }
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpStartRadius"), ugDistance,
                   settings_computer.sector_defaults.start_radius);

  wp = (WndProperty*)wf->FindByName(_T("prpFinishType"));
  if (wp) {
    const AbstractTaskFactory::LegalPointVector point_types =
        temptask->get_factory().getValidFinishTypes();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (unsigned i = 0; i < point_types.size(); i++) {
      dfe->addEnumText(OrderedTaskPointName(point_types[i]), (unsigned)point_types[i],
          OrderedTaskPointDescription(point_types[i]));
      if (point_types[i] == settings_computer.sector_defaults.finish_type)
        dfe->Set((unsigned)point_types[i]);
    }
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpFinishRadius"), ugDistance,
                   settings_computer.sector_defaults.finish_radius);

  wp = (WndProperty*)wf->FindByName(_T("prpTurnpointType"));
  if (wp) {
    const AbstractTaskFactory::LegalPointVector point_types =
        temptask->get_factory().getValidIntermediateTypes();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (unsigned i = 0; i < point_types.size(); i++) {
      dfe->addEnumText(OrderedTaskPointName(point_types[i]),
          (unsigned)point_types[i],
          OrderedTaskPointDescription(point_types[i]));
      if (point_types[i] == settings_computer.sector_defaults.turnpoint_type) {
        dfe->Set((unsigned)point_types[i]);
      }
    }
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpTurnpointRadius"), ugDistance,
                   settings_computer.sector_defaults.turnpoint_radius);

  wp = (WndProperty*)wf->FindByName(_T("prpTaskType"));
  if (wp) {
    const std::vector<TaskBehaviour::Factory_t> factory_types =
        temptask->get_factory_types();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (unsigned i = 0; i < factory_types.size(); i++) {
      dfe->addEnumText(OrderedTaskFactoryName(factory_types[i]),
          (unsigned)factory_types[i], OrderedTaskFactoryDescription(
              factory_types[i]));
      if (factory_types[i] == settings_computer.task_type_default)
        dfe->Set((unsigned)factory_types[i]);
    }
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpAATMinTime"),
                   (unsigned)(settings_computer.ordered_defaults.aat_min_time / 60));

  delete temptask;
}


bool
TaskDefaultsConfigPanel::Save()
{
  bool changed = false;
  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();
  WndProperty *wp;

  unsigned sdtemp = 0;
  sdtemp = (unsigned)settings_computer.sector_defaults.start_type;
  changed |= SaveFormProperty(*wf, _T("prpStartType"),
                              szProfileStartType,
                              sdtemp);
  settings_computer.sector_defaults.start_type =
      (AbstractTaskFactory::LegalPointType_t)sdtemp;

  changed |= SaveFormProperty(*wf, _T("prpStartRadius"),
                              ugDistance,
                              settings_computer.sector_defaults.start_radius,
                              szProfileStartRadius);

  sdtemp = (unsigned)settings_computer.sector_defaults.turnpoint_type;
  changed |= SaveFormProperty(*wf, _T("prpTurnpointType"),
                              szProfileTurnpointType,
                              sdtemp);
  settings_computer.sector_defaults.turnpoint_type =
      (AbstractTaskFactory::LegalPointType_t)sdtemp;

  changed |= SaveFormProperty(*wf, _T("prpTurnpointRadius"),
                              ugDistance,
                              settings_computer.sector_defaults.turnpoint_radius,
                              szProfileTurnpointRadius);

  sdtemp = (unsigned)settings_computer.sector_defaults.finish_type;
  changed |= SaveFormProperty(*wf, _T("prpFinishType"),
                              szProfileFinishType,
                              sdtemp);
  settings_computer.sector_defaults.finish_type =
      (AbstractTaskFactory::LegalPointType_t)sdtemp;

  changed |= SaveFormProperty(*wf, _T("prpFinishRadius"),
                              ugDistance,
                              settings_computer.sector_defaults.finish_radius,
                              szProfileFinishRadius);

  sdtemp = (unsigned)settings_computer.task_type_default;
  changed |= SaveFormProperty(*wf, _T("prpTaskType"),
                              szProfileTaskType,
                              sdtemp);
  settings_computer.task_type_default =
      (TaskBehaviour::Factory_t)sdtemp;

  unsigned aatminutes = unsigned(settings_computer.ordered_defaults.aat_min_time) / 60;
  wp = (WndProperty*)wf->FindByName(_T("prpAATMinTime"));
  if (aatminutes != (unsigned)wp->GetDataField()->GetAsInteger()) {
    aatminutes = wp->GetDataField()->GetAsInteger();
    settings_computer.ordered_defaults.aat_min_time = fixed(aatminutes * 60);
    Profile::Set(szProfileAATMinTime, aatminutes * 60);
    changed = true;
  }

  return changed;
}
