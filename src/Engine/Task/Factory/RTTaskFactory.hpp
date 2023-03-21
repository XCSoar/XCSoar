// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "AbstractTaskFactory.hpp"

/**
 * Factory for construction of legal FAI racing tasks
 */
class RTTaskFactory: 
  public AbstractTaskFactory 
{
public:
/** 
 * Constructor
 * 
 * @param _task Ordered task to be managed by this factory
 * @param tb Behaviour (options)
 */  
  RTTaskFactory(OrderedTask &_task, const TaskBehaviour &tb) noexcept;

  /**
   * swaps AAT OZs for AST_CYLINDERs
   * @param tp
   * @return: point type compatible with current factory, most
   * similar to type of tp
   */
  TaskPointFactoryType GetMutatedPointType(const OrderedTaskPoint &tp) const noexcept override;
};
