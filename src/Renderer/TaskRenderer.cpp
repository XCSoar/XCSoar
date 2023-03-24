// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
