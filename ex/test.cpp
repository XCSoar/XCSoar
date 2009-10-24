#include <stdlib.h>
#include <assert.h>
#include "GlideSolvers/MacCready.hpp"
#include <math.h>
#include <stdio.h>
#include "Math/FastMath.h"
#include "Math/Earth.hpp"

int n_samples = 0;


extern long count_mc;
//extern int count_distance;

void distance_counts() {
//  printf("#     distance queries %d\n",count_distance/n_samples); 
  printf("#     mc calcs %d\n",count_mc/n_samples);
  printf("# num samples %d\n",n_samples);
}

#include "Tasks/TaskManager.h"
#include "Tasks/TaskEvents.hpp"

#include "BaseTask/ConvexHull/GrahamScan.hpp"

double small_rand() {
  return rand()*0.001/RAND_MAX;
}


////////////////////////////////////////////////
/*
#include "ConvexHull/GrahamScan.hpp"
#include "ConvexHull/PolygonInterior.hpp"
#include <math.h>

std::vector<SearchPoint> sampled_points;


void test_polygon() 
{
  for (double t=0; t<1.0; t+= 0.1) {
    SearchPoint sp;
    double ang = t*2.0*3.1415926;
    sp.Location.Longitude = cos(ang);
    sp.Location.Latitude = sin(ang);
    sp.actual = true;
    sp.saved_rank = 0;
    sampled_points.push_back(sp);
  }

  GrahamScan gs(sampled_points);
  sampled_points = gs.prune_interior();

  GEOPOINT location;
  location.Longitude = 0.4;
  location.Latitude = 1.0;
  if (PolygonInterior(location, sampled_points)) {
    printf("inside\n");
  } else {
    printf("outside\n");
  }
  location.Longitude = 0.4;
  location.Latitude = 0.0;

  if (PolygonInterior(location, sampled_points)) {
    printf("inside\n");
  } else {
    printf("outside\n");
  }

}
*/
////////////////////////////////////////////////

char wait_prompt(const double time) {
  printf("# %g [enter to continue]\n",time);
//  return getchar();
  return 0;
}

int main() {
  ::InitSineTable();
//  test_mc();

  TaskEvents default_events;
  TaskManager test_task(default_events);

  AIRCRAFT_STATE state, state_last;
  state.Location.Longitude=0.8;
  state.Location.Latitude=1.1;  
  state.Altitude = 1500.0;
  state.Time = 0.0;
  state.WindSpeed = 0.0;
  state.WindDirection = 0;

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

  state.Location = w[0];
  state_last.Location = w[0];
  test_task.report(state);

  unsigned counter=0;

  for (int i=0; i<num_wp-1-1; i++) {
    wait_prompt(state.Time);
    for (double t=0; t<1.0; t+= 0.0025) {
      state.Location.Latitude = 
        w[i].Latitude*(1.0-t)+w[i+1].Latitude*t+small_rand();
      state.Location.Longitude = 
        w[i].Longitude*(1.0-t)+w[i+1].Longitude*t+small_rand();

      double d = ::Distance(state.Location, state_last.Location);
      double V = 15.0;
      state.Time += d/V;

      test_task.update(state, state_last);

      test_task.update_idle(state);

      if (counter++ % 10==0) {
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
