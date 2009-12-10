#include "AirspaceAircraftPerformance.hpp"
#include "Util/ZeroFinder.hpp"

static const fixed fixed_big = 1e6;

fixed 
AirspaceAircraftPerformance::solution_general(const fixed& distance,
                                              const fixed& dh) const
{
  // can at least descend as fast as s_ld
  fixed mod_descent_rate = max(descent_rate, s_ld);

  const fixed t_cruise = distance/v_ld;
  const fixed h_descent = dh-t_cruise*s_ld;

  if (!fabs(h_descent)) {
    return t_cruise;
  } 
  if (h_descent>0) {
    // descend steeper than best glide
    if (!positive(mod_descent_rate)) {
      return fixed_big;
    }
    const fixed t_descent = h_descent/mod_descent_rate;
    return max(t_cruise, t_descent);
  } else {
    // require climb
    if (!positive(climb_rate)) {
      return fixed_big;
    }
    const fixed t_climb = -h_descent/climb_rate;
    return t_cruise+t_climb;
  }
}


/**
 * Utility class to scan for height difference that produces
 * minimum arrival time intercept with a vertical line
 */
class AirspaceAircraftInterceptVertical: 
  public ZeroFinder 
{
public:
/** 
 * Constructor
 * 
 * @param aap Performance model
 * @param distance Distance to line (m)
 * @param alt Altitude of observer (m)
 * @param h_min Height of base of line (m)
 * @param h_max Height of top of line (m)
 * 
 * @return Initialised object
 */
  AirspaceAircraftInterceptVertical(const AirspaceAircraftPerformance& aap,
                                    const fixed &distance,
                                    const fixed &alt,
                                    const fixed &h_min,
                                    const fixed &h_max):
    ZeroFinder(h_min, h_max, fixed_one),
    m_perf(aap),
    m_distance(distance),
    m_alt(alt),
    m_h_min(h_min),
    m_h_max(h_max)
    {}

  fixed f(const fixed h) {
    return m_perf.solution_general(m_distance, m_alt-h);
  }

/** 
 * Find distance of minimum time intercept with line
 * 
 * @param h Altitude of intercept to be set if solution found (m)
 * 
 * @return Time of arrival (or -1 if no solution found)
 */
  fixed solve(fixed &h) {
    fixed h_this = find_min(m_h_min); 
    fixed t = f(h_this);
    if (t<fixed_big) {
      h = h_this;
      return t;
    } else {
      return -fixed_one;
    }
  }
private:
  const AirspaceAircraftPerformance& m_perf;
  const fixed &m_distance;
  const fixed &m_alt;
  const fixed &m_h_min;
  const fixed &m_h_max;
};


fixed 
AirspaceAircraftPerformance::solution_vertical(const fixed& distance,
                                               const fixed& altitude,
                                               const fixed& base,
                                               const fixed& top,
                                               fixed& intercept_alt) const
{
  if (!solution_exists(distance, altitude, base, top)) {
    return -fixed_one;
  }
  if (!(top>base)) {
    // unique solution
    fixed t_this = solution_general(distance, altitude-top);
    if (t_this!= fixed_big) {
      intercept_alt = top;
      return t_this;
    }
  }
  AirspaceAircraftInterceptVertical aaiv(*this,
                                         distance,
                                         altitude,
                                         base,
                                         top);
  return aaiv.solve(intercept_alt);
}


/**
 * Utility class to scan for distance that produces
 * minimum arrival time intercept with a horizontal line
 */
class AirspaceAircraftInterceptHorizontal: 
  public ZeroFinder 
{
public:
/** 
 * Constructor
 * 
 * @param aap Performance model
 * @param distance_min Distance to line start (m)
 * @param distance_max Distance to line end (m)
 * @param dh Height difference between observer and line
 * 
 * @return Initialised object
 */
  AirspaceAircraftInterceptHorizontal(const AirspaceAircraftPerformance& aap,
                                      const fixed &distance_min,
                                      const fixed &distance_max,
                                      const fixed &dh):
    ZeroFinder(distance_min, distance_max, fixed_one),
    m_perf(aap),
    m_d_min(distance_min),
    m_d_max(distance_max),
    m_dh(dh)
    {}

  fixed f(const fixed distance) {
    return m_perf.solution_general(distance, m_dh);
  }

/** 
 * Find distance of minimum time intercept with line
 * 
 * @param distance Distance of intercept to be set if solution found (m)
 * 
 * @return Time of arrival (or -1 if no solution found)
 */
  fixed solve(fixed &distance) {
    fixed distance_this = find_min(m_d_min); 
    fixed t = f(distance_this);
    if (t<fixed_big) {
      distance = distance_this;
      return t;
    } else {
      return -fixed_one;
    }
  }
private:
  const AirspaceAircraftPerformance& m_perf;
  const fixed &m_d_min;
  const fixed &m_d_max;
  const fixed &m_dh;
};


fixed 
AirspaceAircraftPerformance::solution_horizontal(const fixed& distance_min,
                                                 const fixed& distance_max,
                                                 const fixed& altitude,
                                                 const fixed& h,
                                                 fixed& intercept_distance) const
{
  if (!solution_exists(distance_max, altitude, h, h)) {
    return -fixed_one;
  }
  const fixed dh = altitude-h;

  if (!(distance_max>distance_min)) {
    // unique solution
    fixed t_this = solution_general(distance_max, dh);
    if (t_this!= fixed_big) {
      intercept_distance = distance_max;
      return t_this;
    }
  }
  AirspaceAircraftInterceptHorizontal aaih(*this,
                                           distance_min,
                                           distance_max,
                                           dh);
  return aaih.solve(intercept_distance);
}
                                      

/*
TODO: write a sorter/visitor so that we can visit airspaces in increasing
  order of arrival time (plus other criteria). 
 */


bool 
AirspaceAircraftPerformance::solution_exists(const fixed& distance_max,
                                             const fixed& altitude,
                                             const fixed& h_min,
                                             const fixed& h_max) const
{
  if (positive(altitude-h_max) && !positive(descent_rate)) {
    return false;
  }
  if (positive(h_min-altitude) && !positive(climb_rate)) {
    return false;
  }
  if (positive(distance_max) && !positive(v_ld)) {
    return false;
  }
  return true;
}

