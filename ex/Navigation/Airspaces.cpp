#include "Airspaces.hpp"
#include "AirspaceCircle.hpp"
#include "AirspacePolygon.hpp"
#include <fstream>
#include <deque>

extern unsigned n_queries;

void 
Airspaces::scan_nearest(const AIRCRAFT_STATE &state,
  const bool do_report) const 
{
  Airspace bb_target(state.Location,task_projection);

  std::pair<AirspaceTree::const_iterator, double> 
    found = airspace_tree.find_nearest(bb_target);

  n_queries++;

  if (found.first != airspace_tree.end()) {
    if (do_report) {
      std::ofstream foutn("res-bb-nearest.txt");
      (found.first)->print(foutn, task_projection);
    }
    // also should do scan_range with range = 0 since there
    // could be more than one with zero dist
    if (found.second==0) {
//      printf("inside\n");
      scan_range(state, 0, do_report);
      return;
    } 
  }
//  printf("outside\n");
}


void 
Airspaces::scan_range(const AIRCRAFT_STATE &state, const double &range,
  const bool do_report) const
{
  Airspace bb_target(state.Location, task_projection);
  int mrange = task_projection.project_range(state.Location, range);
  
  if (do_report) { 
    Airspace bb_rtarget(state.Location, task_projection, range);
    std::ofstream foutt("res-bb-target.txt");
    bb_rtarget.print(foutt, task_projection);
  }
  
  std::deque< Airspace > vectors;
  airspace_tree.find_within_range(bb_target, -mrange, std::back_inserter(vectors));

  n_queries++;

  if (do_report)  { // reporting
    std::ofstream foutr("res-bb-range.txt");
    for (std::deque<Airspace>::iterator v=vectors.begin();
         v != vectors.end(); v++) {
      if ((*v).distance(bb_target)<= range) {
        if ((*v).inside(state) || (range>0)) {
          (*v).print(foutr, task_projection);
        }
      }        
    }
  }
}


std::vector< Airspace >
Airspaces::find_inside(const AIRCRAFT_STATE &state,
  const bool do_report) const
{
  Airspace bb_target(state.Location, task_projection);

  std::vector< Airspace > vectors;
  airspace_tree.find_within_range(bb_target, 0, std::back_inserter(vectors));

  n_queries++;

  std::ofstream foutn("res-bb-inside.txt");

  for (std::vector<Airspace>::iterator v=vectors.begin();
       v != vectors.end(); ) {
    if (!(*v).inside(state)) {
      vectors.erase(v);
    } else {
      (*v).print(foutn, task_projection);
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
  Airspace a(asp, task_projection);
  airspace_tree.insert(a);
}


