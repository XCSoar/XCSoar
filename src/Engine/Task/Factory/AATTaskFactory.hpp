/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */

#ifndef AAT_TASK_FACTORY_HPP
#define AAT_TASK_FACTORY_HPP

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
  AATTaskFactory(OrderedTask& _task,
                 const TaskBehaviour &tb);

  ~AATTaskFactory() {};

  /**
   * swaps non AAT OZs with either AAT_SEGMENT or AAT_CYLINDER
   * based on the shape of the input point
   * @param tp
   * @return: point type compatible with current factory, most
   * similar to type of tp
   */
  TaskPointFactoryType GetMutatedPointType(const OrderedTaskPoint &tp) const override;
};


#endif
