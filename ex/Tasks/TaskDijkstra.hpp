#ifndef TASK_DIJKSTRA_HPP
#define TASK_DIJKSTRA_HPP

#include "Util.h"
#include "Dijkstra.hpp"
#include "SearchPoint.hpp"
#include <stdio.h>
#include <vector>

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

  double distance(const ScanTaskPoint &sp1,
                  const ScanTaskPoint &sp2);

  unsigned num_taskpoints;

  /**
   * @clientCardinality 1
   * @supplierCardinality 0..* 
   */
  std::vector<SearchPoint> solution;

  double precision;

  bool shortest;

  /**
   * @supplierCardinality 1 
   */
  OrderedTask* task;
  const SearchPoint &get_point(const ScanTaskPoint &sp) const;
};

#endif
