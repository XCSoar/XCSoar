#ifndef TASKPROJECTIONCLIENT_HPP
#define TASKPROJECTIONCLIENT_HPP

#include "TaskProjection.hpp"

/**
 * Convenience class used by read-only clients of a TaskProjection
 */
class TaskProjectionClient {
public:
  TaskProjectionClient(const TaskProjection& _task_projection):
    task_projection(_task_projection) {};

  FLAT_GEOPOINT project(const GEOPOINT& tp) const;
  GEOPOINT unproject(const FLAT_GEOPOINT& tp) const;
  FlatPoint fproject(const GEOPOINT& tp) const;
  GEOPOINT funproject(const FlatPoint& tp) const;
  unsigned project_range(const GEOPOINT &tp, const double range) const;
  double fproject_range(const GEOPOINT &tp, const double range) const;

  const TaskProjection& get_task_projection() const {
    return task_projection;
  }

protected:
  const TaskProjection &task_projection;
};


#endif
