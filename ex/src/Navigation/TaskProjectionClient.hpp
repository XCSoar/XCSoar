#ifndef TASKPROJECTIONCLIENT_HPP
#define TASKPROJECTIONCLIENT_HPP

#include "TaskProjection.hpp"

/**
 * Convenience class used by read-only clients of a TaskProjection
 */
class TaskProjectionClient {
public:
/** 
 * Constructor; takes a reference to an externally owned task projection.
 * Note that some facility needs to ensure that clients re-calculate projections
 * if the task projection changes.
 * 
 * @param _task_projection Task projection to remember
 */
  TaskProjectionClient(const TaskProjection& _task_projection):
    task_projection(_task_projection) {};

/** 
 * Re-project boundary and interior sample polygons.
 * Must be called if task_projection changes.
 * 
 */
  virtual void update_projection() = 0;

/** 
 * Project a Geodetic point to an integer 2-d representation
 * 
 * @param tp Point to project
 * 
 * @return Projected point
 */
  FLAT_GEOPOINT project(const GEOPOINT& tp) const;

/** 
 * Projects an integer 2-d representation to a Geodetic point
 * 
 * @param tp Point to project
 * 
 * @return Projected point
 */
  GEOPOINT unproject(const FLAT_GEOPOINT& tp) const;


/** 
 * Project a Geodetic point to an floating point 2-d representation
 * 
 * @param tp Point to project
 * 
 * @return Projected point
 */
  FlatPoint fproject(const GEOPOINT& tp) const;

/** 
 * Projects a floating point 2-d representation to a Geodetic point
 * 
 * @param tp Point to project
 * 
 * @return Projected point
 */
  GEOPOINT funproject(const FlatPoint& tp) const;

/** 
 * Calculates the integer flat earth distance from an actual distance
 * from a Geodetic point.  Note this is approximate.
 * 
 * @param tp Point to project
 * @param range Distance (m) from the Geodetic point
 * 
 * @return Distance in flat earth projected units
 */
  unsigned project_range(const GEOPOINT &tp, const double range) const;

/** 
 * Calculates the floating point flat earth distance from an actual distance
 * from a Geodetic point.  Note this is approximate.
 * 
 * @param tp Point to project
 * @param range Distance (m) from the Geodetic point
 * 
 * @return Distance in flat earth projected units
 */
  double fproject_range(const GEOPOINT &tp, const double range) const;

/** 
 * Accessor for task projection copy (used if need to pass it on to children)
 * 
 * @return Task projection used by this object
 */
  const TaskProjection& get_task_projection() const {
    return task_projection;
  }

protected:
  const TaskProjection &task_projection; /**< Projection used for transformations */
};


#endif
