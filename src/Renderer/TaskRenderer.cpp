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

#include "TaskRenderer.hpp"
#include "Task/Tasks/AbstractTask.hpp"
#include "Task/Tasks/GotoTask.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/Tasks/AbortTask.hpp"
#include "RenderTaskPoint.hpp"

TaskRenderer::TaskRenderer(RenderTaskPoint &_tpv, GeoBounds _screen_bounds)
  :tpv(_tpv), screen_bounds(_screen_bounds) {}

void 
TaskRenderer::Draw(const AbortTask &task)
{
  tpv.SetActiveIndex(task.getActiveIndex());
  for (unsigned i = 0; i < 4; i++) {
    tpv.ResetIndex();

    tpv.SetModeOptional(false);
    for (unsigned j = 0, end = task.TaskSize(); j < end; ++j)
      tpv.Draw(task.GetAlternate(j), (RenderTaskLayer)i);
  }
}

void 
TaskRenderer::Draw(const OrderedTask &task)
{
  tpv.SetBoundingBox(task.get_bounding_box(screen_bounds));
  tpv.SetActiveIndex(task.getActiveIndex());
  for (unsigned i = 0; i < 4; i++) {
    tpv.ResetIndex();

    if (i != RENDER_TASK_SYMBOLS && i != RENDER_TASK_LEG) {
      tpv.SetModeOptional(true);

      for (unsigned j = 0, end = task.optional_start_points_size(); j < end; ++j)
        tpv.Draw(*task.get_optional_start(j), (RenderTaskLayer)i);
    }

    tpv.SetModeOptional(false);
    for (unsigned j = 0, end = task.TaskSize(); j < end; ++j)
      tpv.Draw(*task.getTaskPoint(j), (RenderTaskLayer)i);
  }
}

void 
TaskRenderer::Draw(const GotoTask &task)
{
  tpv.SetActiveIndex(0);
  for (unsigned i = 0; i < 4; i++) {
    tpv.ResetIndex();

    tpv.SetModeOptional(false);
    tpv.Draw(*task.GetActiveTaskPoint(), (RenderTaskLayer)i);
  }
}

void
TaskRenderer::Draw(const TaskInterface &task)
{
  switch (task.type) {
  case TaskInterface::ORDERED:
    Draw((const OrderedTask &)task);
    break;

  case TaskInterface::ABORT:
    Draw((const AbortTask &)task);
    break;

  case TaskInterface::GOTO:
    Draw((const GotoTask &)task);
    break;
  }
}
