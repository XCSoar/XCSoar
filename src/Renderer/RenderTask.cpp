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

#include "RenderTask.hpp"
#include "Task/Tasks/AbstractTask.hpp"
#include "Task/Tasks/GotoTask.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/Tasks/AbortTask.hpp"
#include "RenderTaskPoint.hpp"

RenderTask::RenderTask(RenderTaskPoint &_tpv, GeoBounds _screen_bounds)
  :tpv(_tpv), screen_bounds(_screen_bounds) {}

void 
RenderTask::DrawLayers(const AbstractTask &task)
{
  for (unsigned i = 0; i < 4; i++) {
    tpv.set_layer((RenderTaskLayer)i);

    switch (task.type) {
    case TaskInterface::ORDERED: {
      const OrderedTask &ordered_task = (const OrderedTask &)task;
      if (i != RENDER_TASK_SYMBOLS && i != RENDER_TASK_LEG) {
        tpv.set_mode_optional(true);

        for (unsigned j = 0, end = ordered_task.optional_start_points_size();
             j < end; ++j)
          tpv.Draw(*ordered_task.get_optional_start(j));
      }

      tpv.set_mode_optional(false);
      for (unsigned j = 0, end = task.task_size(); j < end; ++j)
        tpv.Draw(*ordered_task.getTaskPoint(j));

      break;
    }

    case TaskInterface::ABORT: {
      const AbortTask &abort_task = (const AbortTask &)task;

      tpv.set_mode_optional(false);
      for (unsigned j = 0, end = abort_task.task_size(); j < end; ++j)
        tpv.Draw(abort_task.GetAlternate(j));

      break;
    }

    case TaskInterface::GOTO:
      tpv.set_mode_optional(false);
      tpv.Draw(*task.getActiveTaskPoint());
      break;
    }
  }
}

void 
RenderTask::Visit(const AbortTask &task)
{
  tpv.set_active_index(task.getActiveIndex());
  DrawLayers(task);
}

void 
RenderTask::Visit(const OrderedTask &task)
{
  tpv.set_bounding_box(task.get_bounding_box(screen_bounds));
  tpv.set_active_index(task.getActiveIndex());
  DrawLayers(task);
}

void 
RenderTask::Visit(const GotoTask &task)
{
  tpv.set_active_index(0);
  DrawLayers(task);
}
