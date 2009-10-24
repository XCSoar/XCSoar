#include "Waypoints.hpp"


/*
typedef KDTree::KDTree<2, FLAT_GEOPOINT, 
                       std::pointer_to_binary_function<FLAT_GEOPOINT,unsigned,
                                                       int> > WPTree;

inline int return_wptree( FLAT_GEOPOINT d, unsigned k ) { return d[k]; }

void test_waypoints2()
{
  WPTree wp_tree(std::ptr_fun(return_wptree));
  
  wp_tree.insert(FLAT_GEOPOINT(0,0));
  wp_tree.insert(FLAT_GEOPOINT(20,0));
  wp_tree.insert(FLAT_GEOPOINT(0,40));
  wp_tree.insert(FLAT_GEOPOINT(10,3));
  wp_tree.insert(FLAT_GEOPOINT(5,5));
  wp_tree.optimise();
  
  FLAT_GEOPOINT target(8,4);
  
  std::pair<WPTree::const_iterator, double> found = wp_tree.find_nearest(target);
  if (found.first != wp_tree.end()) {
    printf("found at %d %d\n", (found.first)->Longitude,
           (found.first)->Latitude);
  }
  
  std::deque< FLAT_GEOPOINT > vectors;
  wp_tree.find_within_range(target, 5.0, std::back_inserter(vectors));
  if (found.first != wp_tree.end()) {
    for (std::deque<FLAT_GEOPOINT>::iterator v=vectors.begin();
         v != vectors.end(); v++) {
      printf("close %d %d\n", (*v).Longitude,
             (*v).Latitude);
    }
  }
}

*/
