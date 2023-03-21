// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FAITriangleTask.hpp"
#include "FAITriangleRules.hpp"
#include "../Ordered/OrderedTask.hpp"
#include "../Ordered/Points/OrderedTaskPoint.hpp"
#include "../Factory/AbstractTaskFactory.hpp"

bool
FAITriangleValidator::Validate(const OrderedTask &task)
{
  if (!task.GetFactory().IsUnique())
    return false;

  if (task.TaskSize() != 4)
    return false;

  const double d1 = task.GetTaskPoint(1).GetVectorPlanned().distance;
  const double d2 = task.GetTaskPoint(2).GetVectorPlanned().distance;
  const double d3 = task.GetTaskPoint(3).GetVectorPlanned().distance;

  return FAITriangleRules::TestDistances(d1, d2, d3,
                                         task.GetOrderedTaskSettings().fai_triangle);
}
