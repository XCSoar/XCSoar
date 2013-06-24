/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef FINISHPOINT_HPP
#define FINISHPOINT_HPP

#include "OrderedTaskPoint.hpp"
#include "Task/Ordered/FinishConstraints.hpp"

struct FinishConstraints;

/**
 * A FinishPoint is an abstract OrderedTaskPoint,
 * can manage finish transitions
 * but does not yet have an observation zone.
 * No taskpoints shall be present following a FinishPoint.
 *
 * Entry requires previous point to have entered to prevent spurious crossing.
 */
class FinishPoint final : public OrderedTaskPoint
{
  fixed safety_height;

  /**
   * A copy of OrderedTaskSettings::finish_constraints, managed by
   * SetOrderedTaskSettings().
   */
  FinishConstraints constraints;

  fixed fai_finish_height;

public:
  /**
   * Constructor.  Sets task area to non-scorable; distances
   * are relative to crossing point or origin.
   *
   * @param _oz Observation zone for this task point
   * @param wp Waypoint origin of turnpoint
   * @param tb Task Behaviour defining options (esp safety heights)
   * @param to OrderedTask Behaviour defining options
   *
   * @return Partially-initialised object
   */
  FinishPoint(ObservationZonePoint *_oz, const Waypoint &wp,
              const TaskBehaviour &tb,
              const FinishConstraints &constraints,
              bool boundary_scored=false);

  /**
   * Set FAI finish height
   *
   * @param height FAI finish height (m)
   */
  void set_fai_finish_height(const fixed height);

  /* virtual methods from class TaskPoint */
  virtual fixed GetElevation() const override;

  /* virtual methods from class ScoredTaskPoint */
  virtual void Reset() override;
  virtual bool CheckEnterTransition(const AircraftState &ref_now,
                                    const AircraftState &ref_last) const override;

  /* virtual methods from class OrderedTaskPoint */
  virtual void SetTaskBehaviour(const TaskBehaviour &tb) override;
  virtual void SetOrderedTaskSettings(const OrderedTaskSettings &otb) override;
  virtual void SetNeighbours(OrderedTaskPoint *prev,
                             OrderedTaskPoint *next) override;
  virtual bool IsInSector(const AircraftState &ref) const override;

private:
  /* virtual methods from class ScoredTaskPoint */
  virtual bool EntryPrecondition() const override;
  virtual bool ScoreFirstEntry() const override {
    return true;
  }

  /**
   * called by isInSector
   * @param state
   * @return true if state is within valid height for sector
   */
  bool is_in_height_limit(const AircraftState &state) const;
};

#endif
