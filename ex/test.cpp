#include <stdlib.h>
#include <assert.h>

#include <stdio.h>

extern int count_distance;
void distance_counts() {
  printf("#     distance queries %d\n",count_distance); count_distance=0;
}

#include "Task.h"

#include "ConvexHull/GrahamScan.hpp"

int main() {

  Task test_task;

  GEOPOINT location;
  location.Longitude=8;
  location.Latitude=8;

  test_task.setActiveTaskPoint(2);
  test_task.scan_distance(location);

//  test_task.remove(2);
//  test_task.scan_distance(location);

  return 0;
}
