// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "AbstractTaskFactory.hpp"

/**
 * Factory for mixed tasks (mixture of AST and AAT sectors)
 * This is the most general of the factories.
 * The idea is that any other type of task can be "demoted"
 * to a Mixed AAT task with no changes required to an TPs.
 */
class MixedTaskFactory: public AbstractTaskFactory 
{
public:
/** 
 * Constructor
 * 
 * @param _task Ordered task to be managed by this factory
 * @param tb Behaviour (options)
 */  
  MixedTaskFactory(OrderedTask& _task,
                   const TaskBehaviour &tb);
};
