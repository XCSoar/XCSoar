#include <stdlib.h>
#include <assert.h>
#include <math.h>
#ifdef DO_PRINT
#include <stdio.h>
#endif
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Task/TaskManager.hpp"
#include "Task/TaskEvents.hpp"
#include "Util/Filter.hpp"

#include "Task/Visitors/TaskVisitor.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Airspace/AirspaceVisitor.hpp"
#include "Waypoint/WaypointVisitor.hpp"

#ifdef DO_PRINT
#include <fstream>
#endif

int n_samples = 0;

#ifdef INSTRUMENT_TASK
extern long count_mc;
extern unsigned count_intersections;
extern unsigned n_queries;
extern unsigned count_distbearing;
extern unsigned num_dijkstra;
#endif


void distance_counts() {
  if (n_samples) {
#ifndef NEWTASK
    printf("# Instrumentation\n");
#ifdef INSTRUMENT_TASK
    printf("#     dist+bearing calcs/c %d\n",count_distbearing/n_samples); 
    printf("#     mc calcs/c %d\n",(int)(count_mc/n_samples));
    printf("#     dijkstra/c %d\n",num_dijkstra/n_samples);
    printf("#     intersection tests/q %d\n",count_intersections/n_queries);
    printf("#    (total queries %d)\n\n",n_queries);
#endif
    printf("#    (total cycles %d)\n\n",n_samples);
#endif
  }
  n_samples = 0;
#ifdef INSTRUMENT_TASK
  count_intersections = 0;
  n_queries = 0;
  count_distbearing = 0;
  count_mc = 0;
  num_dijkstra = 0;
#endif
}

Filter heading_filt(8.0);


double small_rand() {
  return heading_filt.update(-40.0+rand()*80.0/RAND_MAX);
}

/** 
 * Wait-for-key prompt
 * 
 * @param time time of simulation
 * 
 * @return character received by keyboard
 */
char wait_prompt(const double time) {
#ifdef DO_PRINT
  printf("# %g [enter to continue]\n",time);
#endif
//  return getchar();
  return 0;
}

Waypoint wp[6];

/** 
 * Initialises waypoints with random and non-random waypoints
 * for testing
 *
 * @param waypoints waypoints class to add waypoints to
 */
void setup_waypoints(Waypoints &waypoints) {
#ifdef DO_PRINT
  std::ofstream fin("results/res-wp-in.txt");
#endif
  for (unsigned i=0; i<5; i++) {
    waypoints.insert(wp[i]);
#ifdef DO_PRINT
    fin << wp[i];
#endif
  }

  for (unsigned i=0; i<150; i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    Waypoint ff; 
    ff.Location.Longitude = x/1000.0; 
    ff.Location.Latitude = y/1000.0;
    ff.id = i+4;
#ifdef DO_PRINT
    fin << ff;
#endif
    waypoints.insert(ff);
  }
  waypoints.optimise();
}


void setup_airspaces(Airspaces& airspaces, TaskProjection& task_projection) {
#ifdef DO_PRINT
  std::ofstream fin("results/res-bb-in.txt");
#endif
  for (unsigned i=0; i<150; i++) {
    AbstractAirspace* as;
    if (rand()%3==0) {
      GEOPOINT c;
      c.Longitude = (rand()%1200-600)/1000.0+0.5;
      c.Latitude = (rand()%1200-600)/1000.0+0.5;
      double radius = 10000.0*(0.2+(rand()%12)/12.0);
      as = new AirspaceCircle(c,radius);
    } else {
      as = new AirspacePolygon(task_projection);
    }
    airspaces.insert(*as);
#ifdef DO_PRINT
    fin << *as;
#endif
  }
  airspaces.optimise();
}


class TaskPointVisitorPrint: public TaskPointVisitor
{
public:
  virtual void Visit(const TaskPoint& tp) {
    printf("got a tp\n");
  }
  virtual void Visit(const OrderedTaskPoint& tp) {
    printf("got an otp\n");
  }
  virtual void Visit(const FinishPoint& tp) {
    printf("got an ftp\n");
  }
  virtual void Visit(const StartPoint& tp) {
    printf("got an stp\n");
  }
};

class TaskVisitorPrint: public TaskVisitor
{
public:
  virtual void Visit(const AbortTask& task) {
    TaskPointVisitorPrint tpv;
    printf("task is abort\n");
    task.Accept(tpv);
  };
  virtual void Visit(const OrderedTask& task) {
    TaskPointVisitorPrint tpv;
    printf("task is ordered\n");
    task.Accept(tpv);
  };
  virtual void Visit(const GotoTask& task) {
    printf("task is goto\n");
  };
};

void setup_task(TaskManager& task_manager)
{
  AbstractTaskFactory *fact;
  OrderedTaskPoint *tp;

  task_manager.set_factory(TaskManager::FACTORY_MIXED);
  fact = task_manager.get_factory();

  tp = fact->createStart(AbstractTaskFactory::START_LINE,wp[0]);
  task_manager.append(tp);

  tp = fact->createIntermediate(AbstractTaskFactory::FAI_SECTOR,wp[1]);
  task_manager.append(tp);

  tp = fact->createIntermediate(AbstractTaskFactory::AAT_CYLINDER,wp[2]);
  task_manager.append(tp);

  tp = fact->createIntermediate(AbstractTaskFactory::AAT_CYLINDER,wp[3]);
  task_manager.append(tp);

  tp = fact->createIntermediate(AbstractTaskFactory::AAT_CYLINDER,wp[4]);
  task_manager.append(tp);

  tp = fact->createFinish(AbstractTaskFactory::FINISH_SECTOR,wp[0]);
  task_manager.append(tp);

  if (task_manager.check_task()) {
    task_manager.reset();
    task_manager.setActiveTaskPoint(0);
    task_manager.resume();
  }
}

class AirspaceVisitorPrint: public AirspaceVisitor {
public:
  AirspaceVisitorPrint(const char* fname,
                       const bool _do_report):
    do_report(_do_report)
    {      
      if (do_report) {
#ifdef DO_PRINT
        fout = new std::ofstream(fname);
#endif
      }
    };
  ~AirspaceVisitorPrint() {
#ifdef DO_PRINT
    if (do_report) {
      delete fout;
    }
#endif    
  }
  virtual void Visit(const AirspaceCircle& as) {
    if (do_report) {
#ifdef DO_PRINT
      *fout << as;
#endif
    }
  }
  virtual void Visit(const AirspacePolygon& as) {
    if (do_report) {
#ifdef DO_PRINT
      *fout << as;
#endif
    }
  }
private:
#ifdef DO_PRINT
  std::ofstream *fout;
#endif
  const bool do_report;
};


void scan_airspaces(const AIRCRAFT_STATE state, 
                    const Airspaces& airspaces,
                    bool do_report,
                    GEOPOINT &target) 
{
  const std::vector<Airspace> vn = airspaces.scan_nearest(state);
  AirspaceVisitorPrint pvn("results/res-bb-nearest.txt",
                           do_report);
  pvn.for_each(vn);

//  std::for_each(vn.begin(), vn.end(), pvn);
// (will work for simple cases where visitor is stateless)

  AirspaceVisitorPrint visitor("results/res-bb-range.txt",
                               do_report);
  airspaces.visit_within_range(state.Location, 5000.0, visitor);

  const std::vector<Airspace> vi = airspaces.find_inside(state);
  AirspaceVisitorPrint pvi("results/res-bb-inside.txt",
                           do_report);
  pvi.for_each(vi);
  
  {
    AirspaceVisitorPrint ivisitor("results/res-bb-intersects.txt",
                                  true);
    GeoVector vec(state.Location, target);
    airspaces.visit_intersecting(state.Location, vec, ivisitor);
  }

}

void test_flight(TaskManager &task_manager,
                 Airspaces &airspaces,
                 GlidePolar &glide_polar,
                 int test_num) 
{
#define  num_wp 6
  GEOPOINT w[num_wp];
  w[0].Longitude = -0.025; 
  w[0].Latitude = -0.125; 
  w[1].Longitude = -0.05; 
  w[1].Latitude = 1.05; 
  w[2].Longitude = 1.05; 
  w[2].Latitude = 1.05; 
  w[3].Longitude = 0.75; 
  w[3].Latitude = 0.5; 
  w[4].Longitude = 0.95; 
  w[4].Latitude = 0; 
  w[5].Longitude = -0.025; 
  w[5].Latitude = 0.0; 

  AIRCRAFT_STATE state, state_last;
  state.Location = w[0];
  state_last.Location = w[0];
  state.Altitude = 1500.0;
  state.Time = 0.0;
  state.WindSpeed = 0.0;
  state.WindDirection = 0;

  if (test_num<4) {
    scan_airspaces(state, airspaces, true, w[1]);
  }

#ifdef DO_PRINT
  std::ofstream f4("results/res-sample.txt");
#endif

  unsigned counter=0;
  TaskVisitorPrint tv;

  enum AcState {
    Climb = 0,
    Cruise,
    FinalGlide
  };
  
  AcState acstate = Cruise;
  double bearing = 0;
  state.Speed = 16.0;

  for (int i=0; i<num_wp-1; i++) {

    if ((test_num==1) && (n_samples>500)) {
      return;
    }
    wait_prompt(state.Time);

    while (w[i+1].distance(state.Location)>state.Speed) {

// get_stats();
      // remaining
      const ElementStat stat = task_manager.get_stats().current_leg;
      double sinkrate = 0.0;

      switch (acstate) {
      case Cruise:
        state.Speed = stat.solution_remaining.VOpt;
        sinkrate = glide_polar.SinkRate(state.Speed);        
        bearing = state.Location.bearing(w[i+1])+small_rand();
        if ((task_manager.get_stats().total.solution_remaining.DistanceToFinal<= state.Speed)
            && (i>1)) {
          printf("fg\n");
          acstate = FinalGlide;
        } else {
          if (state.Altitude<=300) {
            printf("climb\n");
            acstate = Climb;
          }
        }
        break;
      case FinalGlide:
        state.Speed = stat.solution_remaining.VOpt*0.97;
        sinkrate = glide_polar.SinkRate(state.Speed);
        bearing = state.Location.bearing(w[i+1])+small_rand();
        break;
      case Climb:
        state.Speed = 25.0;
        bearing += 20+small_rand();
        sinkrate = -glide_polar.get_mc();
        if ((task_manager.get_stats().total.solution_remaining.DistanceToFinal<= state.Speed)
            && (i>1)) {
          printf("fg\n");
          acstate = FinalGlide;
        } else if (state.Altitude>=1500) {
          acstate = Cruise;
          printf("cruise\n");
        }
        break;
      };

      state.Location = GeoVector(state.Speed,bearing).end_point(state.Location);
      state.Altitude -= sinkrate;
      state.Time += 1.0;

      task_manager.update(state, state_last);
      task_manager.update_idle(state);

      bool do_print = (counter++ % 1 ==0);
      if (test_num<4) {
        scan_airspaces(state, airspaces, do_print, w[i+1]);
      }

      if (do_print) {
//        task_manager.Accept(tv);
#ifdef DO_PRINT
        task_manager.print(state);
        f4 << state.Time << " " 
           <<  state.Location.Longitude << " " 
           <<  state.Location.Latitude << " "
           <<  state.Altitude << "\n";
        f4.flush();
#endif
      }
      n_samples++;
      state_last = state;
    }    

/*
    if (i==num_wp-2) {
      task_manager.abort();
#ifdef DO_PRINT
      printf("- mode abort\n");
#endif
    } 
//    task_manager.Accept(tv);
*/

  }
  distance_counts();
}

class WaypointVisitorPrint: public WaypointVisitor {
public:
  WaypointVisitorPrint():count(0) {};

  virtual void Visit(const Waypoint& wp) {
    printf("visiting wp %d\n", wp.id);
    count++;
  }
  void summary() {
    printf("there are %d found\n",count);
  }
  unsigned count;
};



int test_newtask(int test_num) {
  GEOPOINT start; start.Longitude = 0.5; start.Latitude = 0.5;

  wp[0].id = 0;
  wp[0].Location.Longitude=0;
  wp[0].Location.Latitude=0;
  wp[0].Altitude=0.25;
  wp[1].id = 1;
  wp[1].Location.Longitude=0;
  wp[1].Location.Latitude=1.0;
  wp[1].Altitude=0.25;
  wp[2].id = 2;
  wp[2].Location.Longitude=1.0;
  wp[2].Location.Latitude=1.0;
  wp[2].Altitude=0.5;
  wp[3].id = 3;
  wp[3].Location.Longitude=0.8;
  wp[3].Location.Latitude=0.5;
  wp[3].Altitude=0.25;
  wp[4].id = 4;
  wp[4].Location.Longitude=1.0;
  wp[4].Location.Latitude=0;
  wp[4].Altitude=0.25;

  ////////////////////////////////////////////////////////////////

  TaskEvents default_events;
  GlidePolar glide_polar(2.0,0.0,0.0);

  ////////////////////////// Waypoints //////
  Waypoints waypoints;
  setup_waypoints(waypoints);

  WaypointVisitorPrint v;
  waypoints.visit_within_range(wp[3].Location, 100, v);
  v.summary();

  ////////////////////////// AIRSPACES //////
  TaskProjection task_projection(start);
  Airspaces airspaces(task_projection);
  setup_airspaces(airspaces, task_projection);

  ////////////////////////// TASK //////

  TaskBehaviour task_behaviour;

  distance_counts();

  if (test_num==0) {
    return 0;
  }
  if (test_num==3) {
    task_behaviour.all_off();
  }

  TaskManager task_manager(default_events,
                           task_behaviour,
                           glide_polar,
                           waypoints);
    
  setup_task(task_manager);
    
  test_flight(task_manager, airspaces, glide_polar, test_num);

//  task_manager.remove(2);
//  task_manager.scan_distance(location);
  return 0;
}

#ifndef NEWTASK
int main() {
  ::InitSineTable();
  test_newtask(2);
//  test_newtask(3);
}
#endif


/*
  100, 1604 cycles
  my ipaq: 
  test 1: 27.7 seconds, 277ms/cycle
  test 2: 117 seconds, 72ms/cycle
  test 3: 45 seconds, 28ms/cycle

  test 1  61209: 81 ms/c
  test 2 116266: 72 ms/c
  test 3  46122: 29 ms/c
  test 4 111742: 70 ms/c

*/
