#ifndef NAV_DIJKSTRA_HPP
#define NAV_DIJKSTRA_HPP

#include "Util/NonCopyable.hpp"
#include "Dijkstra.hpp"

#include <algorithm>
#include <assert.h>

#ifdef INSTRUMENT_TASK
extern long count_dijkstra_queries;
#endif

/**
 * A reference to a trace/search point: first element is the stage
 * number (turn point number); second element is the index in the
 * #TracePointVector / #SearchPointVector.
 */
typedef std::pair<unsigned, unsigned> ScanTaskPoint;

typedef Dijkstra<ScanTaskPoint> DijkstraTaskPoint;

/**
 * Abstract class for Dijsktra searches of nav points, managing edges in multiple 
 * stages (corresponding to turn points).
 *
 * Expected running time, see http://www.avglab.com/andrew/pub/neci-tr-96-062.ps
 *
 * NavDijkstra<SearchPoint>
 */
template <class T>
class NavDijkstra: 
  private NonCopyable 
{
public:
  enum {
    MAX_STAGES = 16,
  };

public:
  /** 
   * Constructor
   * 
   * @param _num_stages Number of stages in search
   * 
   * @return Initialised object
   */
  NavDijkstra(const unsigned _num_stages):
    num_stages(_num_stages)
  {
    assert(num_stages <= MAX_STAGES);
    std::fill(solution, solution + num_stages, T());
  }

/** 
 * Test whether two points (as previous search locations) are significantly
 * different to warrant a new search
 * 
 * @param a1 First point to compare
 * @param a2 Second point to compare
 * @param dist_threshold Threshold distance for significance
 * 
 * @return True if distance is significant
 */
  static bool distance_is_significant(const T& a1,
                                      const T& a2,
                                      const unsigned dist_threshold=1) {
    return a1.flat_distance(a2)> dist_threshold;
  }

protected:

  /** 
   * Determine whether a finished path is valid
   * 
   * @param sp Point to check
   * 
   * @return True if this terminal point completes a valid solution
   */
  virtual bool finish_satisfied(const ScanTaskPoint &sp) const {
    return true;
  }

  /** 
   * Retrieve point
   * 
   * @param sp Index to point to retrieve
   * 
   * @return Point at index position
   */
  virtual const T &get_point(const ScanTaskPoint &sp) const = 0;

  /** 
   * Add edges from an origin node
   * 
   * @param dijkstra Dijkstra structure to add edges to
   * @param curNode Origin node to add edges from
   */
  virtual void add_edges(DijkstraTaskPoint &dijkstra,
                         const ScanTaskPoint &curNode) = 0;

  /** 
   * Determine whether a point is terminal (no further edges)
   * 
   * @param sp Point to test
   * 
   * @return True if point is terminal
   */
  bool is_final(const ScanTaskPoint &sp) const {
    return sp.first+1== num_stages;
  }

  /** 
   * Determine whether a point is a starting point (no previous edges)
   * 
   * @param sp Point to test
   * 
   * @return True if point is in first layer
   */
  bool is_first(const ScanTaskPoint &sp) const {
    return sp.first== 0;
  }

  /** 
   * Iterate search algorithm
   * 
   * @param dijkstra Dijkstra structure to iterate
   * @param max_steps Maximum number of steps to update
   * 
   * @return True if algorithm returns a terminal path or no path found
   */
  bool distance_general(DijkstraTaskPoint &dijkstra, 
                        unsigned max_steps = 0-1) {

#ifdef INSTRUMENT_TASK
 count_dijkstra_queries++;
#endif

    while (!dijkstra.empty()) {
      
      const ScanTaskPoint destination = dijkstra.pop();
      
      if (is_final(destination)) {
        find_solution(dijkstra, destination);
        if (finish_satisfied(destination)) {
          dijkstra.clear();
          return true;
        }
      } else {
        add_edges(dijkstra, destination);
        if (dijkstra.empty()) return true; // error, no way to reach final
      }

      if (max_steps) {
        --max_steps;
      } else {
        return false; // Reached limit
      }
    }
    return false; // No path found
  }

  /** 
   * Distance function for free point
   * 
   * @param curNode Destination node
   * @param currentLocation Origin location
   * 
   * @return Distance (flat) from origin to destination
   */
  unsigned distance(const ScanTaskPoint &curNode,
                    const T &currentLocation) const {
    return get_point(curNode).flat_distance(currentLocation);
  }

  /** 
   * Distance function for edges
   * 
   * @param s1 Origin node
   * @param s2 Destination node
   * 
   * @return Distance (flat) from origin to destination
   */
  unsigned distance(const ScanTaskPoint &s1,
                    const ScanTaskPoint &s2) const {
    return get_point(s1).flat_distance(get_point(s2));
  }

  /** 
   * Determine optimal solution by backtracing the Dijkstra tree
   * 
   * @param dijkstra Dijkstra structure to retrieve solution from
   * @param destination Terminal point to query
   */
  void find_solution(const DijkstraTaskPoint &dijkstra, 
                     const ScanTaskPoint &destination) {
    ScanTaskPoint p(destination); 
    ScanTaskPoint p_last(p);

    do {
      solution[p.first] = get_point(p);
      p_last = p;
      p = dijkstra.get_predecessor(p);
    } while ((p.second != p_last.second) || (p.first != p_last.first));
  }

  unsigned num_stages; /**< Number of stages in search */
  T solution[MAX_STAGES];
};

#endif
