#ifndef GEOELLIPSE_HPP
#define GEOELLIPSE_HPP

#include "BaseTask/TaskProjection.h"

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
private:
  TaskProjection task_projection;
  FlatEllipse ell;
};


#endif
