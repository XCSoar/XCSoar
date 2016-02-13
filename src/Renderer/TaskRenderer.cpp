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

#include "TaskRenderer.hpp"
#include "Engine/Task/Unordered/GotoTask.hpp"
#include "Engine/Task/Unordered/AbortTask.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "TaskPointRenderer.hpp"

TaskRenderer::TaskRenderer(TaskPointRenderer &_tpv, GeoBounds _screen_bounds)
  :tpv(_tpv), screen_bounds(_screen_bounds) {}

void 
TaskRenderer::Draw(const AbortTask &task)
{
  tpv.SetActiveIndex(task.GetActiveIndex());
  tpv.SetModeOptional(false);

  for (unsigned i = 0; i < 4; i++) {
    tpv.ResetIndex();

    for (unsigned j = 0, end = task.TaskSize(); j < end; ++j)
      tpv.Draw(task.GetAlternate(j), (TaskPointRenderer::Layer)i);
  }
}

void 
TaskRenderer::Draw(const OrderedTask &task)
{
  tpv.SetBoundingBox(task.GetTaskProjection().Project(screen_bounds));
  tpv.SetActiveIndex(task.GetActiveIndex());
  for (unsigned i = 0; i < 4; i++) {
    tpv.ResetIndex();

    if (i != TaskPointRenderer::LAYER_SYMBOLS &&
        i != TaskPointRenderer::LAYER_LEG) {
      tpv.SetModeOptional(true);

      for (const auto &tp : task.GetOptionalStartPoints())
        tpv.Draw(tp, (TaskPointRenderer::Layer)i);
    }

    tpv.SetModeOptional(false);
    for (const auto &tp : task.GetPoints())
      tpv.Draw(tp, (TaskPointRenderer::Layer)i);
  }
}

void 
TaskRenderer::Draw(const GotoTask &task)
{
  tpv.SetActiveIndex(0);
  tpv.SetModeOptional(false);

  for (unsigned i = 0; i < 4; i++) {
    tpv.ResetIndex();

    tpv.Draw(*task.GetActiveTaskPoint(), (TaskPointRenderer::Layer)i);
  }
}

void
TaskRenderer::Draw(const TaskInterface &task)
{
  switch (task.GetType()) {
  case TaskType::NONE:
    break;

  case TaskType::ORDERED:
    Draw((const OrderedTask &)task);
    break;

  case TaskType::ABORT:
    Draw((const AbortTask &)task);
    break;

  case TaskType::GOTO:
    Draw((const GotoTask &)task);
    break;
  }
}
