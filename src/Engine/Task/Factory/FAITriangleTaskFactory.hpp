/* Copyright_License {

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
#ifndef FAI_TRIANGLE_TASK_FACTORY_HPP
#define FAI_TRIANGLE_TASK_FACTORY_HPP

#include "FAITaskFactory.hpp"

/**
 * Factory for construction of legal FAI tasks
 * Currently the validate() method will check 4-point tasks as to whether they
 * satisfy short and long-distance FAI triangle rules.
 */
class FAITriangleTaskFactory: 
  public FAITaskFactory 
{
public:
/** 
 * Constructor
 * 
 * @param _task Ordered task to be managed by this factory
 * @param tb Behaviour (options)
 */  
  FAITriangleTaskFactory(OrderedTask& _task,
                 const TaskBehaviour &tb);

  virtual ~FAITriangleTaskFactory() {};

/** 
 * Check whether task is complete and valid according to factory rules
 * 
 * @return True if task is valid according to factory rules
 */
  bool Validate() override;
};

#endif
