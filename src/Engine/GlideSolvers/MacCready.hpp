/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Compiler.h"

struct GlideSettings;
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
  /* This attribute is currently not used, but may be some day. */
  gcc_unused_field const GlideSettings &settings;

  const GlidePolar &glide_polar;
  const double cruise_efficiency;

public:
  /**
   * Constructor for MacCready helper class.
   * Not intended to be used directly, but by GlidePolar class.
   *
   * @param _glide_polar The glide polar used for calculations.
   * @param _cruise_efficiency The efficiency ratio for calculations 
   */
  MacCready(const GlideSettings &settings, const GlidePolar &_glide_polar,
            const double _cruise_efficiency);

  MacCready(const GlideSettings &settings, const GlidePolar &glide_polar);

  /** 
   * Calculates the glide solution with a specified sink rate (or lift rate)
   * instead of the actual sink rate supplied by the glide polar.
   * This can be used to calculate the effective LD required.
   * The system assumes the glider flies at the optimum LD speed irrespective
   * of this artificial sink rate.  The result includes error conditions.
   * 
   * @param task The task for which a solution is desired
   * @param sink_rate The sink rate
   * @return Returns the glide result containing data about the optimal solution
   */
  gcc_pure
  GlideResult SolveSink(const GlideState &task, const double sink_rate) const;

  gcc_pure
  static GlideResult SolveSink(const GlideSettings &settings,
                               const GlidePolar &glide_polar,
                               const GlideState &task, const double sink_rate);

  /**
   * Like Solve(), but always assume straight glide, no cruise.
   */
  gcc_pure
  GlideResult SolveStraight(const GlideState &task) const;

  /** 
   * Calculates the glide solution for a classical MacCready theory task.
   * Internally different calculations are used depending on the nature of the
   * task (for example, zero distance, climb or descent allowable or 
   * altitude-holding cruise-climb).  The result includes error conditions.
   * 
   * @param task The task for which a solution is desired
   * @return Returns the glide result containing data about the optimal solution
   */
  gcc_pure
  GlideResult Solve(const GlideState &task) const;

  gcc_pure
  static GlideResult Solve(const GlideSettings &settings,
                           const GlidePolar &glide_polar,
                           const GlideState &task);

  /**
   * Calculates the glide solution for a classical MacCready theory task
   * with no climb component (pure glide).  This is used internally to
   * determine the optimum speed for this glide component.
   *
   * If allow_partial=true and the task.altitiude_difference is insufficient
   * to glide to the destination, then result.vector.Distance is redueced to
   * reachable partial glide and the result represents this partial glide.
   * If allow_partial=false then the result represents the glide the full
   * distance to the Destination regardless of the task.altitude_difference
   * (result.altitude_difference may be negative).

   * @param task The task for which a solution is desired
   * @param v_set The airspeed the glider will be travelling
   * @param allow_partial Return after glide exhausted
   * @return Returns the glide result containing data about the optimal solution
   */
  gcc_pure
  GlideResult SolveGlide(const GlideState &task, const double v_set,
                         const bool allow_partial = false) const;

private:
  /**
   * Calculates the glide solution for a classical MacCready theory task
   * with no climb component (pure glide).  This is used internally to
   * determine the optimum speed for this glide component.
   *
   * @param task The task for which a solution is desired
   * @param v_set The airspeed the glider will be travelling
   * @param sink_rate The sinkrate of the glider in cruise
   * @param allow_partial Return after glide exhausted
   * @return Returns the glide result containing data about the optimal solution
   */
  gcc_pure
  GlideResult
  SolveGlide(const GlideState &task, const double v_set,
             const double sink_rate,
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
  gcc_pure
  GlideResult OptimiseGlide(const GlideState &task,
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
  gcc_pure
  GlideResult SolveVertical(const GlideState &task) const;

  /**
   * Solve a task which is known to be pure climb-cruise
   * (zero height difference)
   *
   * @param task Task to solve for
   *
   * @return Solution
   */
  gcc_pure
  GlideResult SolveCruise(const GlideState &task) const;
};

#endif
