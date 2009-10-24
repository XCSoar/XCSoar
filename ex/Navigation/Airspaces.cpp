#include "Airspaces.hpp"
#include <fstream>
#include <deque>
#include <algorithm>

void 
Airspaces::scan_nearest(const GEOPOINT &loc) const 
{
  FLAT_GEOPOINT floc = task_projection.project(loc);
  FlatBoundingBox bb_target(floc);
  std::ofstream foutn("res-bb-nearest.txt");
  std::pair<FlatBoundingBoxTree::const_iterator, double> 
    found = airspace_tree.find_nearest(bb_target);
  if (found.first != airspace_tree.end()) {
    (found.first)->print(foutn, task_projection);
  }
}

void 
Airspaces::scan_range(const GEOPOINT &loc, const unsigned &range) 
{
  FLAT_GEOPOINT floc = task_projection.project(loc);
  FlatBoundingBox bb_target(floc);
  
  { // reporting
    FlatBoundingBox bb_rtarget(floc, range);
    std::ofstream foutt("res-bb-target.txt");
    bb_rtarget.print(foutt, task_projection);
  }
  
  std::deque< FlatBoundingBox > vectors;
  airspace_tree.find_within_range(bb_target, range, std::back_inserter(vectors));
  
  { // reporting
    std::ofstream foutr("res-bb-range.txt");
    std::ofstream foutf("res-bb-filtered.txt");
    for (std::deque<FlatBoundingBox>::iterator v=vectors.begin();
         v != vectors.end(); v++) {
      
      if ((*v).distance(bb_target)<= range) {
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
  for (unsigned i=0; i<200; i++) {
    int x = rand()%1000-500;
    int y = rand()%1000-500;
    int w = rand()%100;
    int h = rand()%100;
    FlatBoundingBox ff(x,y,x+w,y+h);
    airspace_tree.insert(ff);
    ff.print(fin, task_projection);
  }
  airspace_tree.optimise();
}

