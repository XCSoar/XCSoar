
#include "OLCFAI.hpp"

/*
 @todo potential to use 3d convex hull to speed search
*/

OLCFAI::OLCFAI(const TracePointVector &_trace):
  ContestDijkstra(_trace, 3, 1000) {}

bool 
OLCFAI::finish_satisfied(const ScanTaskPoint &sp) const
{
  if (!ContestDijkstra::finish_satisfied(sp))
    return false;

  // if d<500km, shortest leg >= 28% d; else shortest leg >= 25%d
  fixed dist = fixed_zero;
  fixed shortest_leg = fixed_minus_one;
  for (unsigned i = 0; i + 1 < num_stages; i++) {
    fixed leg = solution[i].get_location().distance(solution[i+1].get_location());
    if (leg <= fixed_zero)
      break;

    dist += leg;

    if (shortest_leg < fixed_zero)
      shortest_leg = leg;
    else if (shortest_leg > leg)
      shortest_leg = leg;
  }

  shortest_leg = shortest_leg / dist;
  if (dist > fixed(500000))
    return (shortest_leg > fixed(0.25));
  else
    return (shortest_leg > fixed(0.28));
}

bool 
OLCFAI::admit_candidate(const ScanTaskPoint &candidate) const
{
  /// \todo implement check for closure
  /// (end point is within 1km of start)
  return ContestDijkstra::admit_candidate(candidate);
}

void 
OLCFAI::add_edges(DijkstraTaskPoint &dijkstra, const ScanTaskPoint& origin)
{
  ScanTaskPoint destination(origin.first+1, origin.second);

  if (!is_final(destination)) {
    ContestDijkstra::add_edges(dijkstra, origin);
    return;
  }

  find_solution(dijkstra, origin);

  const FlatGeoPoint prev = get_point(origin).get_flatLocation();
  const FlatGeoPoint v_close = solution[0].get_flatLocation() - prev;
  
  for (; destination.second != n_points; ++destination.second) {
    if (admit_candidate(destination)) {
      const FlatGeoPoint v_this = 
          get_point(destination).get_flatLocation() - prev;

      const unsigned d = get_weighting(origin.first) *
                         v_this.projected_distance(v_close);

      dijkstra.link(destination, origin, d);
    }
  }
}
