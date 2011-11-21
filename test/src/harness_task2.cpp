/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

  task_manager.set_factory(TaskBehaviour::FACTORY_RT);
  AbstractTaskFactory& fact = task_manager.get_factory();

  const Waypoint* wp = random_waypoint(waypoints);

  ok (!fact.createFinish((AbstractTaskFactory::LegalPointType_t)20,*wp),"bad finish type",0);
  ok (!fact.createStart((AbstractTaskFactory::LegalPointType_t)20,*wp),"bad start type",0);
  ok (!fact.createIntermediate((AbstractTaskFactory::LegalPointType_t)20,*wp),"bad intermediate type",0);

  // now create a taskpoint from FAI

  AbstractTaskFactory::LegalPointType_t s = 
    fact.getIntermediateTypes()[(rand() % fact.getIntermediateTypes().size())];

  // test it is bad for AAT

  task_manager.set_factory(TaskBehaviour::FACTORY_AAT);

  AbstractTaskFactory& bfact = task_manager.get_factory();

  ok (!bfact.createIntermediate(s,*wp),"bad intermediate type (after task change)",0);

  bfact.remove(1);
  ok (bfact.validate(),"ok with one tp",0);

  bfact.remove(1);
  ok (bfact.validate(),"ok with zero tps (just start and finish)",0);

  ok (bfact.remove(task_manager.task_size()-1,false),"remove finish manually",0);
  ok (!bfact.validate(),"aat is invalid (no finish)",0);

  return true;
}
