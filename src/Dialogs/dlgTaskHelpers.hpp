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
#ifndef DIALOG_TASK_HELPERS_HPP
#define DIALOG_TASK_HELPERS_HPP

#include <tchar.h>

#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Task/Tasks/BaseTask/TaskPoint.hpp"

class SingleWindow;

const TCHAR* OrderedTaskFactoryDescription(TaskBehaviour::Factory_t type);
const TCHAR* OrderedTaskFactoryName(TaskBehaviour::Factory_t type);

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
bool OrderedTaskSave(SingleWindow &parent,
                     const OrderedTask& task, bool noask=false);

const TCHAR* OrderedTaskPointDescription(AbstractTaskFactory::LegalPointType_t type);
const TCHAR* OrderedTaskPointName(AbstractTaskFactory::LegalPointType_t type);
const TCHAR* getTaskValidationErrors(
   const AbstractTaskFactory::TaskValidationErrorVector v);
const TCHAR* TaskValidationError(
   AbstractTaskFactory::TaskValidationErrorType_t type);

#endif


