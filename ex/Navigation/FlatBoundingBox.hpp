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


class FlatBoundingBox {
public:
  FlatBoundingBox(const FLAT_GEOPOINT &ll,
                  const FLAT_GEOPOINT &ur):
    bb_ll(ll.Longitude,ll.Latitude),
    bb_ur(ur.Longitude,ur.Latitude) {};

  FlatBoundingBox(const FLAT_GEOPOINT &loc,
                  const unsigned range=0):
    bb_ll(loc.Longitude-range,loc.Latitude-range),
    bb_ur(loc.Longitude+range,loc.Latitude+range) 
  {

  }

  unsigned distance(const FlatBoundingBox &f) const {
    long dx = std::max(0,std::min(f.bb_ll.Longitude-bb_ur.Longitude,
                                  bb_ll.Longitude-f.bb_ur.Longitude));
    long dy = std::max(0,std::min(f.bb_ll.Latitude-bb_ur.Latitude,
                                  bb_ll.Latitude-f.bb_ur.Latitude));
    return isqrt4(dx*dx+dy*dy);
  };

  virtual void print(std::ostream &f, const TaskProjection &task_projection) const {
    FLAT_GEOPOINT ll(bb_ll.Longitude,bb_ll.Latitude);
    FLAT_GEOPOINT lr(bb_ur.Longitude,bb_ll.Latitude);
    FLAT_GEOPOINT ur(bb_ur.Longitude,bb_ur.Latitude);
    FLAT_GEOPOINT ul(bb_ll.Longitude,bb_ur.Latitude);
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
        return d.bb_ll.Longitude;
      case 1:
        return d.bb_ll.Latitude;
      case 2:
        return d.bb_ur.Longitude;
      case 3:
        return d.bb_ur.Latitude;
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

protected:
  FLAT_GEOPOINT bb_ll;
  FLAT_GEOPOINT bb_ur;
private:

  /** @link dependency */
  /*#  BBDist lnkBBDist; */
protected:
};

#endif
