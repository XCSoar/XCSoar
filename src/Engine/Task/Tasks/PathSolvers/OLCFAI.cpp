
#include "OLCFAI.hpp"
#include "Navigation/Flat/FlatRay.hpp"

/*
 @todo potential to use 3d convex hull to speed search

 2nd point inclusion rules:
   if min leg length is 25%, max is 50%
   so no leg can be more than twice the length of another

*/

OLCFAI::OLCFAI(const TracePointVector &_trace):
  ContestDijkstra(_trace, 3, 1000) {}


fixed
OLCFAI::leg_distance(const unsigned index) const
{
  // final destination point is the start point for potential or actual
  // closures
  const GeoPoint &p_dest = (index < 2)? 
    solution[index+1].get_location(): 
    solution[0].get_location();

  return solution[index].get_location().distance(p_dest);
}


bool 
OLCFAI::fai_triangle_satisfied(const ScanTaskPoint &sp) const
{
  // note that this is expensive since it needs accurate distance computations

  // if d<500km, shortest leg >= 28% d; else shortest leg >= 25%d
  fixed dist = fixed_zero;
  fixed shortest_leg = fixed_minus_one;
  for (unsigned i = 0; i < 3; ++i) {

    const fixed leg = leg_distance(i);
    if (leg <= fixed_zero) {
      // require all legs to have distance
      return false;
    }

    dist += leg;

    if ((i==0) || (shortest_leg > leg))
      shortest_leg = leg;
  }

  if (dist<= fixed_zero)
    return false;

  if (dist > fixed(500000))
    return (shortest_leg > fixed(0.25)*dist);
  else
    return (shortest_leg > fixed(0.28)*dist);
}


bool 
OLCFAI::triangle_closed(const ScanTaskPoint &sp) const
{
  // note this may fail if resolution of sampled trace is too low
  static const fixed fixed_1000(1000);
  return solution[0].get_location().distance(get_point(sp).get_location())
    <= fixed_1000;
}


bool 
OLCFAI::admit_finish(const ScanTaskPoint &candidate) const
{
  // since this is called many times, want to eliminate as early as possible,
  // so put fast checks first

  // for now, all other candidate checks are performed (FAI triangle rule), 
  // as well as triangle closure.

  // @todo may modify this later to allow for partial (in-progress) solutions

  return admit_candidate(candidate)
    && triangle_closed(candidate)
    && fai_triangle_satisfied(candidate);
      
  return true;
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

  const FlatGeoPoint &prev = get_point(origin).get_flatLocation();
  const FlatGeoPoint v_close = solution[0].get_flatLocation()- prev;
  
  for (; destination.second != n_points; ++destination.second) {

    const FlatGeoPoint &v_this = 
      get_point(destination).get_flatLocation() - prev;
    
    const int d = v_this.projected_distance(v_close);

    if (d) { // require positive solution
      if (admit_finish(destination)) {
        dijkstra.link(destination, origin, d * get_weighting(origin.first));
      }
    }
  }
}

fixed
OLCFAI::calc_score() const
{
  // @todo: apply handicap
  if (calc_time()) {
    return calc_distance()*fixed(0.0003);
  } else {
    return fixed_zero;
  }
}
