// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "FAITaskFactory.hpp"

/**
 * Factory for construction of legal FAI tasks
 */
class FAIGoalTaskFactory: 
  public FAITaskFactory 
{
public:
/** 
 * Constructor
 * 
 * @param _task Ordered task to be managed by this factory
 * @param tb Behaviour (options)
 */  
  FAIGoalTaskFactory(OrderedTask& _task,
                 const TaskBehaviour &tb);
};
