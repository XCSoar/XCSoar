#include "Airspaces.hpp"
#include "AirspaceCircle.hpp"
#include "AirspacePolygon.hpp"
#include <fstream>
#include <deque>


void 
Airspaces::scan_nearest(const AIRCRAFT_STATE &state,
  const bool do_report) const 
{
  Airspace bb_target(state.Location,task_projection);

  std::pair<AirspaceTree::const_iterator, double> 
    found = airspace_tree.find_nearest(bb_target);
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

void 
Airspaces::fill_default() 
{
  std::ofstream fin("res-bb-in.txt");
  for (unsigned i=0; i<150; i++) {

    if (rand()%3>0) {
      GEOPOINT c;
      c.Longitude = (rand()%1200-600)/1000.0+0.5;
      c.Latitude = (rand()%1200-600)/1000.0+0.5;
      double radius = 10000.0*(0.2+(rand()%12)/12.0);

      Airspace ff(*(new AirspaceCircle(c,radius)),
                  task_projection);
      airspace_tree.insert(ff);
      ff.print(fin, task_projection);
    } else {
      Airspace ff(*(new AirspacePolygon(task_projection)),
                  task_projection);
      airspace_tree.insert(ff);
      ff.print(fin, task_projection);
    }
  }
  airspace_tree.optimise();
}

