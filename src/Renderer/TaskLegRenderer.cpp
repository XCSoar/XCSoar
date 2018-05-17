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

#include "TaskLegRenderer.hpp"
#include "ChartRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Language/Language.hpp"


static bool
IsTaskLegVisible(const OrderedTaskPoint &tp)
{
  switch (tp.GetType()) {
  case TaskPointType::START:
    return tp.HasExited();

  case TaskPointType::FINISH:
  case TaskPointType::AAT:
  case TaskPointType::AST:
    return tp.HasEntered();

  case TaskPointType::UNORDERED:
    break;
  }

  gcc_unreachable();
}

void
RenderTaskLegs(ChartRenderer &chart,
               const TaskManager &task_manager,
               const NMEAInfo& basic,
               const DerivedInfo& calculated,
               const double y)
{
  const TaskStats &task_stats = calculated.ordered_task_stats;

  if (!task_stats.start.task_started)
    return;

  TCHAR sTmp[5];

  const OrderedTask &task = task_manager.GetOrderedTask();
  for (unsigned i = 0, n = task.TaskSize(); i < n; ++i) {
    const OrderedTaskPoint &tp = task.GetTaskPoint(i);
    if (!IsTaskLegVisible(tp))
      continue;

    auto x = tp.GetEnteredState().time - calculated.flight.takeoff_time;
    if (x >= 0) {
      x /= 3600;
      if (y>=0) {
        if (i==0) {
          chart.DrawBlankRectangle(chart.GetXMin(), chart.GetYMin(),
                                   x, chart.GetYMax());
        } else if (i+1 == task.TaskSize()) {
          chart.DrawBlankRectangle(x, chart.GetYMin(),
                                   chart.GetXMax(), chart.GetYMax());
        }
        chart.DrawLine(x, chart.GetYMin(), x, chart.GetYMax(),
                       ChartLook::STYLE_GRIDZERO);
      }
      if (y>=0) {
        StringFormatUnsafe(sTmp, _T("%d"), i);
        chart.DrawLabel(sTmp, x, chart.GetYMax()*y + chart.GetYMin()*(1-y));
      }
    }
  }
}
