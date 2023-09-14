// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AbstractTaskFactory.hpp"

/**
 * Factory for construction of legal AAT tasks
 * Currently the validate() method simply checks that there is at least one
 * AAT turnpoint.
 */
class AATTaskFactory final : public AbstractTaskFactory
{
public:
/** 
 * Constructor
 * 
 * @param _task Ordered task to be managed by this factory
 * @param tb Behaviour (options)
 */  
  AATTaskFactory(OrderedTask &_task,
                 const TaskBehaviour &tb) noexcept;

  /**
   * swaps non AAT OZs with either AAT_SEGMENT or AAT_CYLINDER
   * based on the shape of the input point
   * @param tp
   * @return: point type compatible with current factory, most
   * similar to type of tp
   */
  TaskPointFactoryType GetMutatedPointType(const OrderedTaskPoint &tp) const noexcept override;
};
