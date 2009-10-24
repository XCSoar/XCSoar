#ifndef FLATBOUNDINGBOX_HPP
#define FLATBOUNDINGBOX_HPP
#include "Math/FastMath.h"
#include "FlatBound.hpp"
#include "Waypoint.hpp"

struct FlatBoundingBox {
  FlatBoundingBox(const int x,
                  const int y):
    bounds_x(x,x),bounds_y(y,y) {};

  FlatBoundingBox(const int xmin,
             const int ymin,
             const int xmax,
             const int ymax):
    bounds_x(xmin,xmax),bounds_y(ymin,ymax) {};

  FlatBoundingBox(const FLAT_GEOPOINT &loc,
                  const unsigned range=0):
    bounds_x(loc.Longitude-range,loc.Longitude+range),
    bounds_y(loc.Latitude-range,loc.Latitude+range) 
  {

  }

  FlatBound bounds_x;
  FlatBound bounds_y;

  unsigned distance(const FlatBoundingBox &f) const {
    long dx = bounds_x.dist(f.bounds_x);
    long dy = bounds_y.dist(f.bounds_y);
    return isqrt4(dx*dx+dy*dy);
  };

  void print(std::ostream &f) const {
    f << bounds_x.min << " " << bounds_y.min << "\n";
    f << bounds_x.max << " " << bounds_y.min << "\n";
    f << bounds_x.max << " " << bounds_y.max << "\n";
    f << bounds_x.min << " " << bounds_y.max << "\n";
    f << bounds_x.min << " " << bounds_y.min << "\n";
    f << "\n";
  }

  struct kd_get_bounds {
    typedef FlatBound result_type;
    FlatBound operator() ( const FlatBoundingBox &d, const unsigned k) const {
      switch(k) {
      case 0:
        return d.bounds_x;
      case 1:
        return d.bounds_y;
      };
      return FlatBound(0,0); 
    };
  };

  // used by KD
  static inline FlatBound get_bounds( FlatBoundingBox d, unsigned k ) {
    switch(k) {
    case 0:
      return d.bounds_x;
    case 1:
      return d.bounds_y;
    };
    return FlatBound(0,0); 
  };
};

#endif
