// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AirspaceInterceptSolution.hpp"
#include "AirspaceWarningInterval.hpp"
#include "Ptr.hpp"

#include <chrono>
#include <cstdint>

#ifdef DO_PRINT
#include <iosfwd>
#endif

class AbstractAirspace;

/**
 * Class to hold information about active airspace warnings
 */
class AirspaceWarning {
  using Duration = std::chrono::duration<unsigned>;

public:

  /**
   * Enumeration of airspace warning types
   */
  enum State : uint8_t {
    WARNING_CLEAR=0, /**< No warning active */
    WARNING_TASK, /**< Warning that task intersects airspace */
    WARNING_FILTER, /**< Warning that filtered state intersects airspace */
    WARNING_GLIDE, /**< Warning that short-term glide intersects airspace */
    WARNING_INSIDE /**< Warning that aircraft is currently inside airspace */
  };

private:
  const ConstAirspacePtr airspace;
  State state = WARNING_CLEAR;
  State state_last = WARNING_CLEAR;
  AirspaceInterceptSolution solution = AirspaceInterceptSolution::Invalid();

  Duration acktime_warning{};
  Duration acktime_inside{};
  Duration debounce_time = std::chrono::minutes{1};
  bool ack_day = false;
  bool cleared_day = false;
  bool is_exit_warning = false;
  bool covered_by_clearance = false;
  bool expired = true;
  bool expired_last = true;

  /**
   * Distance intervals along predicted path, one per
   * prediction method.  Reset each cycle in SaveState().
   */
  AirspaceWarningInterval interval_task_ =
    AirspaceWarningInterval::Invalid();
  AirspaceWarningInterval interval_filter_ =
    AirspaceWarningInterval::Invalid();
  AirspaceWarningInterval interval_glide_ =
    AirspaceWarningInterval::Invalid();

  static constexpr auto null_acktime = Duration::max();

public:
  /**
   * Constructor.  All warnings uniquely refer to an airspace.
   *
   * @param the_airspace Airspace that this object will manage warnings for
   */
  template<typename T>
  explicit AirspaceWarning(T &&_airspace) noexcept
    :airspace(std::forward<T>(_airspace)) {}

  AirspaceWarning(const AirspaceWarning &) noexcept = default;

  /**
   * Save warning state prior to performing update
   */
  void SaveState() noexcept;

  /**
   * Update warning state and solution vector
   *
   * @param state New warning state
   * @param solution Intercept vector (to outside if currently inside,
   * otherwise to inside)
   */
  void UpdateSolution(const State state,
                      const AirspaceInterceptSolution &_solution) noexcept;

  /**
   * Determine whether accepting a warning of the supplied state
   * will upgraded or equal the severity of the warning.
   *
   * @param state New warning state
   */
  [[gnu::pure]]
  bool IsStateAccepted(const State _state) const noexcept {
    return _state >= state;
  }

  /**
   * Determine whether during last update, the state of this warning
   * changed.
   *
   * @return True if state upgraded/downgraded
   */
  [[gnu::pure]]
  bool ChangedState() const noexcept;

  /**
   * Access airspace managed by this object
   *
   * @return Airspace
   */
  const AbstractAirspace &GetAirspace() const noexcept {
    return *airspace;
  }

  ConstAirspacePtr GetAirspacePtr() const noexcept {
    return airspace;
  }

  /**
   * Access warning state
   *
   * @return Warning state
   */
  State GetWarningState() const noexcept {
    return state;
  }

  /**
   * Is this a warning?
   *
   * Some instances are not actually warnings, but
   * how XCSoar remembers that an airspace is "ACKed" (but not
   * currently nearby).
   *
   * Note that this method returns true for "ACKed" warnings.
   */
  bool IsWarning() const noexcept {
    return state > WARNING_CLEAR;
  }

  /**
   * Note that this method returns true for "ACKed" inside warnings.
   */
  bool IsInside() const noexcept {
    return state == WARNING_INSIDE;
  }

  /**
   * Is this warning currently active, i.e. it has no valid "ACK"?
   * This implies that IsWarning() is true.
   */
  bool IsActive() const noexcept {
    return expired;
  }

  /**
   * Update warnings, calculate whether this airspace still needs to
   * have warnings managed.
   *
   * @param ack_time Lifetime of acknowledgements
   * @param dt time step (seconds)
   *
   * @return True if warning is still active
   */
  bool WarningLive(const Duration ack_time, const Duration dt) noexcept;

  /**
   * Access solution (nearest to enter, if outside, or to exit, if inside)
   *
   * @return Reference to solution
   */
  const AirspaceInterceptSolution &GetSolution() const noexcept {
    return solution;
  }

  /**
   * Determine if acknowledgement is expired (warning is current)
   *
   * @return True if acknowledgement is expired
   */
  [[gnu::pure]]
  bool IsAckExpired() const noexcept;

  /**
   * Determine if acknowledgement is acknowledged for whole day
   *
   * @return True if acknowledged
   */
  [[gnu::pure]]
  bool GetAckDay() const noexcept {
    return ack_day;
  }

  /**
   * Check if this warning has an explicit acknowledgement (ack_day or
   * a still-running timed ack), independent of covered_by_clearance.
   * Used by the renderer to distinguish "acked by the pilot" from
   * "silenced only because the path is covered by a clearance".
   */
  [[gnu::pure]]
  bool HasExplicitAck() const noexcept {
    return ack_day
      || acktime_warning.count() > 0
      || acktime_inside.count() > 0;
  }

  /**
   * Acknowledge an airspace warning or airspace inside (depending on
   * the state).
   */
  void Acknowledge() noexcept;

  /**
   * Acknowledge an airspace warning
   *
   * @param set Whether to set or cancel acknowledgement
   */
  void AcknowledgeWarning(const bool set=true) noexcept;

  /**
   * Acknowledge an airspace inside
   *
   * @param set Whether to set or cancel acknowledgement
   */
  void AcknowledgeInside(const bool set=true) noexcept;

  /**
   * Acknowledge all warnings for airspace for whole day
   *
   * @param set Whether to set or cancel acknowledgement
   */
  void AcknowledgeDay(const bool set=true) noexcept {
    ack_day = set;
  }

  /**
   * Set or revoke airspace clearance for the whole day.
   *
   * Clearance marks airspace you are allowed
   * to be inside. This suppresses entry and inside warnings
   * for both this airspace and all other airspaces at this location.
   * The warning system then alerts when approaching
   * the exit boundary if that leads into another airspace that is 
   * not cleared.
   *
   * @param set Whether to set or revoke clearance
   */
  void SetCleared(const bool set=true) noexcept {
    cleared_day = set;
  }

  /**
   * Determine if clearance is set
   */
  [[gnu::pure]]
  bool IsCleared() const noexcept {
    return cleared_day;
  }

  /**
   * Mark this warning as an exit warning (approaching exit
   * boundary of a cleared airspace into restricted airspace).
   *
   * This flag is recomputed each update cycle.
   */
  void SetExitWarning(const bool set) noexcept {
    is_exit_warning = set;
  }

  /**
   * Determine if this is an exit warning for a cleared airspace
   */
  [[gnu::pure]]
  bool IsExitWarning() const noexcept {
    return is_exit_warning;
  }

  /**
   * Mark this warning as covered by another airspace's
   * clearance.
   *
   * Warning object remains in manager, but
   * creates no warning message. IsAckExpired() returns 
   * false while the flag is set.
   */
  void SetCoveredByClearance(const bool set) noexcept {
    covered_by_clearance = set;
  }

  /**
   * Is this warning currently covered by another 
   * airspace's clearance?
   */
  [[gnu::pure]]
  bool IsCoveredByClearance() const noexcept {
    return covered_by_clearance;
  }

  /**
   * Set the distance interval for a prediction method.
   *
   * @param method WARNING_TASK, WARNING_FILTER, or WARNING_GLIDE
   */
  void SetInterval(State method,
                   const AirspaceWarningInterval &iv) noexcept;

  /**
   * Get the distance interval for a prediction method.
   */
  [[gnu::pure]]
  const AirspaceWarningInterval &GetInterval(
      State method) const noexcept;

  /**
   * Check whether a valid interval exists for a method.
   */
  [[gnu::pure]]
  bool HasInterval(State method) const noexcept;

  /**
   * Force warning state to a specific value.  Used only by
   * clearance post-processing to suppress fully-covered
   * warnings.
   */
  void ForceState(State s) noexcept {
    state = s;
  }

  /**
   * Replace the intercept solution directly.  Used by
   * clearance post-processing after interval subtraction
   * adjusts the effective warning point.
   */
  void SetSolution(
      const AirspaceInterceptSolution &s) noexcept {
    solution = s;
  }

  /**
   * Ranking operator for warnings
   *
   * @return True if this is more severe than that
   */
  [[gnu::pure]]
  bool operator<(const AirspaceWarning &that) const noexcept;

#ifdef DO_PRINT
public:
  friend std::ostream &operator<<(std::ostream &f,
                                  const AirspaceWarning &aw);
#endif
};
