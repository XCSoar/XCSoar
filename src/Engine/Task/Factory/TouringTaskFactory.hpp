// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "AbstractTaskFactory.hpp"

/**
 * Factory for construction of casual touring tasks
 */
class TouringTaskFactory: 
  public AbstractTaskFactory 
{
public:
/** 
 * Constructor
 * 
 * @param _task Ordered task to be managed by this factory
 * @param tb Behaviour (options)
 */  
  TouringTaskFactory(OrderedTask &_task,
                     const TaskBehaviour &tb) noexcept;

  void UpdateOrderedTaskSettings(OrderedTaskSettings &to) noexcept override;
};
