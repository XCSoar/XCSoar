#include "TaskProjectionClient.hpp"

FLAT_GEOPOINT 
TaskProjectionClient::project(const GEOPOINT& tp) const
{
  return task_projection.project(tp);
}

GEOPOINT 
TaskProjectionClient::unproject(const FLAT_GEOPOINT& tp) const
{
  return task_projection.unproject(tp);
}

FlatPoint
TaskProjectionClient::fproject(const GEOPOINT& tp) const
{
  return task_projection.fproject(tp);
}

GEOPOINT 
TaskProjectionClient::funproject(const FlatPoint& tp) const
{
  return task_projection.funproject(tp);
}

unsigned 
TaskProjectionClient::project_range(const GEOPOINT &tp, const double range) const
{
  return task_projection.project_range(tp, range);
}
 
double 
TaskProjectionClient::fproject_range(const GEOPOINT &tp, const double range) const
{
  return task_projection.fproject_range(tp, range);
}

