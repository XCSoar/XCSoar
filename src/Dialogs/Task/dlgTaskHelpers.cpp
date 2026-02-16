// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "system/Path.hpp"

#include <cassert>

/**
 *
 * @param task
 * @param text
 * @return True if FAI shape
 */
static bool
TaskSummaryShape(const OrderedTask *task, char *text)
{
  bool FAIShape = false;
  switch (task->TaskSize()) {
  case 0:
    text[0] = '\0';
    break;

  case 1:
    strcpy(text, _("Unknown"));
    break;

  case 2:
    strcpy(text, _("Goal"));
    FAIShape = true;

    break;

  case 3:
    if (task->GetFactory().IsClosed()) {
      strcpy(text, _("Out and return"));
      FAIShape = true;
    }
    else
      strcpy(text, _("Two legs"));
    break;

  case 4:
    if (!task->GetFactory().IsUnique() ||!task->GetFactory().IsClosed())
      strcpy(text, _("Three legs"));
    else if (FAITriangleValidator::Validate(*task)) {
      strcpy(text, _("FAI triangle"));
      FAIShape = true;
    }
    else
      strcpy(text, _("non-FAI triangle"));
    break;

  default:
    StringFormatUnsafe(text, _("%d legs"), task->TaskSize() - 1);
    break;
  }
  return FAIShape;
}
void
OrderedTaskSummary(const OrderedTask *task, char *text, bool linebreaks)
{
  const TaskStats &stats = task->GetStats();
  char summary_shape[100];
  bool FAIShape = TaskSummaryShape(task, summary_shape);
  TaskValidationErrorSet validation_errors;
  if (FAIShape || task->GetFactoryType() == TaskFactoryType::FAI_GENERAL)
    validation_errors = task->GetFactory().ValidateFAIOZs();

  char linebreak[3];
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
      StringFormatUnsafe(text, "%s%s%s%s%.0f %s%s%s %.0f %s%s%s %.0f %s (%s)",
                         summary_shape,
                         validation_errors.IsEmpty() ? "" : " / ",
                         validation_errors.IsEmpty() ? "" : getTaskValidationErrors(validation_errors),
                         linebreak,
                         (double)Units::ToUserDistance(stats.distance_nominal),
                         Units::GetDistanceName(),
                         linebreak,
                         _("max."),
                         (double)Units::ToUserDistance(stats.distance_max_total),
                         Units::GetDistanceName(),
                         linebreak,
                         _("min."),
                         (double)Units::ToUserDistance(stats.distance_min),
                         Units::GetDistanceName(),
                         OrderedTaskFactoryName(task->GetFactoryType()));
    else
      StringFormatUnsafe(text, "%s%s%s%s%s %.0f %s (%s)",
                         summary_shape,
                         validation_errors.IsEmpty() ? "" : " / ",
                         validation_errors.IsEmpty() ? "" : getTaskValidationErrors(validation_errors),
                         linebreak,
                         _("dist."),
                         (double)Units::ToUserDistance(stats.distance_nominal),
                         Units::GetDistanceName(),
                         OrderedTaskFactoryName(task->GetFactoryType()));
  }
}

void
OrderedTaskPointLabel(TaskPointType type, const char *name,
                      unsigned index, char* buffer)
{
  switch (type) {
  case TaskPointType::START:
    StringFormatUnsafe(buffer, "S: %s", name);
    break;

  case TaskPointType::AST:
    StringFormatUnsafe(buffer, "T%d: %s", index, name);
    break;

  case TaskPointType::AAT:
    StringFormatUnsafe(buffer, "A%d: %s", index, name);
    break;

  case TaskPointType::FINISH:
    StringFormatUnsafe(buffer, "F: %s", name);
    break;

  default:
    break;
  }
}

void
OrderedTaskPointRadiusLabel(const ObservationZonePoint &ozp, char* buffer)
{
  switch (ozp.GetShape()) {
  case ObservationZone::Shape::FAI_SECTOR:
    strcpy(buffer, _("FAI quadrant"));
    return;

  case ObservationZone::Shape::SECTOR:
  case ObservationZone::Shape::ANNULAR_SECTOR:
    StringFormatUnsafe(buffer,"%s - %s: %.1f%s", _("Sector"), _("Radius"),
                       (double)Units::ToUserDistance(((const SectorZone &)ozp).GetRadius()),
                       Units::GetDistanceName());
    return;

  case ObservationZone::Shape::LINE:
    StringFormatUnsafe(buffer,"%s - %s: %.1f%s", _("Line"), _("Gate width"),
                       (double)Units::ToUserDistance(((const LineSectorZone &)ozp).GetLength()),
                       Units::GetDistanceName());
    return;

  case ObservationZone::Shape::CYLINDER:
    StringFormatUnsafe(buffer,"%s - %s: %.1f%s", _("Cylinder"), _("Radius"),
                       (double)Units::ToUserDistance(((const CylinderZone &)ozp).GetRadius()),
                       Units::GetDistanceName());
    return;

  case ObservationZone::Shape::MAT_CYLINDER:
    strcpy(buffer, _("MAT cylinder"));
    return;

  case ObservationZone::Shape::CUSTOM_KEYHOLE:
    StringFormatUnsafe(buffer,"%s - %s: %.1f%s", _("Keyhole"), _("Radius"),
                       (double)Units::ToUserDistance(((const KeyholeZone &)ozp).GetRadius()),
                       Units::GetDistanceName());
    return;

  case ObservationZone::Shape::DAEC_KEYHOLE:
    strcpy(buffer, _("DAeC Keyhole"));
    return;

  case ObservationZone::Shape::BGAFIXEDCOURSE:
    strcpy(buffer, _("BGA Fixed Course"));
    return;

  case ObservationZone::Shape::BGAENHANCEDOPTION:
    strcpy(buffer, _("BGA Enhanced Option"));
    return;

  case ObservationZone::Shape::BGA_START:
    strcpy(buffer, _("BGA Start Sector"));
    return;

  case ObservationZone::Shape::SYMMETRIC_QUADRANT:
    strcpy(buffer, _("Symmetric quadrant"));
    return;
  }

  gcc_unreachable();
  assert(false);
}

bool
OrderedTaskSave(OrderedTask &task)
{
  char fname[69] = "";
  if (!TextEntryDialog(fname, 64, _("Enter a task name")))
    return false;

  const auto tasks_path = MakeLocalPath("tasks");

  strcat(fname, ".tsk");
  task.SetName(fname);
  SaveTask(AllocatedPath::Build(tasks_path, fname), task);
  return true;
}
