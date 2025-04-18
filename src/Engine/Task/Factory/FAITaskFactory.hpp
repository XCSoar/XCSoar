// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "AbstractTaskFactory.hpp"

/**
 * Factory for construction of legal FAI tasks
 * Currently the validate() method will check 4-point tasks as to whether they
 * satisfy short and long-distance FAI triangle rules.
 */
class FAITaskFactory: 
  public AbstractTaskFactory 
{
protected:
  FAITaskFactory(const TaskFactoryConstraints &_constraints,
                 OrderedTask &_task,
                 const TaskBehaviour &tb) noexcept;

public:
/** 
 * Constructor
 * 
 * @param _task Ordered task to be managed by this factory
 * @param tb Behaviour (options)
 */  
  FAITaskFactory(OrderedTask &_task,
                 const TaskBehaviour &tb) noexcept;

  void UpdateOrderedTaskSettings(OrderedTaskSettings &to) noexcept override;

  TaskValidationErrorSet Validate() const noexcept override;

  /**
   * swaps non FAI OZs with either FAI OZs
   * based on the shape of the input point
   * @param tp
   * @return: point type compatible with current factory, most
   * similar to type of tp
   */
  TaskPointFactoryType GetMutatedPointType(const OrderedTaskPoint &tp) const noexcept override;

  /**
   * @param start_radius: either -1 or a valid value
   * @param turnpoint_radius: either -1 or a valid value
   * @param finish_radius: either -1 or a valid value
   *
   * only affects start cylinder/line, finish cylinder/line and
   * turnpoint sector
   * Does not affects FAI sectors which have their own class types
   *
   * sets radiuses FAI defaults
   */
  void GetPointDefaultSizes(const TaskPointFactoryType type,
                            double &start_radius,
                            double &turnpoint_radius,
                            double &finish_radius) const noexcept override;
};
