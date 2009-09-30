#include <stdlib.h>
#include <assert.h>

#include <stdio.h>

extern int count_distance;
void distance_counts() {
  printf("#     distance queries %d\n",count_distance); count_distance=0;
}

#include "Task.h"

#include "ConvexHull/GrahamScan.hpp"

double small_rand() {
  return rand()*0.3/RAND_MAX;
}

int n_samples = 0;

void do_exit()
{
  if (n_samples) {
    printf("av distance tests %d\n", count_distance/n_samples);
  }
  exit(0);
}


////////////////////////////////////////////////
#include "ConvexHull/GrahamScan.hpp"
#include "ConvexHull/PolygonInterior.hpp"
#include <math.h>

std::vector<SEARCH_POINT> sampled_points;


void test_polygon() 
{
  for (double t=0; t<1.0; t+= 0.1) {
    SEARCH_POINT sp;
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

////////////////////////////////////////////////

int main() {

  test_polygon();

  Task test_task;

  GEOPOINT location, location_last;
  location.Longitude=8;
  location.Latitude=11;  

  test_task.setActiveTaskPoint(0);

#define  num_wp 5
  GEOPOINT w[num_wp];
  w[0].Longitude = -0.25; 
  w[0].Latitude = -0.25; 
  w[1].Longitude = -0.5; 
  w[1].Latitude = 10.5; 
  w[2].Longitude = 10.5; 
  w[2].Latitude = 10.5; 
  w[3].Longitude = 7.5; 
  w[3].Latitude = 4.5; 
  w[4].Longitude = 9; 
  w[4].Latitude = 1; 

  location_last = w[0];

  double t;
  for (int i=0; i<num_wp-1-1; i++) {
    for (double t=0; t<1.0; t+= 0.01) {
      location.Latitude = w[i].Latitude*(1.0-t)+w[i+1].Latitude*t+small_rand();
      location.Longitude = w[i].Longitude*(1.0-t)+w[i+1].Longitude*t+small_rand();

      test_task.update_sample(location, location_last);
      test_task.report(location);
      n_samples++;
      location_last = location;
      if (location.Longitude>10.5) do_exit();
    }
    printf("[enter to continue]\n");
    char c = getchar();
  }

//  test_task.remove(2);
//  test_task.scan_distance(location);
  do_exit();
  return 0;
}
