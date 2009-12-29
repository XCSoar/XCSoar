#ifndef NAV_DIJKSTRA_HPP
#define NAV_DIJKSTRA_HPP

#include <vector>
#include "Util/NonCopyable.hpp"
#include "Dijkstra.hpp"

#ifdef INSTRUMENT_TASK
extern long count_dijkstra_queries;
#endif

typedef std::pair<unsigned, unsigned> ScanTaskPoint;
typedef Dijkstra<ScanTaskPoint> DijkstraTaskPoint;

/**
 * Abstract class for Dijsktra searches of nav points
 *
 * NavDijkstra<SearchPoint>
 */
template <class T>
class NavDijkstra: 
  private NonCopyable 
{
public:

  NavDijkstra(const unsigned _num_stages):
    num_stages(_num_stages)
    {
      solution.reserve(num_stages);
    }

/** 
 * Test whether two points (as previous search locations) are significantly
 * different to warrant a new search
 * 
 * @param a1 First point to compare
 * @param a2 Second point to compare
 * 
 * @return True if distance is significant
 */
  static bool distance_is_significant(const T& a1,
                                      const T& a2,
                                      const unsigned dist_threshold=1) {
    return a1.flat_distance(a2)> dist_threshold;
  }

protected:

  virtual bool finish_satisfied(const ScanTaskPoint &sp) const {
    return true;
  }

  bool is_final(const ScanTaskPoint &sp) const {
    return sp.first+1== num_stages;
  }

  virtual const T &get_point(const ScanTaskPoint &sp) const = 0;

  virtual void add_edges(DijkstraTaskPoint &dijkstra,
                         const ScanTaskPoint &curNode) = 0;
  
  bool distance_general(DijkstraTaskPoint &dijkstra) {

#ifdef INSTRUMENT_TASK
 count_dijkstra_queries++;
#endif

    while (!dijkstra.empty()) {
      
      const ScanTaskPoint destination = dijkstra.pop();
      
      if (is_final(destination)) {
        find_solution(dijkstra, destination);
        if (finish_satisfied(destination)) {
          return true;
        }
      } else {
        add_edges(dijkstra, destination);
      }
    }
    return false; // No path found
  }

  unsigned num_stages;
  std::vector<T> solution;

  unsigned distance(const ScanTaskPoint &curNode,
                    const T &currentLocation) const {
    return get_point(curNode).flat_distance(currentLocation);
  }

  unsigned distance(const ScanTaskPoint &s1,
                    const ScanTaskPoint &s2) const {
    get_point(s1).flat_distance(get_point(s2));
  }

  void find_solution(const DijkstraTaskPoint &dijkstra, 
                     const ScanTaskPoint destination) {
    ScanTaskPoint p(destination); 
    ScanTaskPoint p_last(p);

    do {
      solution[p.first] = get_point(p);
      p_last = p;
      p = dijkstra.get_predecessor(p);
    } while ((p.second != p_last.second) || (p.first != p_last.first));
  }

private:

};

#endif
