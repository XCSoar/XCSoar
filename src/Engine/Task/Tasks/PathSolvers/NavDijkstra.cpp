#include "NavDijkstra.hpp"
#include "Dijkstra.hpp"
#include <algorithm>

#ifdef INSTRUMENT_TASK
unsigned num_dijkstra = 0;
#endif


bool operator == (const ScanTaskPoint &p1, const ScanTaskPoint &p2) 
{
  return (p1.second == p2.second) && (p1.first == p2.first);
}

NavDijkstra::NavDijkstra(const unsigned _num_stages):
  num_stages(_num_stages),
  shortest(false)
{
  solution.reserve(num_stages);
}


unsigned 
NavDijkstra::distance_general(DijkstraTaskPoint &dijkstra)
{
  unsigned lastStage = 0-1;
  while (!dijkstra.empty()) {

    const ScanTaskPoint curNode = dijkstra.pop();

    if (curNode.first != lastStage) {
      lastStage = curNode.first;

      if (curNode.first+1 == num_stages) {

        find_solution(dijkstra, curNode);

        if (finish_satisfied()) {
          return extremal_distance(dijkstra.dist());
        }
      }
    }
    add_edges(dijkstra, curNode);
  }

  return 0-1; // No path found
}


bool 
NavDijkstra::distance_is_significant(const SearchPoint& a1,
                                     const SearchPoint& a2)
{
  return a1.flat_distance(a2)>1;
}


unsigned 
NavDijkstra::distance(const ScanTaskPoint &curNode,
                      const SearchPoint &currentLocation) const
{
#ifdef INSTRUMENT_TASK
  num_dijkstra++;
#endif
  return extremal_distance(get_point(curNode).flat_distance(currentLocation));
}

unsigned 
NavDijkstra::distance(const ScanTaskPoint &s1,
                      const ScanTaskPoint &s2) const
{
#ifdef INSTRUMENT_TASK
  num_dijkstra++;
#endif
  return extremal_distance(get_point(s1).flat_distance(get_point(s2)));
}


unsigned 
NavDijkstra::extremal_distance(const unsigned d) const
{
  if (shortest) {
    return d;
  } else {
    return -d;
  }
}

bool
NavDijkstra::finish_satisfied() const
{
  return true;
}

void
NavDijkstra::find_solution(const DijkstraTaskPoint &dijkstra, 
                           const ScanTaskPoint curNode)
{
  ScanTaskPoint p = curNode; 
  ScanTaskPoint p_last;
  do {
    p_last = p;
    solution[p_last.first] = get_point(p_last);
    p = dijkstra.get_predecessor(p_last);
  } while (!(p == p_last));
}

