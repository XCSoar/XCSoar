// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskRenderer.hpp"
#include "Engine/Task/Unordered/GotoTask.hpp"
#include "Engine/Task/Unordered/AbortTask.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "TaskPointRenderer.hpp"

void 
TaskRenderer::Draw(const AbortTask &task) noexcept
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
TaskRenderer::Draw(const OrderedTask &task) noexcept
{
  tpv.SetBoundingBox(task.GetTaskProjection().Project(screen_bounds));
  tpv.SetActiveIndex(task.GetActiveIndex());
  for (unsigned i = 0; i < 4; i++) {
    tpv.ResetIndex();

    const auto layer = static_cast<TaskPointRenderer::Layer>(i);
    if (layer != TaskPointRenderer::Layer::SYMBOLS &&
        layer != TaskPointRenderer::Layer::LEG) {
      tpv.SetModeOptional(true);

      for (const auto &tp : task.GetOptionalStartPoints())
        tpv.Draw(tp, (TaskPointRenderer::Layer)i);
    }

    tpv.SetModeOptional(false);
    for (const auto &tp : task.GetPoints())
      tpv.Draw(tp, layer);
  }
}

void 
TaskRenderer::Draw(const GotoTask &task) noexcept
{
  tpv.SetActiveIndex(0);
  tpv.SetModeOptional(false);

  for (unsigned i = 0; i < 4; i++) {
    tpv.ResetIndex();

    tpv.Draw(*task.GetActiveTaskPoint(), (TaskPointRenderer::Layer)i);
  }
}

void
TaskRenderer::Draw(const TaskInterface &task) noexcept
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
