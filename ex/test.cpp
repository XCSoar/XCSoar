#include <stdlib.h>
#include <assert.h>

#include "TaskPoints/FAISectorStartPoint.hpp"
#include "TaskPoints/FAISectorASTPoint.hpp"
#include "TaskPoints/FAISectorFinishPoint.hpp"
#include "TaskLeg.h"

#include <stdio.h>

WAYPOINT wp[5];

int main() {

  wp[0].Location.Longitude=0;
  wp[0].Location.Latitude=0;
  wp[1].Location.Longitude=0;
  wp[1].Location.Latitude=1;
  wp[2].Location.Longitude=0;
  wp[2].Location.Latitude=2;
  wp[3].Location.Longitude=0;
  wp[3].Location.Latitude=3;
  
  GEOPOINT me;
  me.Longitude=0;
  me.Latitude=1.75;

  FAISectorStartPoint ts(wp[0]);
  FAISectorASTPoint tp1(wp[1]);
  FAISectorASTPoint tp2(wp[2]);
  FAISectorFinishPoint tf(wp[3]);

  TaskLeg leg0(ts,tp1);
  TaskLeg leg1(tp1,tp2);
  TaskLeg leg2(tp2,tf);

  ts.update_geometry();
  tp1.update_geometry();
  tp2.update_geometry();
  tf.update_geometry();

  if (ts.scan_active(&ts)) {
    printf("ts found\n");
  } else {
    printf("not found\n");
  }

  if (ts.scan_active(&tp1)) {
    printf("tp1 found\n");
  } else {
    printf("not found\n");
  }

  if (ts.scan_active(&tp2)) {
    printf("tp2 found\n");
  } else {
    printf("not found\n");
  }

  if (ts.scan_active(&tf)) {
    printf("tf found\n");
  } else {
    printf("not found\n");
  }

  double d= ts.scan_distance_nominal();
  printf("dist nominal %g\n\n", d);

  ts.scan_active(&tp2);
  d = ts.scan_distance_remaining(me);
  printf("dist remaining %g\n", d);
  d = ts.scan_distance_travelled(me);
  printf("dist travelled %g\n", d);
  d = ts.scan_distance_scored(me);
  printf("dist scored %g\n", d);

  me.Longitude=0;
  me.Latitude=2.75;
  printf("\n");

  ts.scan_active(&tf);
  d = ts.scan_distance_remaining(me);
  printf("dist remaining %g\n", d);
  d = ts.scan_distance_travelled(me);
  printf("dist travelled %g\n", d);
  d = ts.scan_distance_scored(me);
  printf("dist scored %g\n", d);

  return 0;
}
