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

#include "dlgTaskHelpers.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Units/Units.hpp"
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
#include <windef.h> /* for MAX_PATH */

const TCHAR*
OrderedTaskFactoryName(TaskBehaviour::Factory_t type)
{
  switch(type) {
  case TaskBehaviour::FACTORY_RT:
    return _("Racing");
  case TaskBehaviour::FACTORY_FAI_GENERAL:
    return _("FAI badges/records");
  case TaskBehaviour::FACTORY_FAI_TRIANGLE:
    return _("FAI triangle");
  case TaskBehaviour::FACTORY_FAI_OR:
    return _("FAI out and return");
  case TaskBehaviour::FACTORY_FAI_GOAL:
    return _("FAI goal");
  case TaskBehaviour::FACTORY_AAT:
    return _("AAT");
  case TaskBehaviour::FACTORY_MIXED:
    return _("Mixed");
  case TaskBehaviour::FACTORY_TOURING:
    return _("Touring");
  default:
    assert(1);
  }
  return NULL;
}

const TCHAR*
OrderedTaskFactoryDescription(TaskBehaviour::Factory_t type)
{
  switch(type) {
  case TaskBehaviour::FACTORY_RT:
    return _("Racing task around turn points.  Can also be used for FAI badge and record tasks.  Allows all shapes of observation zones.");
  case TaskBehaviour::FACTORY_FAI_GENERAL:
    return _("FAI rules, allows only FAI start, finish and turn point types, for badges and records.  Enables FAI finish height for final glide calculation");
  case TaskBehaviour::FACTORY_FAI_TRIANGLE:
    return _("FAI rules, path from a start to two turn points and return");
  case TaskBehaviour::FACTORY_FAI_OR:
    return _("FAI rules, path from start to a single turn point and return");
  case TaskBehaviour::FACTORY_FAI_GOAL:
    return _("FAI rules, path from start to a goal destination");
  case TaskBehaviour::FACTORY_AAT:
    return _("Task through assigned areas, minimum task time applies.  Restricted to cylinder and sector observation zones.");
  case TaskBehaviour::FACTORY_MIXED:
    return _("Racing task with a mix of assigned areas and turn points, minimum task time applies");
  case TaskBehaviour::FACTORY_TOURING:
    return _("Casual touring task, uses start and finish cylinders and FAI sector turn points");
  default:
    assert(1);
  }
  return NULL;
}

/**
 *
 * @param task
 * @param text
 * @return True if FAI shape
 */
static bool
TaskSummaryShape(OrderedTask* task, TCHAR* text)
{
  bool FAIShape = false;
  switch (task->task_size()) {
  case 0:
    text[0] = '\0';
    break;

  case 1:
    _tcscpy(text, _("Unknown"));
    break;

  case 2:
    _tcscpy(text, _("Goal"));
    FAIShape = true;

    break;

  case 3:
    if (task->get_factory().is_closed()) {
      _tcscpy(text, _("Out and return"));
      FAIShape = true;
    }
    else
      _tcscpy(text, _("Two legs"));
    break;

  case 4:
    if (!task->get_factory().is_unique() ||!task->get_factory().is_closed())
      _tcscpy(text, _("Three legs"));
    else if (task->get_factory().TestFAITriangle()) {
      _tcscpy(text, _("FAI triangle"));
      FAIShape = true;
    }
    else
      _tcscpy(text, _("non-FAI triangle"));
    break;

  default:
    _stprintf(text, _("%d legs"), task->task_size() - 1);
    break;
  }
  return FAIShape;
}
void
OrderedTaskSummary(OrderedTask* task, TCHAR* text, bool linebreaks)
{
  const TaskStats &stats = task->get_stats();
  TCHAR summary_shape[100];
  bool FAIShape = TaskSummaryShape(task, summary_shape);
  if (FAIShape || task->get_factory_type() == TaskBehaviour::FACTORY_FAI_GENERAL) {
    if (!task->get_factory().validateFAIOZs()) {
      _tcscat(summary_shape, _T("/ "));
      _tcscat(summary_shape, getTaskValidationErrors(
          task->get_factory().getValidationErrors()));
    }
  }


  TCHAR linebreak[3];
  if (linebreaks) {
    linebreak[0] = '\n';
    linebreak[1] = 0;
  } else {
    linebreak[0] = ',';
    linebreak[1] = ' ';
    linebreak[2] = 0;
  }

  if (!task->task_size()) {
    _stprintf(text, _("Task is empty (%s)"),
             OrderedTaskFactoryName(task->get_factory_type()));
  } else {
    if (task->has_targets())
      _stprintf(text, _T("%s%s%.0f %s%s%s %.0f %s%s%s %.0f %s (%s)"),
                summary_shape,
                linebreak,
                (double)Units::ToUserDistance(stats.distance_nominal),
                Units::GetDistanceName(),
                linebreak,
                _("max."),
                (double)Units::ToUserDistance(stats.distance_max),
                Units::GetDistanceName(),
                linebreak,
                _("min."),
                (double)Units::ToUserDistance(stats.distance_min),
                Units::GetDistanceName(),
                OrderedTaskFactoryName(task->get_factory_type()));
    else
      _stprintf(text, _T("%s%s%s %.0f %s (%s)"),
                summary_shape,
                linebreak,
                _("dist."),
                (double)Units::ToUserDistance(stats.distance_nominal),
                Units::GetDistanceName(),
                OrderedTaskFactoryName(task->get_factory_type()));
  }
}

class LabelObservationZone
{
public:
  LabelObservationZone(TCHAR* buff): text(buff) {}

  void
  Visit(const FAISectorZone& oz)
  {
    _stprintf(text, _("FAI quadrant"));
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

void
OrderedTaskPointLabel(const OrderedTaskPoint &tp, unsigned index, TCHAR* name)
{
  switch (tp.GetType()) {
  case TaskPoint::START:
    _stprintf(name, _T("S:  %s"), tp.get_waypoint().Name.c_str());
    break;

  case TaskPoint::AST:
    _stprintf(name, _T("T%d: %s"), index, tp.get_waypoint().Name.c_str());
    break;

  case TaskPoint::AAT:
    _stprintf(name, _T("A%d: %s"), index, tp.get_waypoint().Name.c_str());
    break;

  case TaskPoint::FINISH:
    _stprintf(name, _T("F:  %s"), tp.get_waypoint().Name.c_str());
    break;

  default:
    break;
  }
}

void
OrderedTaskPointRadiusLabel(const ObservationZonePoint &ozp, TCHAR* radius)
{
  switch (ozp.shape) {
  case ObservationZonePoint::FAI_SECTOR:
    _stprintf(radius,_("FAI quadrant"));
    break;

  case ObservationZonePoint::SECTOR:
  case ObservationZonePoint::ANNULAR_SECTOR:
    _stprintf(radius,_T("%s  - %s: %.1f%s"), _("Sector"), _("Radius"),
              (double)Units::ToUserDistance(((const SectorZone &)ozp).getRadius()),
              Units::GetDistanceName());
    break;

  case ObservationZonePoint::LINE:
    _stprintf(radius,_T("%s  - %s: %.1f%s"), _("Line"), _("Length"),
              (double)Units::ToUserDistance(((const LineSectorZone &)ozp).getLength()),
              Units::GetDistanceName());
    break;

  case ObservationZonePoint::CYLINDER:
    _stprintf(radius,_T("%s  - %s: %.1f%s"), _("Cylinder"), _("Radius"),
              (double)Units::ToUserDistance(((const CylinderZone &)ozp).getRadius()),
              Units::GetDistanceName());
    break;

  case ObservationZonePoint::KEYHOLE:
    _stprintf(radius,_("DAeC Keyhole"));
    break;

  case ObservationZonePoint::BGAFIXEDCOURSE:
    _stprintf(radius,_("BGA Fixed Course"));
    break;

  case ObservationZonePoint::BGAENHANCEDOPTION:
    _stprintf(radius,_("BGA Enhanced Option"));
    break;

  case ObservationZonePoint::BGA_START:
    _stprintf(radius,_("BGA Start Sector"));
    break;
  }
}

const TCHAR*
OrderedTaskPointDescription(AbstractTaskFactory::LegalPointType_t type)
{
  switch (type) {
  case AbstractTaskFactory::START_SECTOR:
    return _("A 90 degree sector with 1km radius. Cross corner edge from inside area to start");
  case AbstractTaskFactory::START_LINE:
    return _("A straight line.  Cross line from inside area to start");
  case AbstractTaskFactory::START_CYLINDER:
    return _("A cylinder.  Exit area to start");
  case AbstractTaskFactory::START_BGA:
    return _("A 180 degree sector with 5km radius.  Exit area in any direction to start");
  case AbstractTaskFactory::FAI_SECTOR:
    return _("A 90 degree sector with 'infinite' length sides.  Cross any edge, scored from corner point");
  case AbstractTaskFactory::AST_CYLINDER:
    return _("A cylinder.  Any point within area scored from center");
  case AbstractTaskFactory::KEYHOLE_SECTOR:
    return _("(German rules) Any point within 1/2 km of center or 10km of a 90 degree sector.  Scored from center");
  case AbstractTaskFactory::BGAFIXEDCOURSE_SECTOR:
    return _("(British rules) Any point within 1/2 km of center or 20km of a 90 degree sector.  Scored from center");
  case AbstractTaskFactory::BGAENHANCEDOPTION_SECTOR:
    return _("(British rules) Any point within 1/2 km of center or 10km of a 180 degree sector.  Scored from center");
  case AbstractTaskFactory::AAT_CYLINDER:
    return _("A cylinder.  Scored by farthest point reached in area");
  case AbstractTaskFactory::AAT_SEGMENT:
    return _("A sector that can vary in angle and radius.  Scored by farthest point reached inside area");
  case AbstractTaskFactory::AAT_ANNULAR_SECTOR:
    return _("A sector that can vary in angle, inner and outer radius.  Scored by farthest point reached inside area");
  case AbstractTaskFactory::FINISH_SECTOR:
    return _("A 90 degree sector with 1km radius.  Cross edge to finish");
  case AbstractTaskFactory::FINISH_LINE:
    return _("Cross line into area to finish");
  case AbstractTaskFactory::FINISH_CYLINDER:
    return _("Enter cylinder to finish");
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
    return _("FAI start quadrant");
  case AbstractTaskFactory::START_LINE:
    return _("Start line");
  case AbstractTaskFactory::START_CYLINDER:
    return _("Start cylinder");
  case AbstractTaskFactory::START_BGA:
    return _("BGA start sector");
  case AbstractTaskFactory::FAI_SECTOR:
    return _("FAI quadrant");
  case AbstractTaskFactory::KEYHOLE_SECTOR:
    return _("Keyhole sector (DAeC)");
  case AbstractTaskFactory::BGAFIXEDCOURSE_SECTOR:
    return _("BGA Fixed Course sector");
  case AbstractTaskFactory::BGAENHANCEDOPTION_SECTOR:
    return _("BGA Enhanced Option Fixed Course sector");
  case AbstractTaskFactory::AST_CYLINDER:
    return _("Turn point cylinder");
  case AbstractTaskFactory::AAT_CYLINDER:
    return _("Area cylinder");
  case AbstractTaskFactory::AAT_SEGMENT:
    return _("Area sector");
  case AbstractTaskFactory::AAT_ANNULAR_SECTOR:
    return _("Area sector with inner radius");
  case AbstractTaskFactory::FINISH_SECTOR:
    return _("FAI finish quadrant");
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
OrderedTaskSave(SingleWindow &parent, const OrderedTask& task, bool noask)
{
  assert(protected_task_manager != NULL);

  if (!noask
      && MessageBoxX(_("Save task?"), _("Task Selection"),
                     MB_YESNO | MB_ICONQUESTION) != IDYES)
    return false;

  TCHAR fname[69] = _T("");
  if (!dlgTextEntryShowModal(parent, fname, 64))
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
  case AbstractTaskFactory::NON_FAI_OZS:
    return _("non-FAI turn points");
    break;
  }

  return _T("");
}
