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
#ifndef GLIDEPOLAR_HPP
#define GLIDEPOLAR_HPP

struct GlideState;
struct GlideResult;
struct AIRCRAFT_STATE;

#include "PolarCoefficients.hpp"
#include "Math/fixed.hpp"
#include "Compiler.h"

struct PolarInfo;

/**
 * Class implementing basic glide polar performance model
 * 
 * Implements aircraft-specific glide performance, including
 * bugs/ballast, MacCready setting and cruise efficiency.
 *
 * Cruise efficiency is the ratio of actual cruise speed to 
 * target to the classical MacCready speed.
 * Cruise efficiency is stored in this class for convenience,
 * it is used in MacCready class.
 * 
 * The MacCready class uses this GlidePolar data to calculate
 * specific GlideSolutions. 
 *
 * This uses a parabolic glide polar:
 * \f[ w = a.V^2+b.V+c \f]
 * Where \f$ w, V\f$ are in m/s
 *
 */
class GlidePolar
{
  fixed mc;                  /**< MacCready ring setting (m/s) */
  fixed inv_mc;              /**< Inverse of MC setting (s/m) */
  fixed bugs;                /**< Clean ratio (1=clean, 0=100% bugs) */
  fixed ballast;             /**< Ballast ratio (litres) */
  fixed cruise_efficiency;   /**< Cruise efficiency */
  fixed bestLD;              /**< Best lift to drag ratio */
  fixed VbestLD;             /**< Speed for best L/D (m/s) */
  fixed SbestLD;             /**< Sink rate at best L/D (m/s, positive down) */
  fixed Vmax;                /**< Maximum cruise speed (m/s) */
  fixed Smax;                /**< Sink rate at maximum cruise speed (m/s, positive down) */
  fixed Vmin;                /**< Speed for minimum sink (m/s) */
  fixed Smin;                /**< Minimum sink rate (m/s, positive down) */

  PolarCoefficients ideal_polar;  /**< coefficients of glide polar empty/clean */
  PolarCoefficients polar;        /**< coefficients of glide polar at bug/ballast */

  fixed ballast_ratio;       /**< Ratio of mass of ballast to glider empty weight */
  fixed reference_mass;      /**< Reference mass of polar, kg */
  fixed dry_mass;            /**< Dry/unballasted mass of glider, kg */
  fixed wing_area;           /**< Reference wing area, m^2 */

  friend struct PolarInfo;
  friend class GlidePolarTest;

public:
  /**
   * Constructs an uninitialized object.
   */
  GlidePolar() {}

  /**
   * Constructor.  Performs search for best LD at instantiation
   *
   * @param _mc MacCready value at construction
   * @param _bugs Bugs (clean) ratio (default clean)
   * @param _ballast Ballast ratio (default empty)
   */
  GlidePolar(const fixed _mc, const fixed _bugs = fixed_one,
      const fixed _ballast = fixed_zero);

  /**
   * Accesses minimum sink rate
   *
   * @return Sink rate (m/s, positive down)
   */
  gcc_pure
  fixed
  GetSMin() const
  {
    return Smin;
  }

  /**
   * Accesses airspeed for minimum sink rate
   *
   * @return Speed (m/s)
   */
  gcc_pure
  fixed
  GetVMin() const
  {
    return Vmin;
  }

  /**
   * Accesses maximum airspeed
   *
   * @todo this should be set by SETTINGS_COMPUTER SafetySpeed
   *
   * @return Speed (m/s)
   */
  gcc_pure
  fixed
  GetVMax() const
  {
    return Vmax;
  }

  /**
   * Accesses sink rate at max airspeed
   *
   * @return Sink rate (m/s, positive down)
   */
  gcc_pure
  fixed
  GetSMax() const
  {
    return Smax;
  }

  /**
   * Accesses best L/D speed
   *
   * @return Speed of best LD (m/s)
   */
  gcc_pure
  fixed
  GetVBestLD() const
  {
    return VbestLD;
  }

  /**
   * Accesses best L/D sink rate (positive down)
   *
   * @return Sink rate at best L/D (m/s)
   */
  gcc_pure
  fixed
  GetSBestLD() const
  {
    return SbestLD;
  }

  /**
   * Accesses best L/D ratio
   *
   * @return Best L/D ratio
   */
  gcc_pure
  fixed GetBestLD() const
  {
    return bestLD;
  }

  /**
   * Takeoff speed
   * @return Takeoff speed threshold (m/s)
   */
  fixed GetVTakeoff() const;

  /**
   * Set cruise efficiency value.  1.0 = perfect MacCready speed
   *
   * @param _ce The new cruise efficiency value
   */
  void
  SetCruiseEfficiency(const fixed _ce)
  {
    cruise_efficiency = _ce;
  }

  /**
   * Accessor for current cruise efficiency
   *
   * @return Cruise efficiency
   */
  gcc_pure
  fixed
  GetCruiseEfficiency() const
  {
    return cruise_efficiency;
  }

  /**
   * Set bugs value.
   *
   * @param clean The new bugs setting (clean ratio) (0-1]
   */
  void SetBugs(const fixed clean);

  /**
   * Retrieve bugs 
   * @return Cleanliness of glider (0-1]
   */
  gcc_pure
  fixed GetBugs() const {
    return bugs;
  }

  /**
   * Set ballast value.
   *
   * @param ratio The new ballast setting (proportion of possible ballast, [0-1]
   */
  void SetBallast(const fixed ratio);

  /**
   * Set ballast value in litres
   * @param litres The new ballast setting (l or kg)
   */
  void SetBallastLitres(const fixed litres);

  /**
   * Retrieve ballast 
   * @return Proportion of possible ballast [0-1]
   */
  gcc_pure
  fixed GetBallast() const {
    return ballast / (ballast_ratio * reference_mass);
  }

  /**
   * Retrieve if the glider is ballasted
   */
  bool HasBallast() const {
    return positive(ballast);
  }

  /**
   * Retrieve ballast in litres
   * @return Ballast (l or kg)
   */
  fixed GetBallastLitres() const;

  /**
   * Determine if glider carries ballast
   *
   * @return True if glider can carry ballast
   */
  bool IsBallastable() const;

  /**
   * Set MacCready value.  Internally this performs search
   * for best LD values corresponding to this setting.
   *
   * @param _mc The new MacCready ring setting (m/s)
   */
  void SetMC(const fixed _mc);

  /**
   * Accessor for MC setting
   *
   * @return The current MacCready ring setting (m/s)
   */
  gcc_pure
  fixed GetMC() const {
    return mc;
  }

  /**
   * Accessor for inverse of MC setting
   *
   * @return The inverse of current MacCready ring setting (s/m)
   */
  gcc_pure
  fixed GetInvMC() const {
    return inv_mc;
  }

  /**
   * Calculate all up weight
   *
   * @return Mass (kg) of aircraft including ballast
   */
  fixed GetTotalMass() const;

  /**
   * Calculate wing loading
   *
   * @return Wing loading (all up mass divided by reference area, kg/m^2)
   */
  fixed GetWingLoading() const;

  /**
   * Sink rate model (actual glide polar) function.
   *
   * @param V Speed at which sink rate is to be evaluated
   *
   * @return Sink rate (m/s, positive down)
   */
  gcc_pure
  fixed SinkRate(const fixed V) const;

  /**
   * Sink rate model (actual glide polar) function.
   *
   * Uses a parabolic load factor model to calculate additional sink rate
   * from loading:
   *
   * \f[ w(V,n) = w_0 + ({{V}\over{2 \Lambda}})[n^2-1]({{V_\Lambda}\over{V}})^2 \f]
   * Where:
   * - \f$n \f$ is the load factor
   * - \f$\Lambda \f$ is the best L/D ratio
   * - \f$V_\Lambda \f$ is the speed for best L/D
   *
   * @param V Speed at which sink rate is to be evaluated
   * @param n Load factor
   *
   * @return Sink rate (m/s, positive down)
   */
  gcc_pure
  fixed SinkRate(const fixed V, const fixed n) const;

  /**
   * Sink rate model adjusted by MC setting.  This is used
   * to accomodate speed ring (MC) settings in optimal glide
   * calculations.
   *
   * @param V Speed at which sink rate is to be evaluated
   *
   * @return Sink rate plus MC setting (m/s, positive down)
   */
  gcc_pure
  fixed MSinkRate(const fixed V) const;

  /**
   * Quickly determine whether a task is achievable without
   * climb, assuming favorable wind.  This can be used to quickly
   * pre-filter waypoints for arrival altitude before performing
   * expensive optimal glide solution searches.
   *
   * @param task The glide task for which to estimate a solution
   *
   * @return True if a glide solution is feasible (optimistically)
   */
  bool IsGlidePossible(const GlideState &task) const;

  /**
   * Calculate speed-to-fly according to MacCready dolphin theory
   * with ring setting at current MC value.
   *
   * @param state Aircraft state (taking TrueAirspeed and Vario)
   * @param solution Solution for which Vopt is desired
   * @param block_stf Whether to use block speed to fly or dolphin
   *
   * @return Speed to fly (true, m/s)
   */
  fixed SpeedToFly(const AIRCRAFT_STATE &state, const GlideResult &solution,
      const bool block_stf) const;

  /**
   * Compute MacCready ring setting to adjust speeds to incorporate
   * risk as the aircraft gets low.
   *
   * @param height_fraction Ratio of height to climb ceiling
   * @param riskGamma Risk adjustment factor.  Lower gamma, MC is uniform with height.  High gamma, MC scales almost uniformly with height
   *
   * @return MC value adjusted for risk (m/s)
   */
  fixed GetRiskMC(const fixed height_fraction, const fixed riskGamma) const;

  /**
   * Find LD relative to ground for specified track bearing
   *
   * @param state Aircraft state (for wind)
   *
   * @return LD ratio (distance travelled per unit height loss)
   */
  fixed GetLDOverGround(const AIRCRAFT_STATE &state) const;

  /** Returns the wing area in m^2 */
  fixed GetWingArea() const {
    return wing_area;
  }

  /** Sets the wing area in m^2 */
  void SetWingArea(fixed _wing_area) {
    wing_area = _wing_area;
  }

  /** Returns the reference mass in kg */
  fixed GetReferenceMass() const {
    return reference_mass;
  }

  /** Sets the reference mass in kg */
  void SetReferenceMass(fixed _reference_mass, bool update = true) {
    reference_mass = _reference_mass;

    if (update)
      Update();
  }

  /** Returns the dry mass in kg */
  fixed GetDryMass() const {
    return dry_mass;
  }

  /** Sets the dry mass in kg */
  void SetDryMass(fixed _dry_mass, bool update = true) {
    dry_mass = _dry_mass;

    if (update)
      Update();
  }

  /** Returns the ballast ratio */
  fixed GetBallastRatio() const {
    return ballast_ratio;
  }

  /** Sets the ballast ratio */
  void SetBallastRatio(fixed _ballast_ratio) {
    ballast_ratio = _ballast_ratio;
  }

private:
  /** Update computed values after change */
  void Update();

  /** Update glide polar coefficients from ideal terms */
  void UpdatePolar();

  /** Solve for best LD at current MC/bugs/ballast setting. */
  void SolveLD();

  /** Solve for min sink rate at current bugs/ballast setting. */
  void SolveSMin();
};

#endif
