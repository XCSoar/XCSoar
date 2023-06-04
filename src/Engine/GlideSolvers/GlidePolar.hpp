// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "PolarCoefficients.hpp"

#include <type_traits>
#include <cassert>

struct GlideState;
struct GlideResult;
struct AircraftState;
class Angle;
struct PolarInfo;
struct SpeedVector;

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
  static constexpr double TOLERANCE_MIN_SINK = 0.01;
  static constexpr double TOLERANCE_BEST_LD = 0.000001;

  /** MacCready ring setting (m/s) */
  double mc;
  /** Inverse of MC setting (s/m) */
  double inv_mc;

  /** Clean ratio (1=clean, 0=100% bugs) */
  double bugs;
  /** Ballast (litres alias kg) */
  double ballast;
  /** Cruise efficiency */
  double cruise_efficiency;

  /** Best lift to drag ratio */
  double bestLD;
  /** Speed for best L/D (m/s) */
  double VbestLD;
  /** Sink rate at best L/D (m/s, positive down) */
  double SbestLD;

  /** Maximum cruise speed (m/s) */
  double Vmax;
  /** Sink rate at maximum cruise speed (m/s, positive down) */
  double Smax;

  /** Speed for minimum sink (m/s) */
  double Vmin;
  /** Minimum sink rate (m/s, positive down) */
  double Smin;

  /** coefficients of glide polar empty/clean */
  PolarCoefficients reference_polar;
  /** coefficients of glide polar at bug/ballast */
  PolarCoefficients polar;

  /** Ratio of mass of ballast to glider empty weight */
  double ballast_ratio;
  /** Reference mass of reference_polar, kg */
  double reference_mass;
  /** Plain rigged/unballasted mass of glider, kg */
  double empty_mass;
  /** Crew mass addition to empty mass (anything except droppable ballast), kg */
  double crew_mass;
  /** Reference wing area, m^2 */
  double wing_area;

  friend class GlidePolarTest;

public:
  /**
   * Constructs an uninitialized object.
   */
  constexpr GlidePolar() noexcept = default;

  /**
   * Constructor.  Performs search for best LD at instantiation
   *
   * @param _mc MacCready value at construction
   * @param _bugs Bugs (clean) ratio (default clean)
   * @param _ballast Ballast ratio (default empty)
   */
  GlidePolar(const double _mc, const double _bugs=1,
             const double _ballast=0) noexcept;

  /**
   * Constructs a GlidePolar object that is invalid.
   */
  [[gnu::const]]
  static GlidePolar Invalid() noexcept {
    GlidePolar gp(0);
    gp.SetInvalid();
    return gp;
  }

  /**
   * Mark this polar as "invalid", but retain the settings (MacCready,
   * bugs, ballast, cruise efficiency).
   */
  void SetInvalid() noexcept {
    reference_polar.SetInvalid();
    polar.SetInvalid();
    Update();
  }

  /**
   * Perform basic checks on the validity of the object.
   */
  constexpr bool IsValid() const noexcept {
    return Vmin < Vmax;
  }

  /**
   * Accesses minimum sink rate
   *
   * @return Sink rate (m/s, positive down)
   */
  constexpr double GetSMin() const noexcept {
    assert(IsValid());
    return Smin;
  }

  /**
   * Accesses airspeed for minimum sink rate
   *
   * @return Speed (m/s)
   */
  constexpr double GetVMin() const noexcept {
    assert(IsValid());
    return Vmin;
  }

  /**
   * Accesses maximum airspeed
   *
   * @todo this should be set by ComputerSettings SafetySpeed
   *
   * @return Speed (m/s)
   */
  constexpr double GetVMax() const noexcept {
    assert(IsValid());
    return Vmax;
  }

  void SetVMax(double _v_max, bool update = true) noexcept {
    Vmax = _v_max;

    if (update) {
      UpdateSMax();
      UpdateSMin();
    }
  }

  /**
   * Accesses sink rate at max airspeed
   *
   * @return Sink rate (m/s, positive down)
   */
  constexpr double GetSMax() const noexcept {
    assert(IsValid());

    return Smax;
  }

  /**
   * Accesses best L/D speed
   *
   * @return Speed of best LD (m/s)
   */
  constexpr double GetVBestLD() const noexcept {
    assert(IsValid());

    return VbestLD;
  }

  /**
   * Accesses best L/D sink rate (positive down)
   *
   * @return Sink rate at best L/D (m/s)
   */
  constexpr double GetSBestLD() const noexcept {
    assert(IsValid());

    return SbestLD;
  }

  /**
   * Accesses best L/D ratio
   *
   * @return Best L/D ratio
   */
  constexpr double GetBestLD() const noexcept {
    assert(IsValid());

    return bestLD;
  }

  /**
   * Calculate the airspeed for the best glide ratio over ground,
   * considering the given head wind.
   */
  [[gnu::pure]]
  double GetBestGlideRatioSpeed(double head_wind) const noexcept;

  /**
   * Takeoff speed
   * @return Takeoff speed threshold (m/s)
   */
  [[gnu::pure]]
  double GetVTakeoff() const noexcept;

  /**
   * Set cruise efficiency value.  1.0 = perfect MacCready speed
   *
   * @param _ce The new cruise efficiency value
   */
  void SetCruiseEfficiency(const double _ce) noexcept {
    cruise_efficiency = _ce;
  }

  /**
   * Accessor for current cruise efficiency
   *
   * @return Cruise efficiency
   */
  constexpr double GetCruiseEfficiency() const noexcept {
    return cruise_efficiency;
  }

  /**
   * Set bugs value.
   *
   * @param clean The new bugs setting (clean ratio) (0-1]
   */
  void SetBugs(const double clean) noexcept;

  /**
   * Retrieve bugs 
   * @return Cleanliness of glider (0-1]
   */
  constexpr double GetBugs() const noexcept {
    return bugs;
  }

  /**
   * Set ballast value.
   *
   * @param ratio The new ballast setting (proportion of possible ballast, [0-1]
   */
  void SetBallast(const double ratio) noexcept;

  /**
   * Set ballast value in litres
   * @param litres The new ballast setting (l or kg)
   */
  void SetBallastLitres(const double litres) noexcept;

  /**
   * Retrieve ballast 
   * @return Proportion of possible ballast [0-1]
   */
  constexpr double GetBallast() const noexcept {
    return ballast / (ballast_ratio * reference_mass);
  }

  /**
   * Retrieve if the glider is ballasted
   */
  constexpr bool HasBallast() const noexcept {
    return ballast > 0;
  }

  /**
   * Retrieve ballast in litres
   * @return Ballast (l or kg)
   */
  constexpr double GetBallastLitres() const noexcept {
    return ballast;
  }

  /**
   * Determine if glider carries ballast
   *
   * @return True if glider can carry ballast
   */
  constexpr bool IsBallastable() const noexcept {
    return ballast_ratio > 0;
  }

  /**
   * Set MacCready value.  Internally this performs search
   * for best LD values corresponding to this setting.
   *
   * @param _mc The new MacCready ring setting (m/s)
   */
  void SetMC(const double _mc) noexcept;

  /**
   * Accessor for MC setting
   *
   * @return The current MacCready ring setting (m/s)
   */
  constexpr double GetMC() const noexcept {
    return mc;
  }

  /**
   * Accessor for inverse of MC setting
   *
   * @return The inverse of current MacCready ring setting (s/m)
   */
  constexpr double GetInvMC() const noexcept {
    return inv_mc;
  }

  /**
   * Calculate all up weight
   *
   * @return Mass (kg) of aircraft including ballast
   */
  [[gnu::pure]]
  double GetTotalMass() const noexcept;

  /**
   * Calculate wing loading
   *
   * @return Wing loading (all up mass divided by reference area, kg/m^2)
   */
  [[gnu::pure]]
  double GetWingLoading() const noexcept;

  /**
   * Sink rate model (actual glide polar) function.
   *
   * @param V Speed at which sink rate is to be evaluated
   *
   * @return Sink rate (m/s, positive down)
   */
  [[gnu::pure]]
  double SinkRate(double V) const noexcept;

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
  [[gnu::pure]]
  double SinkRate(double V, double n) const noexcept;

  /**
   * Sink rate model adjusted by MC setting.  This is used
   * to accomodate speed ring (MC) settings in optimal glide
   * calculations.
   *
   * @param V Speed at which sink rate is to be evaluated
   *
   * @return Sink rate plus MC setting (m/s, positive down)
   */
  [[gnu::pure]]
  double MSinkRate(double V) const noexcept;

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
  [[gnu::pure]]
  bool IsGlidePossible(const GlideState &task) const noexcept;

  /**
   * Calculate speed-to-fly according to MacCready dolphin theory
   * with ring setting at current MC value.
   *
   * @param state Aircraft state (taking TrueAirspeed and Vario)
   * @param solution Solution for which Vopt is desired
   * @param block_stf Whether to use block speed to fly or dolphin
   *
   * @return Speed to fly (true, m/s)SpeedToFly
   */
  [[gnu::pure]]
  double SpeedToFly(const AircraftState &state, const GlideResult &solution,
                   bool block_stf) const noexcept;

  /**
   * Calculate speed-to-fly according to MacCready dolphin theory
   * with ring setting at current MC value, at specified netto sink rate
   * and head wind.
   *
   * @param stf_sink_rate_vario Netto sink rate (m/s)
   * @param head_wind Head wind component (m/s)
   *
   * @return Speed to fly (true, m/s)SpeedToFly
   */
  [[gnu::pure]]
  double SpeedToFly(double stf_sink_rate_vario,
                    double head_wind) const noexcept;

  /**
   * Compute MacCready ring setting to adjust speeds to incorporate
   * risk as the aircraft gets low.
   *
   * @param height_fraction Ratio of height to climb ceiling
   * @param riskGamma Risk adjustment factor.  Lower gamma, MC is uniform
   * with height.  High gamma, MC scales almost uniformly with height
   *
   * @return MC value adjusted for risk (m/s)
   */
  [[gnu::pure]]
  double GetRiskMC(double height_fraction, double riskGamma) const noexcept;

  /**
   * Find LD relative to ground for specified track bearing
   *
   * @param track the true aircraft ground direction
   * @param wind the wind vector
   * @return LD ratio (distance travelled per unit height loss)
   */
  [[gnu::pure]]
  double GetLDOverGround(Angle track, SpeedVector wind) const noexcept;

  /**
   * Find LD relative to ground for specified track bearing
   *
   * @param state Aircraft state (for wind)
   *
   * @return LD ratio (distance travelled per unit height loss)
   */
  [[gnu::pure]]
  double GetLDOverGround(const AircraftState &state) const noexcept;

  /**
   * Calculates the thermal value of next leg that is equivalent (gives the
   * same average speed) to the current MacCready setting.
   *
   * @param current_wind The head wind component on current leg
   * @param next_wind The head wind component on next leg
   *
   * @return Equivalent thermal strength. Normally a positive value, but in
   * some situations it can be negative.
   */
  [[gnu::pure]]
  double GetNextLegEqThermal(double current_wind, double next_wind) const noexcept;

  /** Returns the wing area in m^2 */
  constexpr double GetWingArea() const noexcept {
    return wing_area;
  }

  /** Sets the wing area in m^2 */
  constexpr void SetWingArea(double _wing_area) noexcept {
    wing_area = _wing_area;
  }

  /** Returns the reference mass in kg */
  constexpr double GetReferenceMass() const noexcept {
    return reference_mass;
  }

  /** Sets the reference mass in kg */
  void SetReferenceMass(double _reference_mass, bool update=true) noexcept {
    reference_mass = _reference_mass;

    if (update)
      Update();
  }

  /** Returns the dry mass in kg */
  constexpr double GetDryMass() const noexcept {
    return empty_mass + crew_mass;
  }
  
  /** Returns the empty mass in kg */
  constexpr double GetEmptyMass() const noexcept {
    return empty_mass;
  }
  
  /** Sets the empty mass in kg */
  void SetEmptyMass(double _empty_mass, bool update=true) noexcept {
    empty_mass = _empty_mass;

    if (update)
      Update();
  }
  
  /** Sets the crew mass in kg */
  void SetCrewMass(double _crew_mass, bool update=true) noexcept {
    crew_mass = _crew_mass;

    if (update)
      Update();
  }
  
  /** Returns the crew mass in kg */
  constexpr double GetCrewMass() const noexcept {
    return crew_mass;
  }
  
  /** Returns the ballast ratio */
  constexpr double GetBallastRatio() const noexcept {
    return ballast_ratio;
  }

  /** Sets the ballast ratio */
  constexpr void SetBallastRatio(double _ballast_ratio) noexcept {
    ballast_ratio = _ballast_ratio;
  }

  /** Returns the ideal polar coefficients */
  constexpr const PolarCoefficients &GetCoefficients() const noexcept {
    return reference_polar;
  }

  /** Returns the real polar coefficients */
  constexpr const PolarCoefficients &GetRealCoefficients() const noexcept {
    return polar;
  }

  /** Sets the ideal polar coefficients */
  void SetCoefficients(PolarCoefficients coeff, bool update=true) noexcept {
    reference_polar = coeff;

    if (update)
      Update();
  }

  /** Update glide polar coefficients and values depending on them */
  void Update() noexcept;

  /** Calculate average speed in still air */
  [[gnu::pure]]
  double GetAverageSpeed() const noexcept;

private:
  /** Update sink rate at max. cruise speed */
  void UpdateSMax() noexcept;

  /** Solve for best LD at current MC/bugs/ballast setting. */
  void UpdateBestLD() noexcept;

  /** Solve for min sink rate at current bugs/ballast setting. */
  void UpdateSMin() noexcept;
};

static_assert(std::is_trivial<GlidePolar>::value, "type is not trivial");
