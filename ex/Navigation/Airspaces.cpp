#include "Airspaces.hpp"
#include "AirspaceCircle.hpp"
#include "AirspacePolygon.hpp"
#include <deque>

extern unsigned n_queries;

const std::vector<Airspace>
Airspaces::scan_nearest(const AIRCRAFT_STATE &state) const 
{
  Airspace bb_target(state.Location,get_task_projection());

  std::pair<AirspaceTree::const_iterator, double> 
    found = airspace_tree.find_nearest(bb_target);

  n_queries++;

  std::vector<Airspace> res;
  if (found.first != airspace_tree.end()) {
    // also should do scan_range with range = 0 since there
    // could be more than one with zero dist
    if (found.second==0) {
      return scan_range(state, 0);
    } else {
      res.push_back(*found.first);
    }
  }
  return res;
}


const std::vector<Airspace>
Airspaces::scan_range(const AIRCRAFT_STATE &state, const double &range) const
{
  Airspace bb_target(state.Location, get_task_projection());
  int mrange = project_range(state.Location, range);
  
  std::deque< Airspace > vectors;
  airspace_tree.find_within_range(bb_target, -mrange, std::back_inserter(vectors));

  n_queries++;

  std::vector<Airspace> res;

  for (std::deque<Airspace>::iterator v=vectors.begin();
       v != vectors.end(); v++) {
    if ((*v).distance(bb_target)<= range) {
      if ((*v).inside(state) || (range>0)) {
        res.push_back(*v);
      }
    }        
  }
  return res;
}


std::vector< Airspace >
Airspaces::find_inside(const AIRCRAFT_STATE &state) const
{
  Airspace bb_target(state.Location, get_task_projection());

  std::vector< Airspace > vectors;
  airspace_tree.find_within_range(bb_target, 0, std::back_inserter(vectors));

  n_queries++;

  for (std::vector<Airspace>::iterator v=vectors.begin();
       v != vectors.end(); ) {
    if (!(*v).inside(state)) {
      vectors.erase(v);
    } else {
      v++;
    }
  }
  return vectors;
}

void 
Airspaces::optimise()
{
  airspace_tree.optimise();
}

void 
Airspaces::insert(AbstractAirspace& asp)
{
  Airspace a(asp, get_task_projection());
  airspace_tree.insert(a);
}


