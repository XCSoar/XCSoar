#include "Airspaces.hpp"
#include <fstream>
#include <deque>


void 
Airspaces::scan_nearest(const GEOPOINT &loc,
  const bool do_report) const 
{
  FLAT_GEOPOINT floc = task_projection.project(loc);
  FlatBoundingBox bb_target(floc);

  std::pair<FlatBoundingBoxTree::const_iterator, double> 
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
      scan_range(loc, 0, do_report);
      return;
    } 
  }
//  printf("outside\n");
}


void 
Airspaces::scan_range(const GEOPOINT &loc, const unsigned &range,
  const bool do_report) const
{
  FLAT_GEOPOINT floc = task_projection.project(loc);
  FlatBoundingBox bb_target(floc);
  
  if (do_report) { 
    FlatBoundingBox bb_rtarget(floc, range);
    std::ofstream foutt("res-bb-target.txt");
    bb_rtarget.print(foutt, task_projection);
  }
  
  std::deque< FlatBoundingBox > vectors;
  airspace_tree.find_within_range(bb_target, -range, std::back_inserter(vectors));

  if (do_report)  { // reporting
    std::ofstream foutr("res-bb-range.txt");
    std::ofstream foutf("res-bb-filtered.txt");
    for (std::deque<FlatBoundingBox>::iterator v=vectors.begin();
         v != vectors.end(); v++) {      
      if (1 || ((*v).distance(bb_target)<= range)) {
        (*v).print(foutr, task_projection);
      } else {
        (*v).print(foutf, task_projection);
      }        
    }
  }
}

void 
Airspaces::fill_default() 
{
  std::ofstream fin("res-bb-in.txt");
  for (unsigned i=0; i<150; i++) {
    int x = rand()%1200-600;
    int y = rand()%1200-600;
    int w = rand()%300;
    int h = rand()%300;
    FlatBoundingBox ff(x,y,x+w,y+h);
    airspace_tree.insert(ff);
    ff.print(fin, task_projection);
  }
  airspace_tree.optimise();
}

