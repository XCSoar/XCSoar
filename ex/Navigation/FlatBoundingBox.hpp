#ifndef FLATBOUNDINGBOX_HPP
#define FLATBOUNDINGBOX_HPP
#include "Math/FastMath.h"
#include "Waypoint.hpp"
#include "BaseTask/TaskProjection.h"
#include <algorithm>

class BBDist {
public:
  BBDist(const size_t _dim, const int _val)
    {
      set_vals(-1);
      val[_dim%2] = _val;
      calc_d();
    }
  BBDist(const int _val) {
    set_vals(-1);
    d = _val;
  }
  BBDist& operator+=(const BBDist &rhs) {
    set_max(0, rhs);
    set_max(1, rhs);
    calc_d();
    return *this;
  }
  operator double () const {
    return d;
  }
private:
  void set_max(const size_t _dim, const BBDist &rhs) {
    val[_dim] = std::max(val[_dim],rhs.val[_dim]);
  }
  void calc_d() {
    d=0;
    for (unsigned i=0; i<2; i++) {
      if (val[i]>0) {
        d+= val[i]*val[i];
      }
    }
  }
  void set_vals(const int _val) {
    val[0] = _val;
    val[1] = _val;
  }
  int val[2];
  int d;  
};


struct FlatBoundingBox {
  FlatBoundingBox(const int x,
                  const int y):
    fmin(x,y),fmax(x,y) {};

  FlatBoundingBox(const int xmin,
             const int ymin,
             const int xmax,
             const int ymax):
    fmin(xmin,ymin),fmax(xmax,ymax) {};

  FlatBoundingBox(const FLAT_GEOPOINT &loc,
                  const unsigned range=0):
    fmin(loc.Longitude-range,loc.Latitude-range),
    fmax(loc.Longitude+range,loc.Latitude+range) 
  {

  }

  FLAT_GEOPOINT fmin;
  FLAT_GEOPOINT fmax;

  unsigned distance(const FlatBoundingBox &f) const {
    long dx = std::max(0,std::min(f.fmin.Longitude-fmax.Longitude,
                                  fmin.Longitude-f.fmax.Longitude));
    long dy = std::max(0,std::min(f.fmin.Latitude-fmax.Latitude,
                                  fmin.Latitude-f.fmax.Latitude));
    return isqrt4(dx*dx+dy*dy);
  };

  void print(std::ostream &f, const TaskProjection &task_projection) const {
    FLAT_GEOPOINT ll(fmin.Longitude,fmin.Latitude);
    FLAT_GEOPOINT lr(fmax.Longitude,fmin.Latitude);
    FLAT_GEOPOINT ur(fmax.Longitude,fmax.Latitude);
    FLAT_GEOPOINT ul(fmin.Longitude,fmax.Latitude);
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
    typedef int result_type;
    int operator() ( const FlatBoundingBox &d, const unsigned k) const {
      switch(k) {
      case 0:
        return d.fmin.Longitude;
      case 1:
        return d.fmin.Latitude;
      case 2:
        return d.fmax.Longitude;
      case 3:
        return d.fmax.Latitude;
      };
      return 0; 
    };
  };

  struct kd_distance {
    typedef BBDist distance_type;
    distance_type operator() (const int &a, const int &b, const size_t dim) const {
      int val = 0;
      if (dim<2) {
        val= std::max(b-a,0);
      } else {
        val= std::max(a-b,0);
      }
      return BBDist(dim,val);
    }
  };
};

#endif
