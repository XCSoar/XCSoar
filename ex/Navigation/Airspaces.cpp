#include "Airspaces.hpp"
#include <fstream>
#include <deque>
#include <algorithm>

void 
Airspaces::scan_nearest(const FLAT_GEOPOINT &loc) const 
{
  FlatBoundingBox bb_target(loc);
  std::ofstream foutn("res-bb-nearest.txt");
  std::pair<FlatBoundingBoxTree::const_iterator, double> 
    found = airspace_tree.find_nearest(bb_target);
  if (found.first != airspace_tree.end()) {
    (found.first)->print(foutn);
  }
}

void 
Airspaces::scan_range(const FLAT_GEOPOINT &loc, const unsigned &range) 
{
  FlatBoundingBox bb_target(loc);
  
  { // reporting
    FlatBoundingBox bb_rtarget(loc, range);
    std::ofstream foutt("res-bb-target.txt");
    bb_rtarget.print(foutt);
  }
  
  std::deque< FlatBoundingBox > vectors;
  airspace_tree.find_within_range(bb_target, range, std::back_inserter(vectors));
  
  { // reporting
    std::ofstream foutr("res-bb-range.txt");
    std::ofstream foutf("res-bb-filtered.txt");
    for (std::deque<FlatBoundingBox>::iterator v=vectors.begin();
         v != vectors.end(); v++) {
      
      if ((*v).distance(bb_target)<= range) {
        (*v).print(foutr);
      } else {
        (*v).print(foutf);
      }        
    }
  }
}

void 
Airspaces::fill_default() 
{
  std::ofstream fin("res-bb-in.txt");
  for (unsigned i=0; i<500; i++) {
    int x = rand()%500-250;
    int y = rand()%500-250;
    int w = rand()%50;
    int h = rand()%50;
    FlatBoundingBox ff(x,y,x+w,y+h);
    airspace_tree.insert(ff);
    ff.print(fin);
  }
  airspace_tree.optimise();
}

