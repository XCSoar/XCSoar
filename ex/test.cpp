#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Navigation/Airspaces.hpp"
#include "Navigation/AirspaceCircle.hpp"
#include "Navigation/AirspacePolygon.hpp"
#include "Navigation/Waypoints.hpp"
#include "Tasks/TaskManager.h"
#include "Tasks/TaskEvents.hpp"
#include "TaskPoints/FAISectorStartPoint.hpp"
#include "TaskPoints/FAISectorFinishPoint.hpp"
#include "TaskPoints/FAISectorASTPoint.hpp"
#include "TaskPoints/FAICylinderASTPoint.hpp"
#include "TaskPoints/CylinderAATPoint.hpp"

#include <fstream>

int n_samples = 0;

extern long count_mc;
extern unsigned count_intersections;
extern unsigned n_queries;
extern unsigned count_distbearing;
extern unsigned num_dijkstra;

void distance_counts() {
  if (n_samples) {
    printf("# Instrumentation\n");
    printf("#     dist+bearing calcs/c %d\n",count_distbearing/n_samples); 
    printf("#     mc calcs/c %d\n",count_mc/n_samples);
    printf("#     dijkstra/c %d\n",num_dijkstra/n_samples);
    printf("#    (total cycles %d)\n\n",n_samples);
    printf("#     intersections tests/q %d\n",count_intersections/n_queries);
    printf("#    (total queries %d)\n\n",n_queries);
  }
  count_intersections = 0;
  n_queries = 0;
  n_samples = 0;
  count_distbearing = 0;
  count_mc = 0;
  num_dijkstra = 0;
}

double small_rand() {
  return rand()*0.001/RAND_MAX;
}


char wait_prompt(const double time) {
  printf("# %g [enter to continue]\n",time);
  return getchar();
//  return 0;
}

WAYPOINT wp[6];

void setup_waypoints(Waypoints &waypoints) {
  for (unsigned i=0; i<5; i++) {
    waypoints.insert(wp[i]);
  }

  for (unsigned i=0; i<150; i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    WAYPOINT ff; 
    ff.Location.Longitude = x/1000.0; 
    ff.Location.Latitude = y/1000.0;
    ff.id = i+4;
    waypoints.insert(ff);
  }
  waypoints.optimise();
}


void setup_airspaces(Airspaces& airspaces, TaskProjection& task_projection) {
  std::ofstream fin("res-bb-in.txt");
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
    as->print(fin, task_projection);
  }
  airspaces.optimise();
}


void setup_task(TaskManager& task_manager,
                TaskProjection &task_projection)
{
  task_manager.append(new FAISectorStartPoint(task_projection,wp[0]));
  task_manager.append(new FAISectorASTPoint(task_projection,wp[1]));
  task_manager.append(new CylinderAATPoint(task_projection,wp[2]));
  task_manager.append(new CylinderAATPoint(task_projection,wp[3]));
  task_manager.append(new CylinderAATPoint(task_projection,wp[4]));
  task_manager.append(new FAISectorFinishPoint(task_projection,wp[0]));

  if (task_manager.check_task()) {
    task_manager.setActiveTaskPoint(0);
    task_manager.resume();
  }
}

void test_flight(TaskManager &task_manager,
                 Airspaces &airspaces) 
{
#define  num_wp 5
  GEOPOINT w[num_wp];
  w[0].Longitude = -0.025; 
  w[0].Latitude = -0.125; 
  w[1].Longitude = -0.05; 
  w[1].Latitude = 1.05; 
  w[2].Longitude = 1.05; 
  w[2].Latitude = 1.05; 
  w[3].Longitude = 0.75; 
  w[3].Latitude = 0.5; 
  w[4].Longitude = 0.9; 
  w[4].Latitude = 0.1; 

  AIRCRAFT_STATE state, state_last;
  state.Location = w[0];
  state_last.Location = w[0];
  state.Altitude = 1500.0;
  state.Time = 0.0;
  state.WindSpeed = 0.0;
  state.WindDirection = 0;

  airspaces.scan_nearest(state, true);
  airspaces.scan_range(state, 5000.0, true);
  airspaces.find_inside(state, true);

  unsigned counter=0;

  for (int i=0; i<num_wp-1; i++) {
    if (i==num_wp-2) {
      task_manager.abort();
      printf("- mode abort\n");
    }
    wait_prompt(state.Time);
    for (double t=0; t<1.0; t+= 0.0025) {
      state.Location.Latitude = 
        w[i].Latitude*(1.0-t)+w[i+1].Latitude*t+small_rand();
      state.Location.Longitude = 
        w[i].Longitude*(1.0-t)+w[i+1].Longitude*t+small_rand();

      double d = ::Distance(state.Location, state_last.Location);
      double V = 19.0;
      state.Time += d/V;

      task_manager.update(state, state_last);

      task_manager.update_idle(state);

      bool do_report = (counter++ % 10 ==0);
      airspaces.scan_range(state, 5000.0, do_report);
      airspaces.find_inside(state, do_report);

      if (do_report) {
        task_manager.report(state);
      }
      n_samples++;
      state_last = state;
    }    
  }
  distance_counts();
}

int main() {
  GEOPOINT start; start.Longitude = 0.5; start.Latitude = 0.5;

  ::InitSineTable();

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
  TaskProjection task_projection(start);

  ////////////////////////// WAYPOINTS //////
  Waypoints waypoints(task_projection);
  setup_waypoints(waypoints);
  task_projection.report();

  ////////////////////////// AIRSPACES //////
  Airspaces airspaces(task_projection);
  setup_airspaces(airspaces, task_projection);

  ////////////////////////// TASK //////

  TaskBehaviour task_behaviour;

  distance_counts();

  for (unsigned i=0; i<2; i++) {
    if (i==1) {
      printf("ALL OFF\n");
      task_behaviour.all_off();
    }

    TaskManager task_manager(default_events,
                             task_behaviour,
                             task_projection,
                             glide_polar,waypoints);
    
    setup_task(task_manager, task_projection);
    
    ////////////////////////// TEST FLIGHT //////
    
    test_flight(task_manager, airspaces);

  }

//  task_manager.remove(2);
//  task_manager.scan_distance(location);
  return 0;
}
