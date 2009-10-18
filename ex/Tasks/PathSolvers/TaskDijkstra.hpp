#ifndef TASK_DIJKSTRA_HPP
#define TASK_DIJKSTRA_HPP

#include "BaseTask/SearchPointVector.hpp"

class OrderedTask;
typedef std::pair<unsigned, unsigned> ScanTaskPoint;
template <class Node> class Dijkstra;

class TaskDijkstra {
public:
  TaskDijkstra(OrderedTask* _task, unsigned task_size);
  ~TaskDijkstra();

  unsigned distance_max();

  unsigned distance_min(const SearchPoint &currentLocation);

private:

  unsigned distance_general(Dijkstra<ScanTaskPoint> &dijkstra);

  void add_edges(Dijkstra<ScanTaskPoint> &dijkstra,
                 const ScanTaskPoint &curNode);

  void add_start_edges(Dijkstra<ScanTaskPoint> &dijkstra,
                 const SearchPoint &loc);

  unsigned distance(const ScanTaskPoint &sp,
                    const SearchPoint &loc) const;

  unsigned distance(const ScanTaskPoint &sp1,
                    const ScanTaskPoint &sp2) const;

  void save_max();
  void save_min();

  const unsigned num_taskpoints;
  unsigned activeStage;

  unsigned extremal_distance(const unsigned d);

  /**
   * @clientCardinality 1
   * @supplierCardinality 0..* 
   */
  SearchPointVector solution;

  bool shortest;

  /**
   * @supplierCardinality 1 
   */
  OrderedTask* task;
  const SearchPoint &get_point(const ScanTaskPoint &sp) const;
};

#endif
