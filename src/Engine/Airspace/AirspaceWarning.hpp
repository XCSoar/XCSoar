/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef AIRSPACE_WARNING_HPP
#define AIRSPACE_WARNING_HPP

#include "AirspaceInterceptSolution.hpp"
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
  bool expired = true;
  bool expired_last = true;

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

#endif
