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
  return rand()*0.1/RAND_MAX;
}

int main() {

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

      location_last = location;
    }
    printf("[enter to continue]\n");
    char c = getchar();
    if (i==3) exit(0);
  }

//  test_task.remove(2);
//  test_task.scan_distance(location);

  return 0;
}
