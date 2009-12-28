#ifndef NAV_DIJKSTRA_HPP
#define NAV_DIJKSTRA_HPP

#include <vector>
#include "Util/NonCopyable.hpp"
#include "Dijkstra.hpp"

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
    num_stages(_num_stages),
    shortest(false)
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
                                      const T& a2) {
    return a1.flat_distance(a2)>1;
  }

protected:

  virtual bool finish_satisfied() const {
    return true;
  }

  virtual const T &get_point(const ScanTaskPoint &sp) const = 0;

  virtual void add_edges(DijkstraTaskPoint &dijkstra,
                         const ScanTaskPoint &curNode) = 0;
  
  unsigned distance_general(DijkstraTaskPoint &dijkstra) {
    unsigned lastStage = 0-1;
    while (!dijkstra.empty()) {
      
      const ScanTaskPoint destination = dijkstra.pop();
      
      if (destination.first != lastStage) {
        lastStage = destination.first;
        
        if (destination.first+1 == num_stages) {
          
          find_solution(dijkstra, destination);
          
          if (finish_satisfied()) {
            return extremal_distance(dijkstra.dist());
          }
        }
      }
      add_edges(dijkstra, destination);
    }

    return 0-1; // No path found
  }

  bool shortest;
  unsigned num_stages;
  std::vector<T> solution;

  unsigned distance(const ScanTaskPoint &curNode,
                    const T &currentLocation) const {
    return extremal_distance(get_point(curNode).flat_distance(currentLocation));
  }

  unsigned distance(const ScanTaskPoint &s1,
                    const ScanTaskPoint &s2) const {
    extremal_distance(get_point(s1).flat_distance(get_point(s2)));
  }

  void find_solution(const DijkstraTaskPoint &dijkstra, 
                     const ScanTaskPoint destination) {
    ScanTaskPoint p = destination; 
    ScanTaskPoint p_last;
    do {
      p_last = p;
      solution[p_last.first] = get_point(p_last);
      p = dijkstra.get_predecessor(p_last);
    } while ((p.second != p_last.second) || (p.first != p_last.first));    
  }

private:

  unsigned extremal_distance(const unsigned d) const {
    return shortest? d:-d;
  }

};

#endif
