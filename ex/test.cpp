#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Navigation/Airspaces.hpp"
#include "Tasks/TaskManager.h"
#include "Tasks/TaskEvents.hpp"

int n_samples = 0;

extern long count_mc;
//extern int count_distance;

void distance_counts() {
//  printf("#     distance queries %d\n",count_distance/n_samples); 
  printf("#     mc calcs %d\n",count_mc/n_samples);
  printf("# num samples %d\n",n_samples);
}

double small_rand() {
  return rand()*0.001/RAND_MAX;
}


char wait_prompt(const double time) {
  printf("# %g [enter to continue]\n",time);
  return getchar();
//  return 0;
}





int main() {
  ::InitSineTable();

  TaskEvents default_events;
  GlidePolar glide_polar(2.0,0.0,0.0);
  TaskManager test_task(default_events,glide_polar);
  Airspaces airspaces(test_task.get_task_projection());

  test_task.setActiveTaskPoint(0);

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

  test_task.report(state);
  airspaces.scan_nearest(state.Location, true);
  airspaces.scan_range(state.Location, 30, true);

  unsigned counter=0;

  for (int i=0; i<num_wp-1-1; i++) {
    wait_prompt(state.Time);
    for (double t=0; t<1.0; t+= 0.0025) {
      state.Location.Latitude = 
        w[i].Latitude*(1.0-t)+w[i+1].Latitude*t+small_rand();
      state.Location.Longitude = 
        w[i].Longitude*(1.0-t)+w[i+1].Longitude*t+small_rand();

      double d = ::Distance(state.Location, state_last.Location);
      double V = 19.0;
      state.Time += d/V;

      test_task.update(state, state_last);

      test_task.update_idle(state);

      bool do_report = (counter++ % 100 ==0);
      airspaces.scan_nearest(state.Location, do_report);
      airspaces.scan_range(state.Location, 30, do_report);

      if (do_report) {
        test_task.report(state);
      }
      n_samples++;
      state_last = state;
    }    
  }
  distance_counts();

//  test_task.remove(2);
//  test_task.scan_distance(location);
  return 0;
}
