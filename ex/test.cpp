#include <stdlib.h>
#include <assert.h>

#include "TaskPoints/FAISectorStartPoint.hpp"
#include "TaskPoints/FAISectorASTPoint.hpp"
#include "TaskPoints/FAISectorFinishPoint.hpp"
#include "TaskLeg.h"

#include <stdio.h>
#include "TaskDijkstra.hpp"

#include <stdio.h>

extern int count_distance;
void distance_counts() {
  printf("#     distance queries %d\n",count_distance); count_distance=0;
}

int main() {

  WAYPOINT wp[6];
  wp[0].Location.Longitude=0;
  wp[0].Location.Latitude=0;
  wp[1].Location.Longitude=0;
  wp[1].Location.Latitude=10;
  wp[2].Location.Longitude=10;
  wp[2].Location.Latitude=10;
  wp[3].Location.Longitude=10;
  wp[3].Location.Latitude=0;
  wp[4].Location.Longitude=20;
  wp[4].Location.Latitude=0;
  
  GEOPOINT me;
  me.Longitude=8;
  me.Latitude=8;

  FAISectorStartPoint ts(wp[0]);
  FAISectorASTPoint tp1(wp[1]);
  FAISectorASTPoint tp2(wp[2]);
  FAISectorASTPoint tp3(wp[3]);
  FAISectorFinishPoint tf(wp[4]);


  TaskLeg leg0(ts,tp1);
  TaskLeg leg1(tp1,tp2);
  TaskLeg leg2(tp2,tp3);
  TaskLeg leg3(tp3,tf);

  ts.update_geometry();
  tp1.update_geometry();
  tp2.update_geometry();
  tp3.update_geometry();
  tf.update_geometry();

  ts.default_search_points();
  tp1.default_search_points();
  tp2.default_search_points();
  tp3.default_search_points();
  tf.default_search_points();

  count_distance = 0;

  double d= ts.scan_distance_nominal();
  printf("# dist nominal %g\n", d);
  distance_counts();

  ts.scan_active(&tp2);
  d = ts.scan_distance_remaining(me);
  printf("# dist remaining %g\n", d);
  distance_counts();

  d = ts.scan_distance_travelled(me);
  printf("# dist travelled %g\n", d);
  distance_counts();

  d = ts.scan_distance_scored(me);
  printf("# dist scored %g\n", d);
  distance_counts();

  OrderedTaskPoint* tps[5];
  tps[0]= &ts;
  tps[1]= &tp1;
  tps[2]= &tp2;
  tps[3]= &tp3;
  tps[4]= &tf;

  printf("# Dijkstra search\n\n");
  TaskDijkstra task_dijkstra(tps, 5);
  ScanTaskPoint start(0,0);

  d = task_dijkstra.distance_opt(start,true);
  printf("# min dist %g\n",d);
  distance_counts();

  d = task_dijkstra.distance_opt(start,false);
  printf("# max dist %g\n",d);
  distance_counts();

  ts.scan_active(&tp2);

  d = task_dijkstra.distance_opt_achieved(&tp2, me, true);  
  printf("# min dist after achieving max %g\n",d);
  distance_counts();

  d = task_dijkstra.distance_opt_achieved(&tp2, me, false);
  printf("# max dist after achieving max %g\n",d);
  distance_counts();

  return 0;
}


