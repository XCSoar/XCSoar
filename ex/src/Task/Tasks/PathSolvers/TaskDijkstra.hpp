#ifndef TASK_DIJKSTRA_HPP
#define TASK_DIJKSTRA_HPP

#include "Navigation/SearchPointVector.hpp"

class OrderedTask;
typedef std::pair<unsigned, unsigned> ScanTaskPoint;
template <class Node> class Dijkstra;
typedef Dijkstra<ScanTaskPoint> DijkstraTaskPoint;

/**
 * Class used to scan an OrderedTask for maximum/minimum distance
 * points, sensitive to active task point.
 *
 * Uses flat-projected integer representation of search points for
 * speed, but this also makes the system approximate.
 *
 * This uses a Dijkstra search and so is O(N log(N)).
 */
class TaskDijkstra 
{
public:
  TaskDijkstra(OrderedTask* _task, unsigned task_size);
  ~TaskDijkstra();

/** 
 * Search 
 * 
 * 
 * @return Approximate flat-earth distance of maximum task
 */  unsigned distance_max();

  unsigned distance_min(const SearchPoint &currentLocation);

private:

  unsigned distance_general(DijkstraTaskPoint &dijkstra);

  void add_edges(DijkstraTaskPoint &dijkstra,
                 const ScanTaskPoint &curNode);

  void add_start_edges(DijkstraTaskPoint &dijkstra,
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

  /** @link dependency
   *@associates <{Dijkstra}> */
  /*# int lnkDijkstra; */

  const SearchPoint &get_point(const ScanTaskPoint &sp) const;
  unsigned get_size(unsigned stage) const;
};

#endif
