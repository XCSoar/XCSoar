/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Language/Language.hpp"
#include "Units/Units.hpp"
#include "Task/TypeStrings.hpp"
#include "Task/ValidationErrorStrings.hpp"
#include "Task/SaveFile.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/ObservationZones/SectorZone.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/Shapes/FAITriangleTask.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Points/Type.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "LocalPath.hpp"
#include "OS/Path.hpp"

#include <assert.h>

/**
 *
 * @param task
 * @param text
 * @return True if FAI shape
 */
static bool
TaskSummaryShape(const OrderedTask *task, TCHAR *text)
{
  bool FAIShape = false;
  switch (task->TaskSize()) {
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
    if (task->GetFactory().IsClosed()) {
      _tcscpy(text, _("Out and return"));
      FAIShape = true;
    }
    else
      _tcscpy(text, _("Two legs"));
    break;

  case 4:
    if (!task->GetFactory().IsUnique() ||!task->GetFactory().IsClosed())
      _tcscpy(text, _("Three legs"));
    else if (FAITriangleValidator::Validate(*task)) {
      _tcscpy(text, _("FAI triangle"));
      FAIShape = true;
    }
    else
      _tcscpy(text, _("non-FAI triangle"));
    break;

  default:
    StringFormatUnsafe(text, _("%d legs"), task->TaskSize() - 1);
    break;
  }
  return FAIShape;
}
void
OrderedTaskSummary(const OrderedTask *task, TCHAR *text, bool linebreaks)
{
  const TaskStats &stats = task->GetStats();
  TCHAR summary_shape[100];
  bool FAIShape = TaskSummaryShape(task, summary_shape);
  if (FAIShape || task->GetFactoryType() == TaskFactoryType::FAI_GENERAL) {
    if (!task->GetFactory().ValidateFAIOZs()) {
      _tcscat(summary_shape, _T("/ "));
      _tcscat(summary_shape, getTaskValidationErrors(
          task->GetFactory().GetValidationErrors()));
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

  if (!task->TaskSize()) {
    StringFormatUnsafe(text, _("Task is empty (%s)"),
                       OrderedTaskFactoryName(task->GetFactoryType()));
  } else {
    if (task->HasTargets())
      StringFormatUnsafe(text, _T("%s%s%.0f %s%s%s %.0f %s%s%s %.0f %s (%s)"),
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
                         OrderedTaskFactoryName(task->GetFactoryType()));
    else
      StringFormatUnsafe(text, _T("%s%s%s %.0f %s (%s)"),
                         summary_shape,
                         linebreak,
                         _("dist."),
                         (double)Units::ToUserDistance(stats.distance_nominal),
                         Units::GetDistanceName(),
                         OrderedTaskFactoryName(task->GetFactoryType()));
  }
}

void
OrderedTaskPointLabel(TaskPointType type, const TCHAR *name,
                      unsigned index, TCHAR* buffer)
{
  switch (type) {
  case TaskPointType::START:
    StringFormatUnsafe(buffer, _T("S: %s"), name);
    break;

  case TaskPointType::AST:
    StringFormatUnsafe(buffer, _T("T%d: %s"), index, name);
    break;

  case TaskPointType::AAT:
    StringFormatUnsafe(buffer, _T("A%d: %s"), index, name);
    break;

  case TaskPointType::FINISH:
    StringFormatUnsafe(buffer, _T("F: %s"), name);
    break;

  default:
    break;
  }
}

void
OrderedTaskPointRadiusLabel(const ObservationZonePoint &ozp, TCHAR* buffer)
{
  switch (ozp.GetShape()) {
  case ObservationZone::Shape::FAI_SECTOR:
    _tcscpy(buffer, _("FAI quadrant"));
    return;

  case ObservationZone::Shape::SECTOR:
  case ObservationZone::Shape::ANNULAR_SECTOR:
    StringFormatUnsafe(buffer,_T("%s - %s: %.1f%s"), _("Sector"), _("Radius"),
                       (double)Units::ToUserDistance(((const SectorZone &)ozp).GetRadius()),
                       Units::GetDistanceName());
    return;

  case ObservationZone::Shape::LINE:
    StringFormatUnsafe(buffer,_T("%s - %s: %.1f%s"), _("Line"), _("Gate width"),
                       (double)Units::ToUserDistance(((const LineSectorZone &)ozp).GetLength()),
                       Units::GetDistanceName());
    return;

  case ObservationZone::Shape::CYLINDER:
    StringFormatUnsafe(buffer,_T("%s - %s: %.1f%s"), _("Cylinder"), _("Radius"),
                       (double)Units::ToUserDistance(((const CylinderZone &)ozp).GetRadius()),
                       Units::GetDistanceName());
    return;

  case ObservationZone::Shape::MAT_CYLINDER:
    _tcscpy(buffer, _("MAT cylinder"));
    return;

  case ObservationZone::Shape::CUSTOM_KEYHOLE:
    StringFormatUnsafe(buffer,_T("%s - %s: %.1f%s"), _("Keyhole"), _("Radius"),
                       (double)Units::ToUserDistance(((const KeyholeZone &)ozp).GetRadius()),
                       Units::GetDistanceName());
    return;

  case ObservationZone::Shape::DAEC_KEYHOLE:
    _tcscpy(buffer, _("DAeC Keyhole"));
    return;

  case ObservationZone::Shape::BGAFIXEDCOURSE:
    _tcscpy(buffer, _("BGA Fixed Course"));
    return;

  case ObservationZone::Shape::BGAENHANCEDOPTION:
    _tcscpy(buffer, _("BGA Enhanced Option"));
    return;

  case ObservationZone::Shape::BGA_START:
    _tcscpy(buffer, _("BGA Start Sector"));
    return;

  case ObservationZone::Shape::SYMMETRIC_QUADRANT:
    _tcscpy(buffer, _("Symmetric quadrant"));
    return;
  }

  gcc_unreachable();
  assert(false);
}

bool
OrderedTaskSave(OrderedTask &task)
{
  TCHAR fname[69] = _T("");
  if (!TextEntryDialog(fname, 64, _("Enter a task name")))
    return false;

  const auto tasks_path = MakeLocalPath(_T("tasks"));

  _tcscat(fname, _T(".tsk"));
  task.SetName(StaticString<64>(fname));
  SaveTask(AllocatedPath::Build(tasks_path, fname), task);
  return true;
}
