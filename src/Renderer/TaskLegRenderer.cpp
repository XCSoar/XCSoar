// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskLegRenderer.hpp"
#include "ChartRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
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
               [[maybe_unused]] const NMEAInfo& basic,
               const DerivedInfo& calculated,
               const double y)
{
  const TaskStats &task_stats = calculated.ordered_task_stats;

  if (!task_stats.start.HasStarted())
    return;

  TCHAR sTmp[5];

  const OrderedTask &task = task_manager.GetOrderedTask();
  for (unsigned i = 0, n = task.TaskSize(); i < n; ++i) {
    const OrderedTaskPoint &tp = task.GetTaskPoint(i);
    if (!IsTaskLegVisible(tp))
      continue;

    auto dt = tp.GetScoredState().time - calculated.flight.takeoff_time;
    if (dt.count() >= 0) {
      const double x = dt / std::chrono::hours{1};
      if (y>=0) {
        if (i==0 && x>chart.GetXMin()) {
          chart.DrawBlankRectangle({chart.GetXMin(), chart.GetYMin()},
                                   {x, chart.GetYMax()});
        } else if (i+1 == task.TaskSize()) {
          chart.DrawBlankRectangle({std::max(x,chart.GetXMin()), chart.GetYMin()},
                                   {chart.GetXMax(), chart.GetYMax()});
        }
        if (x>chart.GetXMin()){
          chart.DrawLine({x, chart.GetYMin()}, {x, chart.GetYMax()},
                          ChartLook::STYLE_GRIDZERO);
        }
      }
      if (y>=0 && x>chart.GetXMin()) {
        StringFormatUnsafe(sTmp, _T("%d"), i);
        chart.DrawLabel({x, chart.GetYMax()*y + chart.GetYMin()*(1-y)},
                        sTmp);
      }
    }
  }
}
