#include "Math/FastMath.h"
#include "harness_task.hpp"
#include "harness_waypoints.hpp"
#include "test_debug.hpp"

#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"


#include "Task/Visitors/TaskPointVisitor.hpp"



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
  virtual void Visit(const TaskPoint& tp) {
  }
};

class SafeTaskEdit: 
  public TaskPointVisitor 
  // also could make this an ObservationZoneVisitor
{
public:
  SafeTaskEdit(TaskManager& tm, const Waypoints &_waypoints):
    task_mgr(tm),
    waypoints(_waypoints),
    tp_to_edit(0),
    counter(0)
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
      return true; /* don't actually do anything */
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
  virtual void Visit(const TaskPoint& tp) {
    if (counter==tp_to_edit) {
      // ??? unknown type, this shouldn't happen
//        new_tp = new ASTPoint(tp);
    }
    counter++;
  }

  unsigned counter;
  TaskManager &task_mgr;
  unsigned tp_to_edit;
  OrderedTaskPoint *new_tp;
  AbstractTaskFactory *fact;
  const Waypoints &waypoints;
};


int main(int argc, char** argv) {
  ::InitSineTable();

  // default arguments
  verbose=1;  
  
  if (!parse_args(argc,argv)) {
    return 0;
  }

  TaskBehaviour task_behaviour;
  TaskEvents default_events;
  GlidePolar glide_polar(2.0,0.0,0.0);

  Waypoints waypoints;
  setup_waypoints(waypoints);

  TaskManager task_manager(default_events,
                           task_behaviour,
                           glide_polar,
                           waypoints);
  test_task(task_manager, waypoints, 0);

  plan_tests(task_manager.task_size());

  // here goes, example, edit all task points
  SafeTaskEdit ste(task_manager, waypoints);
  for (unsigned i=0; i<task_manager.task_size(); i++) {
    ok(ste.edit(i),"edit tp",0);
    task_report(task_manager, "edit tp\n");
  }

  return exit_status();
}

