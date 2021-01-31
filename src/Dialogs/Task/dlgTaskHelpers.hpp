/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include <cstdint>

enum class TaskPointType : uint8_t;
class OrderedTask;
class ObservationZonePoint;

/**
 *
 * @param task The Task
 * @param text A buffer written to
 * @param linebreaks True if each summary item should be separated with a line break
 */
void
OrderedTaskSummary(const OrderedTask *task, TCHAR *text, bool linebreaks);

void
OrderedTaskPointLabel(TaskPointType type, const TCHAR *name,
                      unsigned index, TCHAR *buffer);

void OrderedTaskPointRadiusLabel(const ObservationZonePoint &ozp, TCHAR* radius);

bool
OrderedTaskSave(OrderedTask &task);

#endif


