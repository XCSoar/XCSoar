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
    return _T("RT");
  case OrderedTask::FACTORY_FAI_GENERAL:
    return _T("FAI generic");
  case OrderedTask::FACTORY_FAI_TRIANGLE:
    return _T("FAI triangle");
  case OrderedTask::FACTORY_FAI_OR:
    return _T("FAI out and return");
  case OrderedTask::FACTORY_FAI_GOAL:
    return _T("FAI goal");
  case OrderedTask::FACTORY_AAT:
    return _T("AAT");
  case OrderedTask::FACTORY_MIXED:
    return _T("Mixed AAT");
  case OrderedTask::FACTORY_TOURING:
    return _T("Touring");
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
    return _T("Racing task around turnpoints");
  case OrderedTask::FACTORY_FAI_GENERAL:
    return _T("FAI rules, path around 3 or more turnpoints and return");
  case OrderedTask::FACTORY_FAI_TRIANGLE:
    return _T("FAI rules, path from a start to two turnpoints and return");
  case OrderedTask::FACTORY_FAI_OR:
    return _T("FAI rules, path from start to a single turnpoint and return");
  case OrderedTask::FACTORY_FAI_GOAL:
    return _T("FAI rules, path from start to a goal destination");
  case OrderedTask::FACTORY_AAT:
    return _T("Racing task through assigned areas, minimum task time applies");
  case OrderedTask::FACTORY_MIXED:
    return _T("Racing task with a mix of assigned areas and turnpoints, minimum task time applies");
  case OrderedTask::FACTORY_TOURING:
    return _T("Casual touring task");
  default:
    assert(1);
  }
  return NULL;
}

void
OrderedTaskSummary(OrderedTask* task, TCHAR* text)
{
  const TaskStats &stats = task->get_stats();

  if (!task->task_size()) {
    _stprintf(text, _T("%s\nTask is empty"),
              OrderedTaskFactoryName(task->get_factory_type()));
  } else {
    if (task->has_targets())
      _stprintf(text, _T("%s\nNominal dist: %.0f %s\nMax dist: %.0f %s\nMin dist: %.0f %s"), 
                OrderedTaskFactoryName(task->get_factory_type()),
                (double)Units::ToUserDistance(stats.distance_nominal),
                Units::GetDistanceName(),
                (double)Units::ToUserDistance(stats.distance_max),
                Units::GetDistanceName(),
                (double)Units::ToUserDistance(stats.distance_min),
                Units::GetDistanceName());
    else
      _stprintf(text, _T("%s\nDistance: %.0f %s"), 
                OrderedTaskFactoryName(task->get_factory_type()),
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
    _stprintf(text, _T("FAI Sector"));
  }

  void
  Visit(const KeyholeZone& oz)
  {
    _stprintf(text, _T("Keyhole"));
  }

  void
  Visit(const BGAFixedCourseZone& oz)
  {
    _stprintf(text, _T("BGAFixedCourse"));
  }

  void
  Visit(const BGAEnhancedOptionZone& oz)
  {
    _stprintf(text, _T("BGAEnhancedOption"));
  }

  void
  Visit(const SectorZone& oz)
  {
    _stprintf(text, _T("Sector"));
  }

  void
  Visit(const LineSectorZone& oz)
  {
    _stprintf(text, _T("Line"));
  }

  void
  Visit(const CylinderZone& oz)
  {
    _stprintf(text, _T("Cylinder"));
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
    _stprintf(radius,_T("FAI"));
  }
  void
  Visit(const KeyholeZone& oz)
  {
    _stprintf(radius,_T("DAe"));
  }

  void
  Visit(const BGAFixedCourseZone& oz)
  {
    _stprintf(radius,_T("BGA"));
  }

  void
  Visit(const BGAEnhancedOptionZone& oz)
  {
    _stprintf(radius,_T("BGAE"));
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
    return _T("Cross corner edge from inside area to start");
  case AbstractTaskFactory::START_LINE:
    return _T("Cross line from inside area to start");
  case AbstractTaskFactory::START_CYLINDER:
    return _T("Exit area to start");
  case AbstractTaskFactory::FAI_SECTOR:
    return _T("Any point within area scored from corner point");
  case AbstractTaskFactory::AST_CYLINDER:
    return _T("Any point within area scored from center");
  case AbstractTaskFactory::KEYHOLE_SECTOR:
    return _T("(As used in German rules) Any point within area scored from center");
  case AbstractTaskFactory::BGAFIXEDCOURSE_SECTOR:
    return _T("Any point within area scored from center");
  case AbstractTaskFactory::BGAENHANCEDOPTION_SECTOR:
    return _T("Any point within area scored from center");
  case AbstractTaskFactory::AAT_CYLINDER:
    return _T("Scored inside area");
  case AbstractTaskFactory::AAT_SEGMENT:
    return _T("Scored inside area");
  case AbstractTaskFactory::FINISH_SECTOR:
    return _T("Cross corner edge to finish");
  case AbstractTaskFactory::FINISH_LINE:
    return _T("Cross line into area to finish");
  case AbstractTaskFactory::FINISH_CYLINDER:
    return _T("Enter area to finish");
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
    return _T("Start sector");
  case AbstractTaskFactory::START_LINE:
    return _T("Start line");
  case AbstractTaskFactory::START_CYLINDER:
    return _T("Start cylinder");
  case AbstractTaskFactory::FAI_SECTOR:
    return _T("Turnpoint FAI sector");
  case AbstractTaskFactory::KEYHOLE_SECTOR:
    return _T("Keyhole sector (DAe)");
  case AbstractTaskFactory::BGAFIXEDCOURSE_SECTOR:
    return _T("Standard BGA Fixed Course sector");
  case AbstractTaskFactory::BGAENHANCEDOPTION_SECTOR:
    return _T("BGA Enhanced Option Fixed Course sector");
  case AbstractTaskFactory::AST_CYLINDER:
    return _T("Turnpoint cylinder");
  case AbstractTaskFactory::AAT_CYLINDER:
    return _T("Area cylinder");
  case AbstractTaskFactory::AAT_SEGMENT:
    return _T("Area sector");
  case AbstractTaskFactory::FINISH_SECTOR:
    return _T("Finish sector");
  case AbstractTaskFactory::FINISH_LINE:
    return _T("Finish line");
  case AbstractTaskFactory::FINISH_CYLINDER:
    return _T("Finish cylinder");
  default:
    assert(1);
  }
  return NULL;
}

bool
OrderedTaskSave(const OrderedTask& task, bool noask)
{
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
  protected_task_manager.task_save(path, task);
  return true;
}
