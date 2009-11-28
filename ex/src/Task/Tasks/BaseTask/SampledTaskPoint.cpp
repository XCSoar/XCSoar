#include "SampledTaskPoint.hpp"
#include "Navigation/ConvexHull/PolygonInterior.hpp"

SampledTaskPoint::SampledTaskPoint(const TaskProjection& tp,
                                   const Waypoint & wp,
                                   const TaskBehaviour &tb,
                                   const bool b_scored):
    TaskPoint(wp,tb),
    m_boundary_scored(b_scored),
    m_task_projection(tp),
    m_search_max(get_location(),tp),
    m_search_min(get_location(),tp),
    m_search_reference(get_location(),tp)
{
}

////////////// SAMPLES /////////////////////////


bool 
SampledTaskPoint::update_sample(const AIRCRAFT_STATE& state,
                                const TaskEvents &task_events)
{
  if (isInSector(state)) {
    // if sample is inside sample polygon
    //   return false (no update required)
    // else
    //   add sample to polygon
    //   re-compute convex hull
    //   return true; (update required)
    //
    if (PolygonInterior(state.Location, m_sampled_points)) {
      // do nothing
      return false;
    } else {
      SearchPoint sp(state.Location, m_task_projection, true);
      m_sampled_points.push_back(sp);
      // only return true if hull changed 
      return prune_interior(m_sampled_points);
    }
  }
  return false;
}


void 
SampledTaskPoint::clear_sample_all_but_last(const AIRCRAFT_STATE& ref_last) 
{
  if (!m_sampled_points.empty()) {
    m_sampled_points.clear();
    SearchPoint sp(ref_last.Location, m_task_projection, true);
    m_sampled_points.push_back(sp);
  }
}

////////////// BOUNDARY

void 
SampledTaskPoint::update_oz() 
{ 
  m_search_max = m_search_reference;
  m_search_min = m_search_reference;
  m_boundary_points.clear();
  if (m_boundary_scored) {
    for (double t=0; t<=1.0; t+= 0.05) {
      SearchPoint sp(get_boundary_parametric(t), m_task_projection);
      m_boundary_points.push_back(sp);
    }
    prune_interior(m_boundary_points);
  } else {
    m_boundary_points.push_back(m_search_reference);
  }
  update_projection();
}


///////////// SAMPLES + BOUNDARY

void 
SampledTaskPoint::update_projection()
{
  m_search_max.project(m_task_projection);
  m_search_min.project(m_task_projection);
  m_search_reference.project(m_task_projection);
  project(m_sampled_points, m_task_projection);
  project(m_boundary_points, m_task_projection);
}


void
SampledTaskPoint::reset() 
{
  m_sampled_points.clear();
}


const SearchPointVector& 
SampledTaskPoint::get_search_points()
{
  if (search_boundary_points()) {
    return m_boundary_points;
  } else {
    if (m_sampled_points.empty()) {
      if (search_nominal_if_unsampled()) {
        // this adds a point in case the waypoint was skipped
        // this is a crude way of handling the situation --- may be best
        // to de-rate the score in some way
        m_sampled_points.push_back(m_search_reference);
        return m_sampled_points;
      } else {
        return m_boundary_points;
      }
    } else {
      return m_sampled_points;
    }
  }
}

