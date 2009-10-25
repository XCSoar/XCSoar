#ifndef FLATBOUNDINGBOX_HPP
#define FLATBOUNDINGBOX_HPP
#include "Math/FastMath.h"
#include "FlatBound.hpp"
#include "Waypoint.hpp"
#include "BaseTask/TaskProjection.h"

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

  void print(std::ostream &f, const TaskProjection &task_projection) const {
    FLAT_GEOPOINT ll(bounds_x.min,bounds_y.min);
    FLAT_GEOPOINT lr(bounds_x.max,bounds_y.min);
    FLAT_GEOPOINT ur(bounds_x.max,bounds_y.max);
    FLAT_GEOPOINT ul(bounds_x.min,bounds_y.max);
    GEOPOINT gll = task_projection.unproject(ll);
    GEOPOINT glr = task_projection.unproject(lr);
    GEOPOINT gur = task_projection.unproject(ur);
    GEOPOINT gul = task_projection.unproject(ul);

    f << gll.Longitude << " " << gll.Latitude << "\n";
    f << glr.Longitude << " " << glr.Latitude << "\n";
    f << gur.Longitude << " " << gur.Latitude << "\n";
    f << gul.Longitude << " " << gul.Latitude << "\n";
    f << gll.Longitude << " " << gll.Latitude << "\n";
    f << "\n";
  }

  // used by KD
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
};

#endif
