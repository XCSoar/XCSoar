// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "UnorderedTask.hpp"
#include "Engine/Waypoint/Ptr.hpp"

#include <memory>

class Waypoints;
class UnorderedTaskPoint;

/**
 * Class providing ability to go to a single task point
 */
class GotoTask final : public UnorderedTask
{
  std::unique_ptr<UnorderedTaskPoint> tp;
  const Waypoints &waypoints;

public:
  /** 
   * Base constructor.
   * 
   * @param tb Global task behaviour settings
   * @param wps Waypoints container to be scanned for takeoff
   * 
   * @return Initialised object (with no waypoint to go to)
   */

  GotoTask(const TaskBehaviour &tb,
           const Waypoints &wps);
  ~GotoTask();

  void SetTaskBehaviour(const TaskBehaviour &tb);

/** 
 * Sets go to task point to specified waypoint. 
 * Obeys TaskBehaviour.goto_nonlandable, won't do anything
 * if destination is not landable.
 * 
 * @param wp Waypoint to Go To
 * @return True if successful
 */
  bool DoGoto(WaypointPtr &&wp);

  /**
   * When called on takeoff, creates a default goto task
   *
   * @param loc Location of takeoff point
   * @param terrain_alt Terrain height at takeoff point
   *
   * @return True if default task was created
   */
  bool TakeoffAutotask(const GeoPoint &loc, double terrain_alt);

public:
  /* virtual methods from class TaskInterface */
  unsigned TaskSize() const noexcept override;
  TaskWaypoint *GetActiveTaskPoint() const noexcept override;
  void SetActiveTaskPoint(unsigned index) noexcept override;
  bool IsValidTaskPoint(const int index_offset) const noexcept override;

protected:
  bool UpdateSample(const AircraftState &state_now,
                    const GlidePolar &glide_polar,
                    bool full_update) noexcept override;
public:
  void AcceptTaskPointVisitor(TaskPointConstVisitor& visitor) const override;
};
