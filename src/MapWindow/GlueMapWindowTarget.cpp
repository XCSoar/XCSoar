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

#include "GlueMapWindow.hpp"
#include "Task/ProtectedTaskManager.hpp"

int
GlueMapWindow::isInAnyActiveSector(const GeoPoint &gp)
{
  assert(task != NULL);

  ProtectedTaskManager::Lease task_manager(*task);
  const AbstractTask *at = task_manager->get_active_task();
  if (at == NULL)
    return -1;

  const unsigned TaskSize = at->task_size();
  const unsigned ActiveIndex = task_manager->getActiveTaskPointIndex();

  if (task_manager->get_mode() != TaskManager::MODE_ORDERED)
    return -1;

  AircraftState a;
  a.location = gp;

  for (unsigned i = ActiveIndex; i < TaskSize; i++) {
    if (task_manager->isInSector(i, a, false))
      return i;
  }

  return -1;
}
