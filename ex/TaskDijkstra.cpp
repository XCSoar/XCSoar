#include "TaskDijkstra.hpp"
#include "OrderedTaskPoint.hpp"
#include <stdio.h>

// Example usage (nodes and edges are represented with ints)

#define MAX_DIST 100000

/*
 TODO: only use actual points (if any) for search on previous
*/

void TaskDijkstra::add_edges(Dijkstra<ScanTaskPoint> &dijkstra,
                             const ScanTaskPoint& curNode) 
{
  ScanTaskPoint destination;
  destination.first = curNode.first+1;

  unsigned n_destinations = tps[destination.first]->
    get_num_search_points();

  for (destination.second=0; 
       destination.second< n_destinations; destination.second++) {

    GEOPOINT p1 = tps[curNode.first]->
      get_search_point(curNode.second).Location;
    GEOPOINT p2 = tps[destination.first]->
      get_search_point(destination.second).Location;

    double dr = Distance(p1,p2);
    if (dr>precision) {
      unsigned d;
      if (shortest) {
        d = (dr/precision+0.5);
      } else {
        d = (MAX_DIST-dr/precision+0.5);
      }
      dijkstra.link(destination, d);
    }
  }
}

double TaskDijkstra::distance_opt(ScanTaskPoint start,
                                  const bool req_shortest)
{
  shortest = req_shortest;

  ScanTaskPoint lastNode = start;
  Dijkstra<ScanTaskPoint> dijkstra(start);
  double d_last=0.0;
  solution[start.first] = start.second;

  while (!dijkstra.empty()) {

    ScanTaskPoint curNode = dijkstra.pop();

    if (curNode.first != lastNode.first) {

      double d;
      if (shortest) {
        d= dijkstra.dist()*precision;
      } else {
        d= (MAX_DIST
            *(curNode.first-start.first)-dijkstra.dist())*precision;
      }
      d_last = d;

      solution[curNode.first] = curNode.second;

      if (curNode.first == num_taskpoints-1) {
        return d;
      }
    }
    lastNode = curNode;

    add_edges(dijkstra, curNode);
  }

  return -1; // No path found
}

/*
  only scan parts that are required, and prune out points
  that become irrelevant (strictly under-performing) 

  if in sector, prune out all default points that result in
  lower distance than current achieved max

  if searching min 
    first search max actual up to current
      (taking into account aircraft location?)
    then search min after that from aircraft location
*/

double TaskDijkstra::distance(const ScanTaskPoint &curNode,
                              const GEOPOINT &currentLocation)
{
  return Distance(tps[curNode.first]->
                  get_search_point(curNode.second).Location,
                  currentLocation);
}


double 
TaskDijkstra::distance_opt_achieved(OrderedTaskPoint* active,
                                    const GEOPOINT &currentLocation,
                                    const bool req_shortest)
{
  shortest = false; // internally

  ScanTaskPoint start(0,0);
  ScanTaskPoint lastNode = start;
  Dijkstra<ScanTaskPoint> dijkstra(start);

  int activeStage = 2; // TODO hardcoded!

  double min_d = MAX_DIST;
  double max_d = 0;
  double min_d_actual = MAX_DIST;
  double max_d_actual = 0;

  while (!dijkstra.empty()) {

    ScanTaskPoint curNode = dijkstra.pop();

    if (curNode.first != lastNode.first) {
      solution[curNode.first] = curNode.second;
    }

    if (curNode.first != activeStage) {
      add_edges(dijkstra, curNode);
    } else {
      tps[curNode.first]->set_index_min(curNode.second);

      TaskDijkstra inner_dijkstra(tps, num_taskpoints);

      double d_this = 
        distance(curNode, currentLocation);
      double df = 
        inner_dijkstra.distance_opt(curNode, req_shortest)
        +(MAX_DIST*curNode.first-dijkstra.dist())*precision;

      bool best=false;
      if (req_shortest) {
        if (df+d_this<min_d) {
          min_d = df+d_this; min_d_actual = df;
          best=true;
        }
      } else {
        if (df+d_this>max_d) {
          max_d = df+d_this; max_d_actual = df;
          best=true;
        }
      }
      if (best) {
        for (int j=activeStage; j<num_taskpoints; j++) {
          solution[j]= inner_dijkstra.solution[j];
        }
      }
    }
    lastNode = curNode;

  }

  for (unsigned j=0; j<num_taskpoints; j++) {
    if (req_shortest) {
      tps[j]->set_index_min(solution[j]);
    } else {
      tps[j]->set_index_max(solution[j]);
    }
  }
  for (unsigned j=0; j<num_taskpoints; j++) {
    GEOPOINT loc = tps[j]->
      get_search_point(solution[j]).Location;
    printf("%g %g %d\n", loc.Longitude, loc.Latitude, j);
  }
  printf("\n");
  printf("%g %g\n\n", currentLocation.Longitude,
         currentLocation.Latitude);

  if (req_shortest) {
    return min_d_actual; 
  } else {
    return max_d_actual;
  }
}


// save solution as best is found
