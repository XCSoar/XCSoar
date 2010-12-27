/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "dlgTaskHelpers.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Units.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Task/Visitors/ObservationZoneVisitor.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/ObservationZones/SectorZone.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Components.hpp"
#include "LocalPath.hpp"

#include <assert.h>
#include <stdio.h>

const TCHAR*
OrderedTaskFactoryName(OrderedTask::Factory_t type)
{
  switch(type) {
  case OrderedTask::FACTORY_RT:
    return _("RT");
  case OrderedTask::FACTORY_FAI_GENERAL:
    return _("FAI generic");
  case OrderedTask::FACTORY_FAI_TRIANGLE:
    return _("FAI triangle");
  case OrderedTask::FACTORY_FAI_OR:
    return _("FAI out and return");
  case OrderedTask::FACTORY_FAI_GOAL:
    return _("FAI goal");
  case OrderedTask::FACTORY_AAT:
    return _("AAT");
  case OrderedTask::FACTORY_MIXED:
    return _("Mixed");
  case OrderedTask::FACTORY_TOURING:
    return _("Touring");
  default:
    assert(1);
  }
  return NULL;
}

const TCHAR*
OrderedTaskFactoryDescription(OrderedTask::Factory_t type)
{
  switch(type) {
  case OrderedTask::FACTORY_RT:
    return _("Racing task around turnpoints");
  case OrderedTask::FACTORY_FAI_GENERAL:
    return _("FAI rules, path around 3 or more turnpoints and return");
  case OrderedTask::FACTORY_FAI_TRIANGLE:
    return _("FAI rules, path from a start to two turnpoints and return");
  case OrderedTask::FACTORY_FAI_OR:
    return _("FAI rules, path from start to a single turnpoint and return");
  case OrderedTask::FACTORY_FAI_GOAL:
    return _("FAI rules, path from start to a goal destination");
  case OrderedTask::FACTORY_AAT:
    return _("Racing task through assigned areas, minimum task time applies");
  case OrderedTask::FACTORY_MIXED:
    return _("Racing task with a mix of assigned areas and turnpoints, minimum task time applies");
  case OrderedTask::FACTORY_TOURING:
    return _("Casual touring task, uses start and finish cylinders and FAI sector turnpoints");
  default:
    assert(1);
  }
  return NULL;
}

void
OrderedTaskSummary(OrderedTask* task, TCHAR* text)
{
  const TaskStats &stats = task->get_stats();
  TCHAR invalid [15];
  if (task->check_task())
    invalid[0] = '\0';
  else
    _tcscpy(invalid,_(" (invalid)"));

  if (!task->task_size()) {
    _stprintf(text, _("%s\nTask is empty"),
              OrderedTaskFactoryName(task->get_factory_type()));
  } else {
    if (task->has_targets())
      _stprintf(text, _("%s%s\nNominal dist: %.0f %s\nMax dist: %.0f %s\nMin dist: %.0f %s"),
                OrderedTaskFactoryName(task->get_factory_type()),
                invalid,
                (double)Units::ToUserDistance(stats.distance_nominal),
                Units::GetDistanceName(),
                (double)Units::ToUserDistance(stats.distance_max),
                Units::GetDistanceName(),
                (double)Units::ToUserDistance(stats.distance_min),
                Units::GetDistanceName());
    else
      _stprintf(text, _("%s%s\nDistance: %.0f %s"),
                OrderedTaskFactoryName(task->get_factory_type()),
                invalid,
                (double)Units::ToUserDistance(stats.distance_nominal),
                Units::GetDistanceName());
  }
}

class LabelObservationZone
{
public:
  LabelObservationZone(TCHAR* buff): text(buff) {}

  void
  Visit(const FAISectorZone& oz)
  {
    _stprintf(text, _("FAI Sector"));
  }

  void
  Visit(const KeyholeZone& oz)
  {
    _stprintf(text, _("Keyhole"));
  }

  void
  Visit(const BGAFixedCourseZone& oz)
  {
    _stprintf(text, _("BGAFixedCourse"));
  }

  void
  Visit(const BGAEnhancedOptionZone& oz)
  {
    _stprintf(text, _("BGAEnhancedOption"));
  }

  void
  Visit(const BGAStartSectorZone& oz)
  {
    _stprintf(text, _("BGA start sector"));
  }

  void
  Visit(const SectorZone& oz)
  {
    _stprintf(text, _("Sector"));
  }

  void
  Visit(const LineSectorZone& oz)
  {
    _stprintf(text, _("Line"));
  }

  void
  Visit(const CylinderZone& oz)
  {
    _stprintf(text, _("Cylinder"));
  }

private:
  TCHAR *text;
};

class LabelSizeObservationZone:
  public ObservationZoneConstVisitor
{
public:
  LabelSizeObservationZone(TCHAR* _radius):
    radius(_radius)
  {
    radius[0] = _T('\0');
  }

  void
  Visit(const FAISectorZone& oz)
  {
    _stprintf(radius,_("FAI"));
  }
  void
  Visit(const KeyholeZone& oz)
  {
    _stprintf(radius,_("DAe"));
  }

  void
  Visit(const BGAFixedCourseZone& oz)
  {
    _stprintf(radius,_("BGA"));
  }

  void
  Visit(const BGAEnhancedOptionZone& oz)
  {
    _stprintf(radius,_("BGAE"));
  }

  void
  Visit(const BGAStartSectorZone& oz)
  {
    _stprintf(radius,_("BGA-S"));
  }

  void
  Visit(const SectorZone& oz)
  {
    _stprintf(radius,_T("%.1f%s"), (double)Units::ToUserDistance(oz.getRadius()),Units::GetUnitName(Units::DistanceUnit));
  }

  void
  Visit(const LineSectorZone& oz)
  {
    _stprintf(radius,_T("%.1f%s"), (double)Units::ToUserDistance(oz.getLength()),Units::GetUnitName(Units::DistanceUnit));
  }

  void
  Visit(const CylinderZone& oz)
  {
    _stprintf(radius,_T("%.1f%s"), (double)Units::ToUserDistance(oz.getRadius()),Units::GetUnitName(Units::DistanceUnit));
  }
  TCHAR* radius;
};

class LabelTaskPoint:
  public TaskPointConstVisitor
{
public:
    LabelTaskPoint(const unsigned index, TCHAR* _name, TCHAR* _radius):
    m_index(0),
    m_active_index(index),
    name(_name),
    ozSize(_radius){
    name[0] = _T('\0');
  }

  void Visit(const UnorderedTaskPoint& tp) {}

  void
  Visit(const StartPoint& tp)
  {
    if (found()) {
      _stprintf(name, _T("S:  %s"), tp.get_waypoint().Name.c_str());
      const ObservationZonePoint *ozp = tp.get_oz();
      ((ObservationZoneConstVisitor &)ozSize).Visit(*ozp);
    }

    inc_index();
  }
  void
  Visit(const FinishPoint& tp)
  {
    if (found()) {
      _stprintf(name, _T("F:  %s"), tp.get_waypoint().Name.c_str());
      const ObservationZonePoint *ozp = tp.get_oz();
      ((ObservationZoneConstVisitor &)ozSize).Visit(*ozp);
    }

    inc_index();
  }
  void
  Visit(const AATPoint& tp)
  {
    if (found()) {
      _stprintf(name, _T("A%d: %s"), m_index, tp.get_waypoint().Name.c_str());
      const ObservationZonePoint *ozp = tp.get_oz();
      ((ObservationZoneConstVisitor &)ozSize).Visit(*ozp);
    }

    inc_index();
  }
  void
  Visit(const ASTPoint& tp)
  {
    if (found()) {
      _stprintf(name, _T("T%d: %s"), m_index, tp.get_waypoint().Name.c_str());
      const ObservationZonePoint *ozp = tp.get_oz();
      ((ObservationZoneConstVisitor &)ozSize).Visit(*ozp);
    }
    inc_index();
  }

private:
  bool found() {
    return (m_index == m_active_index);
  }

  void inc_index() {
    m_index++;
  }

  unsigned m_index;
  const unsigned m_active_index;
  TCHAR* name;
  LabelSizeObservationZone ozSize;
};

void
OrderedTaskPointLabel(OrderedTask* task, const unsigned index, TCHAR* name, TCHAR* radius)
{
  LabelTaskPoint tpv(index, name, radius);
  task->tp_CAccept(tpv);
}

const TCHAR*
OrderedTaskPointDescription(AbstractTaskFactory::LegalPointType_t type)
{
  switch (type) {
  case AbstractTaskFactory::START_SECTOR:
    return _("Cross corner edge from inside area to start");
  case AbstractTaskFactory::START_LINE:
    return _("Cross line from inside area to start");
  case AbstractTaskFactory::START_CYLINDER:
    return _("Exit area to start");
  case AbstractTaskFactory::START_BGA:
    return _("Exit area in any direction to start");
  case AbstractTaskFactory::FAI_SECTOR:
    return _("Any point within area scored from corner point");
  case AbstractTaskFactory::AST_CYLINDER:
    return _("Any point within area scored from center");
  case AbstractTaskFactory::KEYHOLE_SECTOR:
    return _("(As used in German rules) Any point within area scored from center");
  case AbstractTaskFactory::BGAFIXEDCOURSE_SECTOR:
    return _("Any point within area scored from center");
  case AbstractTaskFactory::BGAENHANCEDOPTION_SECTOR:
    return _("Any point within area scored from center");
  case AbstractTaskFactory::AAT_CYLINDER:
    return _("Scored inside area");
  case AbstractTaskFactory::AAT_SEGMENT:
    return _("Scored inside area");
  case AbstractTaskFactory::FINISH_SECTOR:
    return _("Cross corner edge to finish");
  case AbstractTaskFactory::FINISH_LINE:
    return _("Cross line into area to finish");
  case AbstractTaskFactory::FINISH_CYLINDER:
    return _("Enter area to finish");
  default:
    assert(1);
  }
  return NULL;
}

const TCHAR*
OrderedTaskPointName(AbstractTaskFactory::LegalPointType_t type)
{
  switch (type) {
  case AbstractTaskFactory::START_SECTOR:
    return _("Start sector");
  case AbstractTaskFactory::START_LINE:
    return _("Start line");
  case AbstractTaskFactory::START_CYLINDER:
    return _("Start cylinder");
  case AbstractTaskFactory::START_BGA:
    return _("BGA start sector");
  case AbstractTaskFactory::FAI_SECTOR:
    return _("Turnpoint FAI sector");
  case AbstractTaskFactory::KEYHOLE_SECTOR:
    return _("Keyhole sector (DAe)");
  case AbstractTaskFactory::BGAFIXEDCOURSE_SECTOR:
    return _("BGA Fixed Course sector");
  case AbstractTaskFactory::BGAENHANCEDOPTION_SECTOR:
    return _("BGA Enhanced Option Fixed Course sector");
  case AbstractTaskFactory::AST_CYLINDER:
    return _("Turnpoint cylinder");
  case AbstractTaskFactory::AAT_CYLINDER:
    return _("Area cylinder");
  case AbstractTaskFactory::AAT_SEGMENT:
    return _("Area sector");
  case AbstractTaskFactory::FINISH_SECTOR:
    return _("Finish sector");
  case AbstractTaskFactory::FINISH_LINE:
    return _("Finish line");
  case AbstractTaskFactory::FINISH_CYLINDER:
    return _("Finish cylinder");
  default:
    assert(1);
  }
  return NULL;
}

bool
OrderedTaskSave(const OrderedTask& task, bool noask)
{
  assert(protected_task_manager != NULL);

  if (!noask
      && MessageBoxX(_("Save task?"), _("Task Selection"),
                     MB_YESNO | MB_ICONQUESTION) != IDYES)
    return false;

  TCHAR fname[69] = _T("");
  if (!dlgTextEntryShowModal(fname, 64))
    return false;

  TCHAR path[MAX_PATH];
  _tcscat(fname, _T(".tsk"));
  LocalPath(path, fname);
  protected_task_manager->task_save(path, task);
  return true;
}

const TCHAR*
getTaskValidationErrors(const AbstractTaskFactory::TaskValidationErrorVector v)
{

  static TCHAR err[MAX_PATH];
  err[0] = '\0';

  for (unsigned i = 0; i < v.size(); i++) {

    if ((_tcslen(err) + _tcslen(TaskValidationError(v[i]))) < MAX_PATH) {
      _tcscat(err, TaskValidationError(v[i]));
    }
  }
  return err;
  //  _tcscat(err, OrderedTaskFactory_OneValidationError(v[i]);
}
const TCHAR*
TaskValidationError(AbstractTaskFactory::TaskValidationErrorType_t type)
{
  switch (type) {
  case AbstractTaskFactory::NO_VALID_START:
    return _("No valid start.\n");
    break;
  case AbstractTaskFactory::NO_VALID_FINISH:
    return _("No valid finish.\n");
    break;
  case AbstractTaskFactory::TASK_NOT_CLOSED:
    return _("Task not closed.\n");
    break;
  case AbstractTaskFactory::TASK_NOT_HOMOGENEOUS:
    return _("All turnpoints not the same type.\n");
    break;
  case AbstractTaskFactory::INCORRECT_NUMBER_TURNPOINTS:
    return _("Incorrect number of turnpoints.\n");
    break;
  case AbstractTaskFactory::EXCEEDS_MAX_TURNPOINTS:
    return _("Too many turnpoints.\n");
    break;
  case AbstractTaskFactory::UNDER_MIN_TURNPOINTS:
    return _("Not enough turnpoints.\n");
    break;
  case AbstractTaskFactory::TURNPOINTS_NOT_UNIQUE:
    return _("Turnpoints not unique.\n");
    break;
  case AbstractTaskFactory::INVALID_FAI_TRIANGLE_GEOMETRY:
    return _("Invalid FAI triangle shape.\n");
    break;
  case AbstractTaskFactory::EMPTY_TASK:
    return _("Empty task.\n");
    break;
  }

  return _T("");
}
