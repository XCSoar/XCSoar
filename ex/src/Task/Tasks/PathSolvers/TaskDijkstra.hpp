#ifndef TASK_DIJKSTRA_HPP
#define TASK_DIJKSTRA_HPP

#include "Navigation/SearchPointVector.hpp"

class OrderedTask;
typedef std::pair<unsigned, unsigned> ScanTaskPoint;
template <class Node> class Dijkstra;
typedef Dijkstra<ScanTaskPoint> DijkstraTaskPoint;

/**
 * Class used to scan an OrderedTask for maximum/minimum distance
 * points.
 *
 * Uses flat-projected integer representation of search points for
 * speed, but this also makes the system approximate.
 *
 * Search points are located on OZ boundaries and each form a convex
 * hull, as this produces the minimum search vector size without loss
 * of accuracy.
 *
 * Searches are sensitive to active task point, in that task points
 * before the active task point need only be searched for maximum achieved
 * distance rather than border search points. 
 *
 * This uses a Dijkstra search and so is O(N log(N)).
 */
class TaskDijkstra 
{
public:
  TaskDijkstra(OrderedTask* _task, unsigned task_size);
  ~TaskDijkstra();

/** 
 * Search task points for targets within OZs to produce the
 * maximum-distance task.  Saves the max-distance solution 
 * in the corresponding task points for later accurate distance
 * measurement.
 * 
 * @return Approximate flat-earth distance of maximum task
 */  
  unsigned distance_max();

/** 
 * Search task points for targets within OZs to produce the
 * minimum-distance task.  Saves the minimum-distance solution 
 * in the corresponding task points for later accurate distance
 * measurement.
 * 
 * Note that the minimum distance task is the minimum distance
 * remaining and is therefore sensitive to the specified aircraft
 * location.
 *
 * @param location Location of aircraft
 * @return Approximate flat-earth distance of minimum task
 */  
  unsigned distance_min(const SearchPoint& location);

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
