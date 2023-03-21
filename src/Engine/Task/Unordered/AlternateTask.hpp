// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AbortTask.hpp"
#include "AlternateList.hpp"

/**
 * AlternateTask is a specialisation of AbortTask to add functionality
 * to find alternate landing points along the task.
 *
 * @todo: take user preferences of landing points into account.
 */
class AlternateTask final : public AbortTask
{
  struct Divert : public AlternatePoint {
    double delta;

    Divert(WaypointPtr &&_waypoint, const GlideResult &_solution,
           double _delta) noexcept
      :AlternatePoint(std::move(_waypoint), _solution), delta(_delta) {}
  };

  using DivertVector = std::vector<Divert>;

  /// number of alternates
  static constexpr DivertVector::size_type max_alternates = 6;

  AlternateList alternates;
  GeoPoint destination;

public:
  /** 
   * Base constructor.
   * 
   * @param tb Global task behaviour settings
   * @param wps Waypoints container to be scanned during updates
   * 
   * @return Initialised object (with nothing in task)
   */
  AlternateTask(const TaskBehaviour &tb,
                const Waypoints &wps) noexcept;

  /**
   * Sets the target of the task.
   * Must be called before running update_sample!
   */
  void SetTaskDestination(const GeoPoint &_destination) noexcept;

  /**
   * Retrieve a copy of the task alternates
   *
   * @param index Index sequence of alternate
   *
   * @return Vector of alternates
   */
  const AlternateList &GetAlternates() const noexcept {
    return alternates;
  }

private:
  /**
   * Determine if the candidate waypoint is already in the
   * alternate list.
   */
  [[gnu::pure]]
  bool IsWaypointInAlternates(const Waypoint &waypoint) const noexcept;

public:
  /* virtual methods from class AbstractTask */
  virtual void Reset() noexcept override;

protected:
  /* virtual methods from class AbortTask */
  virtual void Clear() noexcept override;
  virtual void ClientUpdate(const AircraftState &state_now,
                            bool reachable) noexcept override;
};
