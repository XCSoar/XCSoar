#include "FlatBoundingBox.hpp"
#include "Math/FastMath.h"
#include <algorithm>

unsigned 
FlatBoundingBox::distance(const FlatBoundingBox &f) const {
  long dx = std::max(0,std::min(f.bb_ll.Longitude-bb_ur.Longitude,
                                bb_ll.Longitude-f.bb_ur.Longitude));
  long dy = std::max(0,std::min(f.bb_ll.Latitude-bb_ur.Latitude,
                                bb_ll.Latitude-f.bb_ur.Latitude));
  return isqrt4(dx*dx+dy*dy);
}

void swap(double &t1, double &t2) 
{
  double t3 = t1;
  t1= t2;
  t2= t3;
}

bool 
FlatBoundingBox::intersects(const FlatRay& ray) const
{
  double tmin = 0.0;
  double tmax = 1.0;
  
  // Longitude
  if (ray.vector.Longitude==0) {
    // ray is parallel to slab. No hit if origin not within slab
    if ((ray.point.Longitude< bb_ll.Longitude) ||
        (ray.point.Longitude> bb_ur.Longitude)) {
      return false;
    }
  } else {
    // compute intersection t value of ray with near/far plane of slab
    double t1 = (bb_ll.Longitude-ray.point.Longitude)*ray.fx;
    double t2 = (bb_ur.Longitude-ray.point.Longitude)*ray.fx;
    // make t1 be intersection with near plane, t2 with far plane
    if (t1>t2) swap(t1, t2);
    tmin = std::max(tmin, t1);
    tmax = std::min(tmax, t2);
    // exit with no collision as soon as slab intersection becomes empty
    if (tmin>tmax) return false;
  }

  // Latitude
  // Longitude
  if (ray.vector.Latitude==0) {
    // ray is parallel to slab. No hit if origin not within slab
    if ((ray.point.Latitude< bb_ll.Latitude) ||
        (ray.point.Latitude> bb_ur.Latitude)) {
      return false;
    }
  } else {
    // compute intersection t value of ray with near/far plane of slab
    double t1 = (bb_ll.Latitude-ray.point.Latitude)*ray.fy;
    double t2 = (bb_ur.Latitude-ray.point.Latitude)*ray.fy;
    // make t1 be intersection with near plane, t2 with far plane
    if (t1>t2) swap(t1, t2);
    tmin = std::max(tmin, t1);
    tmax = std::min(tmax, t2);
    // exit with no collision as soon as slab intersection becomes empty
    if (tmin>tmax) return false;
  }
  return true;
}
