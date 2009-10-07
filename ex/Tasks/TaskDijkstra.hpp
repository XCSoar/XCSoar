#ifndef TASK_DIJKSTRA_HPP
#define TASK_DIJKSTRA_HPP

#include "Util.h"
#include "Dijkstra.hpp"
#include "BaseTask/SearchPoint.hpp"
#include <stdio.h>
#include <vector>

class OrderedTask;

class TaskDijkstra {
public:
  TaskDijkstra(OrderedTask* _task);
  ~TaskDijkstra();

  unsigned distance_opt(const ScanTaskPoint &start,
                        bool _shortest=false);

  unsigned distance_opt_achieved(const SearchPoint &currentLocation,
                                 bool _shortest=false);
private:

  void add_edges(Dijkstra<ScanTaskPoint> &dijkstra,
                 const ScanTaskPoint &curNode);

  unsigned distance(const ScanTaskPoint &sp,
                    const SearchPoint &loc);

  unsigned distance(const ScanTaskPoint &sp1,
                    const ScanTaskPoint &sp2);

  unsigned num_taskpoints;

  /**
   * @clientCardinality 1
   * @supplierCardinality 0..* 
   */
  std::vector<SearchPoint> solution;

  bool shortest;

  /**
   * @supplierCardinality 1 
   */
  OrderedTask* task;
  const SearchPoint &get_point(const ScanTaskPoint &sp) const;
};

#endif
