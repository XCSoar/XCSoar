#ifndef GEOELLIPSE_HPP
#define GEOELLIPSE_HPP

#include "TaskProjection.hpp"
#include "FlatEllipse.hpp"

class GeoEllipse {
public:
  GeoEllipse(const GEOPOINT &f1, const GEOPOINT &f2,
             const GEOPOINT &p,
             const TaskProjection &_task_projection): 
    task_projection(_task_projection)
    {
      ell = FlatEllipse(task_projection.fproject(f1),
                        task_projection.fproject(f2),
                        task_projection.fproject(p));
    }
  GEOPOINT parametric(double t) const {
    FlatPoint fp = ell.parametric(t);
    return task_projection.funproject(fp);
  };

  bool intersect_extended(const GEOPOINT &p,
                          GEOPOINT &i1,
                          GEOPOINT &i2) const {
    FlatPoint pf = task_projection.fproject(p);
    FlatPoint i1f, i2f;
    if (ell.intersect_extended(pf,i1f,i2f)) {
      i1 = task_projection.funproject(i1f);
      i2 = task_projection.funproject(i2f);
      return true;
    } else {
      return false;
    }
  };
private:
  TaskProjection task_projection;
  FlatEllipse ell;
};


#endif
