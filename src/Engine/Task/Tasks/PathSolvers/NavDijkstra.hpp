#ifndef NAV_DIJKSTRA_HPP
#define NAV_DIJKSTRA_HPP

#include <vector>
#include "Util/NonCopyable.hpp"
#include "Navigation/SearchPointVector.hpp"

typedef std::pair<unsigned, unsigned> ScanTaskPoint;
bool operator == (const ScanTaskPoint &p1, const ScanTaskPoint &p2);

template <class Node> class Dijkstra;
typedef Dijkstra<ScanTaskPoint> DijkstraTaskPoint;


/**
 * Abstract class for Dijsktra searches of nav points
 */
class NavDijkstra: 
  private NonCopyable 
{
public:
  NavDijkstra(const unsigned _num_stages);

  virtual ~NavDijkstra() {};

/** 
 * Test whether two points (as previous search locations) are significantly
 * different to warrant a new search
 * 
 * @param a1 First point to compare
 * @param a2 Second point to compare
 * 
 * @return True if distance is significant
 */
  static bool distance_is_significant(const SearchPoint& a1,
                                      const SearchPoint& a2);

protected:

  virtual bool finish_satisfied() const;
  virtual const SearchPoint &get_point(const ScanTaskPoint &sp) const = 0;
  virtual void add_edges(DijkstraTaskPoint &dijkstra,
                         const ScanTaskPoint &curNode) = 0;
  
  unsigned distance_general(DijkstraTaskPoint &dijkstra);

  bool shortest;
  unsigned num_stages;
  SearchPointVector solution;

  unsigned distance(const ScanTaskPoint &sp,
                    const SearchPoint &loc) const;

  unsigned distance(const ScanTaskPoint &sp1,
                    const ScanTaskPoint &sp2) const;

  void find_solution(const DijkstraTaskPoint &dijkstra, 
                     const ScanTaskPoint curNode);

private:

  unsigned extremal_distance(const unsigned d) const;

};

#endif
