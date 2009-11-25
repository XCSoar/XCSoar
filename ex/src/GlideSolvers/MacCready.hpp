#ifndef MACCREADY_HPP
#define MACCREADY_HPP

#ifdef DO_PRINT
#include <iostream>
#endif

struct GlideState;
struct GlideResult;
class GlidePolar;

/**
 *  Helper class used to calculate times/speeds and altitude differences
 *  for a single task.  Takes cruise efficiency into account, which is
 *  the ratio of the average speed in cruise along track to the speed 
 *  directed by MacCready theory.  This can be set to 1.0 for pure
 *  'perfect' MacCready flying, less than one if the glider is off-course
 *  frequently, greater than one if the glider attains higher speeds due
 *  to dolphin/cloud street flying.
 *  
 *  Note that all speeds etc are assumed to be true; air density is NOT
 *  taken into account.
 */
class MacCready 
{
public:
  /** Constructor for MacCready helper class.
   *   Not intended to be used directly, but by GlidePolar class.
   *
   * @param _glide_polar The glide polar used for calculations.
   * @param _cruise_efficiency The efficiency ratio for calculations 
   */
  MacCready(const GlidePolar &_glide_polar,
            const double _cruise_efficiency);

  /** 
   * Calculates the glide solution with a specified sink rate (or lift rate)
   * instead of the actual sink rate supplied by the glide polar.
   * This can be used to calculate the effective LD required.
   * The system assumes the glider flies at the optimum LD speed irrespective
   * of this artificial sink rate.  The result includes error conditions.
   * 
   * @param task The task for which a solution is desired
   * @param S The sink rate 
   * @return Returns the glide result containing data about the optimal solution
   */
  GlideResult solve_sink(const GlideState &task,
                          const double S) const;

  /** 
   * Calculates the glide solution for a classical MacCready theory task.
   * Internally different calculations are used depending on the nature of the
   * task (for example, zero distance, climb or descent allowable or 
   * altitude-holding cruise-climb).  The result includes error conditions.
   * 
   * @param task The task for which a solution is desired
   * @return Returns the glide result containing data about the optimal solution
   */
  GlideResult solve(const GlideState &task) const;

  /**
   * Calculates the glide solution for a classical MacCready theory task
   * with no climb component (pure glide).  This is used internally to
   * determine the optimum speed for this glide component.
   *
   * @param task The task for which a solution is desired
   * @param V The airspeed the glider will be travelling
   * @return Returns the glide result containing data about the optimal solution
   */
  GlideResult solve_glide(const GlideState &task,
			   const double V) const;

  /**
   * Returns current MacCready setting of the glide polar (convenience function)
   */
  double get_mc() const;

private:

  /**
   * Calculates the glide solution for a classical MacCready theory task
   * with no climb component (pure glide).  This is used internally to
   * determine the optimum speed for this glide component.
   *
   * @param task The task for which a solution is desired
   * @param V The airspeed the glider will be travelling
   * @param S The sinkrate of the glider in cruise
   * @return Returns the glide result containing data about the optimal solution
   */
  GlideResult solve_glide(const GlideState &task,
			   const double V,
                           const double S) const;

/** 
 * Solve a task which is known to be pure glide,
 * seeking optimal speed to fly. 
 * 
 * @param task Task to solve for
 * 
 * @return Solution
 */
  GlideResult optimise_glide(const GlideState &task) const;

/** 
 * Solve a task which is known to be pure climb (no distance 
 * to travel other than that due to drift).
 *
 * \todo
 * - Check equations
 * 
 * @param task Task to solve for
 * 
 * @return Solution
 */
  GlideResult solve_vertical(const GlideState &task) const;

/** 
 * Solve a task which is known to be pure climb-cruise 
 * (zero height difference)
 *
 * @param task Task to solve for
 * 
 * @return Solution
 */
  GlideResult solve_cruise(const GlideState &task) const;

  const GlidePolar &glide_polar;
  const double cruise_efficiency;

  /** @link dependency */
  /*#  MacCreadyVopt lnkMacCreadyVopt; */

  /** @link dependency */
  /*#  GlideQuadratic lnkGlideQuadratic; */
};


#endif
