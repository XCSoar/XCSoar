#include "Task.h"
#include "TaskDijkstra.hpp"
#include "OrderedTaskPoint.hpp"
#include <stdio.h>

// Example usage (nodes and edges are represented with ints)

#define MAX_DIST 100000


TaskDijkstra::TaskDijkstra(Task* _task,
                           const double _precision):
  task(_task),
  shortest(false),
  precision(_precision) 
{
  num_taskpoints = task->tps.size();
  solution = new SEARCH_POINT[num_taskpoints];
};

TaskDijkstra::~TaskDijkstra() {
  delete [] solution;
};

/*
 TODO: only use actual points (if any) for search on previous
*/


void TaskDijkstra::add_edges(Dijkstra<ScanTaskPoint> &dijkstra,
                             const ScanTaskPoint& curNode) 
{
  ScanTaskPoint destination;
  destination.first = curNode.first+1;

  const std::vector<SEARCH_POINT>& dest_list =
    task->tps[destination.first]->get_search_points();

  for (destination.second=0; 
       destination.second< dest_list.size(); destination.second++) {

    GEOPOINT p1 = task->tps[curNode.first]->
      get_search_points()[curNode.second].Location;
    GEOPOINT p2 = dest_list[destination.second].Location;

    double dr = ::Distance(p1,p2);
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

double TaskDijkstra::distance_opt(const ScanTaskPoint &start,
                                  const bool req_shortest)
{
  shortest = req_shortest;

  ScanTaskPoint lastNode = start;
  Dijkstra<ScanTaskPoint> dijkstra(start);
  double d_last=0.0;

  solution[start.first] = task->tps[start.first]->
    get_search_points()[start.second];

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

      solution[curNode.first] = task->tps[curNode.first]->
        get_search_points()[curNode.second];

      if (curNode.first == num_taskpoints-1) {
        return d;
      }
    }
    lastNode = curNode;

    add_edges(dijkstra, curNode);
  }

  return -1; // No path found
}


double TaskDijkstra::distance(const ScanTaskPoint &curNode,
                              const GEOPOINT &currentLocation)
{
  return ::Distance(task->tps[curNode.first]->
                  get_search_points()[curNode.second].Location,
                  currentLocation);
}


double 
TaskDijkstra::distance_opt_achieved(const GEOPOINT &currentLocation,
                                    const bool req_shortest)
{
  shortest = false; // internally

  ScanTaskPoint start(0,0);
  ScanTaskPoint lastNode(1000,1000);
  Dijkstra<ScanTaskPoint> dijkstra(start);

  const int activeStage = task->getActiveTaskPointIndex();

  double min_d = MAX_DIST;
  double min_d_actual = MAX_DIST;
  double max_d_actual = 0;

  while (!dijkstra.empty()) {

    ScanTaskPoint curNode = dijkstra.pop();

    if (curNode.first != lastNode.first) {
      solution[curNode.first] = task->tps[curNode.first]->
        get_search_points()[curNode.second];
      lastNode = curNode;
    }

    if (curNode.first != activeStage) {
      add_edges(dijkstra, curNode);
    } else {

      double d_acc = (MAX_DIST*activeStage-dijkstra.dist())*precision;
      double d_remaining = 0;
      TaskDijkstra inner_dijkstra(task);

      if (curNode.first == num_taskpoints-1) {
        d_remaining = 0.0;
      } else {
        d_remaining = inner_dijkstra.distance_opt(curNode, req_shortest);
      }

      bool best=false;
      if (req_shortest) {
        // need to take into account distance from here to target

        double d_this = ::Distance(task->tps[curNode.first]->
                                   get_search_points()[curNode.second].Location,
                                   currentLocation);

        if (d_remaining+d_this<min_d) {
          min_d = d_remaining+d_this; 
          min_d_actual = d_remaining+d_acc;
          best=true;
        }
      } else {
        // here we are only interested in scored distance
        if (d_remaining+d_acc>max_d_actual) {
          max_d_actual = d_remaining+d_acc;
          best=true;
        }
      }
      if (best) {
        solution[curNode.first] = task->tps[curNode.first]->
          get_search_points()[curNode.second];

        for (int j=activeStage+1; j<num_taskpoints; j++) {
          solution[j]= inner_dijkstra.solution[j];
        }
      }
    }
  }

  for (unsigned j=0; j<num_taskpoints; j++) {
    if (req_shortest) {
      task->tps[j]->set_search_min(solution[j]);
    } else {
      task->tps[j]->set_search_max(solution[j]);
    }
  }

  if (req_shortest) {
    return min_d_actual; 
  } else {
    return max_d_actual;
  }
}


/*
  TODO/incomplete 

  only scan parts that are required, and prune out points
  that become irrelevant (strictly under-performing) 

  if in sector, prune out all default points that result in
  lower distance than current achieved max

  if searching min 
    first search max actual up to current
      (taking into account aircraft location?)
    then search min after that from aircraft location

  also update saved rank for potential pruning operations

*/
