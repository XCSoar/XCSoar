#ifndef TASK_DIJKSTRA_HPP
#define TASK_DIJKSTRA_HPP

#include "Util.h"
#include "Dijkstra.hpp"
#include "SearchPoint.hpp"
#include <stdio.h>

class OrderedTask;

class TaskDijkstra {
public:
  TaskDijkstra(OrderedTask* _task,
               const double _precision=0.01);
  ~TaskDijkstra();

  double distance_opt(const ScanTaskPoint &start,
                      bool _shortest=false);

  double distance_opt_achieved(const GEOPOINT &currentLocation,
                               bool _shortest=false);
private:

  void add_edges(Dijkstra<ScanTaskPoint> &dijkstra,
                 const ScanTaskPoint &curNode);
  double distance(const ScanTaskPoint &sp,
                  const GEOPOINT &loc);

  unsigned num_taskpoints;

  /**
   * @clientCardinality 1
   * @supplierCardinality 0..* 
   */
  SEARCH_POINT *solution;

  double precision;

  bool shortest;

  /**
   * @supplierCardinality 1 
   */
  OrderedTask* task;
};

#endif
