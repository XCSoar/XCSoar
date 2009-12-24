#include "BaseTask/ConvexHull/GrahamScan.hpp"

////////////////////////////////////////////////
/*
#include "ConvexHull/GrahamScan.hpp"
#include "ConvexHull/PolygonInterior.hpp"
#include <math.h>

std::vector<SearchPoint> sampled_points;


void test_polygon() 
{
  for (double t=0; t<1.0; t+= 0.1) {
    SearchPoint sp;
    double ang = t*2.0*3.1415926;
    sp.Location.Longitude = cos(ang);
    sp.Location.Latitude = sin(ang);
    sp.actual = true;
    sp.saved_rank = 0;
    sampled_points.push_back(sp);
  }

  GrahamScan gs(sampled_points);
  sampled_points = gs.prune_interior();

  GEOPOINT location;
  location.Longitude = 0.4;
  location.Latitude = 1.0;
  if (PolygonInterior(location, sampled_points)) {
    printf("inside\n");
  } else {
    printf("outside\n");
  }
  location.Longitude = 0.4;
  location.Latitude = 0.0;

  if (PolygonInterior(location, sampled_points)) {
    printf("inside\n");
  } else {
    printf("outside\n");
  }

}
*/
////////////////////////////////////////////////
