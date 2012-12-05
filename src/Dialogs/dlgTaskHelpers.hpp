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

#ifndef DIALOG_TASK_HELPERS_HPP
#define DIALOG_TASK_HELPERS_HPP

#include "Task/Points/TaskPoint.hpp"

#include <tchar.h>
#include <stdint.h>

enum class TaskFactoryType : uint8_t;
enum class TaskPointFactoryType : uint8_t;
enum class TaskValidationErrorType : uint8_t;
class TaskValidationErrorVector;
class OrderedTask;
class ObservationZonePoint;

const TCHAR* OrderedTaskFactoryDescription(TaskFactoryType type);
const TCHAR* OrderedTaskFactoryName(TaskFactoryType type);

/**
 *
 * @param task The Task
 * @param text A buffer written to
 * @param linebreaks True if each summary item should be separated with a line break
 */
void OrderedTaskSummary(OrderedTask* task, TCHAR* text, bool linebreaks);
void OrderedTaskPointLabel(TaskPoint::Type type, const TCHAR *name,
                           unsigned index, TCHAR* buffer);
void OrderedTaskPointRadiusLabel(const ObservationZonePoint &ozp, TCHAR* radius);
bool OrderedTaskSave(const OrderedTask& task, bool noask=false);

const TCHAR* OrderedTaskPointDescription(TaskPointFactoryType type);
const TCHAR* OrderedTaskPointName(TaskPointFactoryType type);

const TCHAR *
getTaskValidationErrors(const TaskValidationErrorVector &v);

const TCHAR *
TaskValidationError(TaskValidationErrorType type);

#endif


