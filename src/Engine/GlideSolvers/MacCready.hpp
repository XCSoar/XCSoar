/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */
#ifndef MACCREADY_HPP
#define MACCREADY_HPP

#include "Math/fixed.hpp"

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
  /**
   * Constructor for MacCready helper class.
   * Not intended to be used directly, but by GlidePolar class.
   *
   * @param _glide_polar The glide polar used for calculations.
   * @param _cruise_efficiency The efficiency ratio for calculations 
   */
  MacCready(const GlidePolar &_glide_polar, const fixed _cruise_efficiency);

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
  GlideResult solve_sink(const GlideState &task, const fixed S) const;
  static GlideResult solve_sink(const GlidePolar &glide_polar,
                                const GlideState &task, const fixed S);

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
  static GlideResult solve(const GlidePolar &glide_polar,
                           const GlideState &task);

  /**
   * Calculates the glide solution for a classical MacCready theory task
   * with no climb component (pure glide).  This is used internally to
   * determine the optimum speed for this glide component.
   *
   * @param task The task for which a solution is desired
   * @param V The airspeed the glider will be travelling
   * @param allow_partial Return after glide exhausted
   * @return Returns the glide result containing data about the optimal solution
   */
  gcc_pure
  GlideResult solve_glide(const GlideState &task, const fixed V,
                          const bool allow_partial = false) const;

private:
  /**
   * Calculates the glide solution for a classical MacCready theory task
   * with no climb component (pure glide).  This is used internally to
   * determine the optimum speed for this glide component.
   *
   * @param task The task for which a solution is desired
   * @param V The airspeed the glider will be travelling
   * @param S The sinkrate of the glider in cruise
   * @param allow_partial Return after glide exhausted
   * @return Returns the glide result containing data about the optimal solution
   */
  GlideResult
  solve_glide(const GlideState &task, const fixed V, const fixed S,
              const bool allow_partial = false) const;

  /**
   * Solve a task which is known to be pure glide,
   * seeking optimal speed to fly.
   *
   * @param task Task to solve for
   * @param allow_partial Return after glide exhausted
   *
   * @return Solution
   */
  GlideResult optimise_glide(const GlideState &task,
                             const bool allow_partial = false) const;

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
  const fixed cruise_efficiency;

  // /** @link dependency */
  /*#  MacCreadyVopt lnkMacCreadyVopt; */

  // /** @link dependency */
  /*#  GlideQuadratic lnkGlideQuadratic; */
};

#endif
