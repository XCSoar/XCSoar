/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "harness_task.hpp"
#include "test_debug.hpp"
#include "harness_waypoints.hpp"

#include "Task/Factory/AbstractTaskFactory.hpp"


bool test_task_bad(TaskManager& task_manager,
                   const Waypoints& waypoints)
{
  test_task_random(task_manager,waypoints,2);

  task_manager.SetFactory(TaskFactoryType::RACING);
  AbstractTaskFactory& fact = task_manager.GetFactory();

  const Waypoint* wp = random_waypoint(waypoints);

  ok (!fact.CreateFinish((TaskPointFactoryType)20,*wp),"bad finish type",0);
  ok (!fact.CreateStart((TaskPointFactoryType)20,*wp),"bad start type",0);
  ok (!fact.CreateIntermediate((TaskPointFactoryType)20,*wp),"bad intermediate type",0);

  // now create a taskpoint from FAI

  TaskPointFactoryType s =
    fact.GetIntermediateTypes()[(rand() % fact.GetIntermediateTypes().size())];

  // test it is bad for AAT

  task_manager.SetFactory(TaskFactoryType::AAT);

  AbstractTaskFactory& bfact = task_manager.GetFactory();

  ok (!bfact.CreateIntermediate(s,*wp),"bad intermediate type (after task change)",0);

  bfact.Remove(1);
  ok (bfact.Validate(),"ok with one tp",0);

  bfact.Remove(1);
  ok (bfact.Validate(),"ok with zero tps (just start and finish)",0);

  ok (bfact.Remove(task_manager.TaskSize()-1,false),"remove finish manually",0);
  ok (!bfact.Validate(),"aat is invalid (no finish)",0);

  return true;
}
