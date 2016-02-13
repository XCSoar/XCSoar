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

#ifndef ALTERNATETASK_HPP
#define ALTERNATETASK_HPP

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
public:
  struct Divert : public AlternatePoint {
    double delta;

    Divert(WaypointPtr &&_waypoint, const GlideResult &_solution,
           double _delta)
      :AlternatePoint(std::move(_waypoint), _solution), delta(_delta) {}
  };

  typedef std::vector<Divert> DivertVector;

  /// number of alternates
  static constexpr unsigned max_alternates = 6;

private:
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
                const Waypoints &wps);

  /**
   * Sets the target of the task.
   * Must be called before running update_sample!
   */
  void SetTaskDestination(const GeoPoint &_destination);

  /**
   * Retrieve a copy of the task alternates
   *
   * @param index Index sequence of alternate
   *
   * @return Vector of alternates
   */
  const AlternateList &GetAlternates() const {
    return alternates;
  }

private:
  /**
   * Determine if the candidate waypoint is already in the
   * alternate list.
   */
  bool IsWaypointInAlternates(const Waypoint &waypoint) const;

public:
  /* virtual methods from class AbstractTask */
  virtual void Reset() override;

protected:
  /* virtual methods from class AbortTask */
  virtual void Clear() override;
  virtual void ClientUpdate(const AircraftState &state_now,
                            bool reachable) override;
};

#endif //ALTERNATETASK_HPP
