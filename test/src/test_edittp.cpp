/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Math/FastMath.h"
#include "harness_task.hpp"
#include "harness_waypoints.hpp"
#include "test_debug.hpp"
#include "TaskEventsPrint.hpp"

#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"


#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Task/Visitors/ObservationZoneVisitor.hpp"

/*
class TaskPointEdit: public TaskPointVisitor
{
public:
  TaskPointEdit():edited_tp(NULL) {};

  OrderedTaskPoint* edit(OrderedTaskPoint* orig_tp) {
    edited_tp = NULL;
    orig_tp->Accept(*this);
    if (edited_tp) {
      delete orig_tp; // destroy original
      return edited_tp;
    } else {
      return orig_tp;
    }
  }

private:
  OrderedTaskPoint *edited_tp;

  virtual void Visit(const UnorderedTaskPoint& tp) {
  }
  virtual void Visit(const FinishPoint& tp) {
  }
  virtual void Visit(const StartPoint& tp) {
  }
  virtual void Visit(const AATPoint& tp) {
    // if really want to edit, construct edited_tp 
    // otherwise do nothing
    edited_tp = new AATPoint(tp);
  }
  virtual void Visit(const ASTPoint& tp) {
  }
};

class SafeTaskEdit: 
  public TaskPointVisitor 
  // also could make this an ObservationZoneVisitor
{
public:
  SafeTaskEdit(TaskManager& tm, const Waypoints &_waypoints):
    task_mgr(tm),
    counter(0),
    tp_to_edit(0),
    waypoints(_waypoints)
    {
      //task_mgr.lock();
      fact = task_mgr.get_factory();
      //task_mgr.unlock();      
    }

  ~SafeTaskEdit() {
  }
  
  bool edit(const unsigned _tp_to_edit) {
    counter = 0;
    new_tp = NULL;

    tp_to_edit = _tp_to_edit;

    {
      //task_mgr.lock();
      task_mgr.ordered_Accept(*this);
      //task_mgr.unlock();      
    }

    TaskPointEdit tpe;
    
    // finished scanning, now see if we found point to edit
    if (new_tp) {
      // ask for new_tp to be edited (e.g. open dialog and wait for it to close)
      // [editing...]

      new_tp = tpe.edit(new_tp);

      {
        // task_mgr.lock();
        fact->replace(new_tp, tp_to_edit, false);
        // task_mgr.unlock();
      }
      return true;

    } else {
      return true; // don't actually do anything 
      // must be a new point, create one for editing
      // (assuming here have asked if it is a finish or intermediate point,
      // and the waypoint)
      // 
      // [editing...]
      
      const Waypoint* wp = waypoints.lookup_id(3);
      if (wp) {

        {
          // task_mgr.lock();
          new_tp = fact->createFinish(*wp);
          // task_mgr.unlock();
        }
        
        // [editing...]
        new_tp = tpe.edit(new_tp);
        
        {
          // task_mgr.lock();
          fact->append(new_tp, false);
          // task_mgr.unlock();
        }

        return true;
      } else {
        return false;
      }
    }
    return false;
  }
  private:

  void clone_on_index(const OrderedTaskPoint &tp) {
    if (counter==tp_to_edit) {
      // task_mgr.lock();
      new_tp = fact->clone(tp,NULL);
      // task_mgr.unlock();
    }
    counter++;
  }
  virtual void Visit(const UnorderedTaskPoint& tp) {
    if (counter==tp_to_edit) {
      // ??? unknown type, this shouldn't happen
//        new_tp = new ASTPoint(tp);
    }
    counter++;
  }
  virtual void Visit(const FinishPoint& tp) {
    clone_on_index(tp);
  }
  virtual void Visit(const StartPoint& tp) {
    clone_on_index(tp);
  }
  virtual void Visit(const AATPoint& tp) {
    clone_on_index(tp);
  }
  virtual void Visit(const ASTPoint& tp) {
    clone_on_index(tp);
  }

  TaskManager &task_mgr;
  unsigned counter;
  unsigned tp_to_edit;
  OrderedTaskPoint *new_tp;
  AbstractTaskFactory *fact;
  const Waypoints &waypoints;
};
*/

class ObservationZoneEdit: public ObservationZoneVisitor
{
public:
  ObservationZoneEdit() {};

private:
  virtual void Visit(FAISectorZone& tp) {
    // nothing to edit
  }
  virtual void Visit(KeyholeZone& tp) {
    // nothing to edit
  }
  virtual void Visit(SectorZone& tp) {
    // radius, radials
  }
  virtual void Visit(LineSectorZone& tp) {
    // length
    tp.setLength(fixed(1000));
  }
  virtual void Visit(CylinderZone& tp) {
    // radius
    tp.setRadius(fixed(2000));
  }
};


class TaskPointEdit: public TaskPointVisitor
{
public:
  TaskPointEdit() {};

private:
  virtual void Visit(FinishPoint& tp) {
  }
  virtual void Visit(StartPoint& tp) {
    ObservationZoneEdit ozv;
    tp.Accept_oz(ozv);
  }
  virtual void Visit(AATPoint& tp) {
    ObservationZoneEdit ozv;
    tp.Accept_oz(ozv);
  }
  virtual void Visit(ASTPoint& tp) {
  }
};


bool test_edit(TaskManager& task, const TaskBehaviour &task_behaviour) 
{
  TaskEventsPrint edit_events(verbose);
  TaskAdvance task_advance;
  GlidePolar glide_polar = task.get_glide_polar();
  OrderedTask* task_copy = task.clone(edit_events,
                                      task_behaviour, 
                                      task_advance,
                                      glide_polar);


//  task_copy->remove(2);

  TaskPointEdit tpv;
  task_copy->tp_Accept(tpv);

  task.commit(*task_copy);

  task_report(task, "AFTER EDIT\n");

  delete task_copy;
  return true;
}

int main(int argc, char** argv) {
  // default arguments
  verbose=1;  
  
  if (!parse_args(argc,argv)) {
    return 0;
  }

  TaskBehaviour task_behaviour;
  TaskEventsPrint default_events(verbose);
  GlidePolar glide_polar(fixed_two);

  Waypoints waypoints;
  setup_waypoints(waypoints);

  TaskManager task_manager(default_events,
                           task_behaviour,
                           waypoints);
  task_manager.set_glide_polar(glide_polar);
  test_task(task_manager, waypoints, 0);

  plan_tests(1);

  ok(test_edit(task_manager, task_behaviour),
     "edit task", 0);

/*
  plan_tests(task_manager.task_size());

  // here goes, example, edit all task points
  SafeTaskEdit ste(task_manager, waypoints);
  for (unsigned i=0; i<task_manager.task_size(); i++) {
    ok(ste.edit(i),"edit tp",0);
    task_report(task_manager, "edit tp\n");
  }
*/
  return exit_status();
}

