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

#include "Create.hpp"
#include "RTTaskFactory.hpp"
#include "FAITaskFactory.hpp"
#include "FAITriangleTaskFactory.hpp"
#include "FAIORTaskFactory.hpp"
#include "FAIGoalTaskFactory.hpp"
#include "AATTaskFactory.hpp"
#include "MatTaskFactory.hpp"
#include "MixedTaskFactory.hpp"
#include "TouringTaskFactory.hpp"

AbstractTaskFactory *
CreateTaskFactory(TaskFactoryType type, OrderedTask &task,
                  const TaskBehaviour &task_behaviour)
{
  switch (type) {
  case TaskFactoryType::RACING:
    return new RTTaskFactory(task, task_behaviour);

  case TaskFactoryType::FAI_GENERAL:
    return new FAITaskFactory(task, task_behaviour);

  case TaskFactoryType::FAI_TRIANGLE:
    return new FAITriangleTaskFactory(task, task_behaviour);

  case TaskFactoryType::FAI_OR:
    return new FAIORTaskFactory(task, task_behaviour);

  case TaskFactoryType::FAI_GOAL:
    return new FAIGoalTaskFactory(task, task_behaviour);

  case TaskFactoryType::AAT:
    return new AATTaskFactory(task, task_behaviour);

  case TaskFactoryType::MAT:
    return new MatTaskFactory(task, task_behaviour);

  case TaskFactoryType::MIXED:
    return new MixedTaskFactory(task, task_behaviour);

  case TaskFactoryType::TOURING:
    return new TouringTaskFactory(task, task_behaviour);

  case TaskFactoryType::COUNT:
    gcc_unreachable();
  };

  /* not reachable */
  gcc_unreachable();
}
