
#include "OLCFAI.hpp"
#include "Navigation/Flat/FlatRay.hpp"

/*
 @todo potential to use 3d convex hull to speed search

 2nd point inclusion rules:
   if min leg length is 25%, max is 45%
   pmin = 0.25
   pmax = 0.45
   ptot = 2*pmin+pmax+0.05
   with 2 legs:
      ptot = pmin+pmax


  0: start
  1: first leg start
  2: second leg start
  3: third leg start
  4: end
*/

OLCFAI::OLCFAI(const TracePointVector &_trace):
  ContestDijkstra(_trace, 4, 1000) {}


fixed
OLCFAI::leg_distance(const unsigned index) const
{  
  // leg 0: 1-2
  // leg 1: 2-3
  // leg 2: 3-1

  const GeoPoint &p_start = solution[index+1].get_location();
  const GeoPoint &p_dest = (index < 2)? 
    solution[index+2].get_location():
    solution[1].get_location();

  return p_start.distance(p_dest);
}


bool 
OLCFAI::fai_triangle_satisfied() const
{
  // note that this is expensive since it needs accurate distance computations

  // if d<500km, shortest leg >= 28% d and longest_leg <= 45% d; 
  // else shortest leg >= 25%d

  fixed dist = fixed_zero;
  fixed shortest_leg = fixed_minus_one;
  fixed longest_leg = fixed_zero;
  for (unsigned i = 0; i < 3; ++i) {

    const fixed leg = leg_distance(i);
    if (leg <= fixed_zero) {
      // require all legs to have distance
      return false;
    }

    dist += leg;

    if ((i==0) || (shortest_leg > leg))
      shortest_leg = leg;
    if (longest_leg < leg)
      longest_leg = leg;
  }

  if (dist<= fixed_zero)
    return false;

  if (dist >= fixed(500000)) 
    return (shortest_leg >= fixed(0.25)*dist) &&
      (longest_leg <= fixed(0.45)*dist);
  else
    return (shortest_leg >= fixed(0.28)*dist);
}


bool 
OLCFAI::triangle_closed() const
{
  // note this may fail if resolution of sampled trace is too low
  static const fixed fixed_1000(1000);
  return solution[0].get_location().distance(solution[4].get_location())
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
//    && triangle_closed()
    && fai_triangle_satisfied();

  return true;
}


void 
OLCFAI::add_edges(DijkstraTaskPoint &dijkstra, const ScanTaskPoint& origin)
{
  ScanTaskPoint destination(origin.first+1, origin.second);

  switch (destination.first) {
  case 1:
    // don't award any points for first partial leg
    for (; destination.second != n_points; ++destination.second) {
      dijkstra.link(destination, origin, 0);
    }
    break;
  case 2:
    ContestDijkstra::add_edges(dijkstra, origin);
    break;
  case 3:    
    find_solution(dijkstra, origin);

    // give first leg points to penultimate node
    for (; destination.second != n_points; ++destination.second) {
      const unsigned d = get_weighting(origin.first) *
        (get_point(destination).flat_distance(solution[1])+
         get_point(destination).flat_distance(get_point(origin)));
      dijkstra.link(destination, origin, d);
    }
    break;
  case 4:
    // don't award any points for last partial leg
    find_solution(dijkstra, origin);

    destination.second = n_points-1;
    solution[4] = get_point(destination);
    if (admit_finish(destination)) {
      dijkstra.link(destination, origin, 0);
    }
    break;
  default:
    assert(1);
  }
}


fixed
OLCFAI::calc_distance() const
{
  if (calc_time()) {
    return leg_distance(0)+leg_distance(1)+leg_distance(2);
  } else {
    return fixed_zero;
  }
}


fixed
OLCFAI::calc_score() const
{
  // @todo: apply handicap
  return calc_distance()*fixed(0.0003);
}
