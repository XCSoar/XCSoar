
#include "OLCFAI.hpp"
#include "Navigation/Flat/FlatRay.hpp"

/*
 @todo potential to use 3d convex hull to speed search
*/

OLCFAI::OLCFAI(const TracePointVector &_trace):
  ContestDijkstra(_trace, 3, 1000) {}

bool 
OLCFAI::fai_triangle_satisfied(const ScanTaskPoint &sp) const
{
  // note that this is expensive since it needs accurate distance computations

  // if d<500km, shortest leg >= 28% d; else shortest leg >= 25%d
  fixed dist = fixed_zero;
  fixed shortest_leg = fixed_minus_one;
  for (unsigned i = 0; i + 1 < num_stages; i++) {

    const GeoPoint p_dest = (i + 2 < num_stages)? solution[i+1].get_location(): 
      get_point(sp).get_location();

    const fixed leg = solution[i].get_location().distance(p_dest);

    if (leg <= fixed_zero) // ignore
      break;

    dist += leg;

    if ((shortest_leg > leg) || (shortest_leg < fixed_zero))
      shortest_leg = leg;
  }

  if (dist<= fixed_zero)
    return false;

  shortest_leg = shortest_leg / dist;
  if (dist > fixed(500000))
    return (shortest_leg > fixed(0.25));
  else
    return (shortest_leg > fixed(0.28));
}


bool 
OLCFAI::triangle_closed(const ScanTaskPoint &sp) const
{
  const FlatRay leg_1(solution[0].get_flatLocation(),
                      solution[1].get_flatLocation());

  const FlatRay leg_3(solution[2].get_flatLocation(),
                      get_point(sp).get_flatLocation());

  return leg_1.intersects(leg_3);
}


bool 
OLCFAI::finish_satisfied(const ScanTaskPoint &candidate) const
{
  // for now, all other candidate checks are performed (FAI triangle rule), 
  // as well as triangle closure.

  // @todo may modify this later to allow for partial (in-progress) solutions

  return admit_candidate(candidate) 
    && triangle_closed(candidate);
}


bool 
OLCFAI::admit_candidate(const ScanTaskPoint &candidate) const
{
  // since this is called many times, want to eliminate as early as possible,
  // so put fast checks first

  if (!ContestDijkstra::admit_candidate(candidate))
    return false;

  if (is_final(candidate)) {
    return fai_triangle_satisfied(candidate);
  } else {
    return true;
  }
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

  // this specialisation of add_edges gives modified distance on last
  // leg for projected distance to start node, so the optimiser will
  // optimise closed distance even if the triangle hasn't closed yet.
  
  // this is essential as this is the basis of scoring.

  const FlatGeoPoint prev = get_point(origin).get_flatLocation();
  const FlatGeoPoint v_close = solution[0].get_flatLocation() - prev;
  
  for (; destination.second != n_points; ++destination.second) {
    if (finish_satisfied(destination)) {
      const FlatGeoPoint v_this = 
          get_point(destination).get_flatLocation() - prev;

      const unsigned d = get_weighting(origin.first) *
                         v_this.projected_distance(v_close);

      dijkstra.link(destination, origin, d);
    }
  }
}
