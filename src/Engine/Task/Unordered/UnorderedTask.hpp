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
#ifndef UNORDEREDTASK_H
#define UNORDEREDTASK_H

#include "Task/AbstractTask.hpp"

/**
 *  Common class for all unordered task types
 */
class UnorderedTask : public AbstractTask {
public:
  /**
   * Base constructor.
   *
   * @param tb Global task behaviour settings
   */
  UnorderedTask(TaskType _type,
                const TaskBehaviour &tb);

public:
  /* virtual methods from class AbstractTask */
  TaskValidationErrorSet CheckTask() const noexcept override;
  bool CheckTransitions(const AircraftState &state_now,
                        const AircraftState &state_last) noexcept override;
  bool CalcBestMC(const AircraftState &state_now,
                  const GlidePolar &glide_polar,
                  double& best) const noexcept override;
  double CalcRequiredGlide(const AircraftState &state_now,
                           const GlidePolar &glide_polar) const noexcept override;
  double CalcGradient(const AircraftState &state_now) const noexcept override;
  TimeStamp ScanTotalStartTime() noexcept override;
  TimeStamp ScanLegStartTime() noexcept override;
  double ScanDistanceNominal() noexcept override;
  double ScanDistancePlanned() noexcept override;
  double ScanDistanceRemaining(const GeoPoint &ref) noexcept override;
  double ScanDistanceScored(const GeoPoint &ref) noexcept override;
  double ScanDistanceTravelled(const GeoPoint &ref) noexcept override;
  void ScanDistanceMinMax(const GeoPoint &ref, bool full,
                          double *dmin, double *dmax) noexcept override;
  void GlideSolutionRemaining(const AircraftState &state_now,
                              const GlidePolar &polar,
                              GlideResult &total, GlideResult &leg) noexcept override;
  void GlideSolutionTravelled(const AircraftState &state_now,
                              const GlidePolar &glide_polar,
                              GlideResult &total, GlideResult &leg) noexcept override;
  void GlideSolutionPlanned(const AircraftState &state_now,
                            const GlidePolar &glide_polar,
                            GlideResult &total,
                            GlideResult &leg,
                            DistanceStat &total_remaining_effective,
                            DistanceStat &leg_remaining_effective,
                            const GlideResult &solution_remaining_total,
                            const GlideResult &solution_remaining_leg) noexcept override;
  bool IsScored() const noexcept override { return false; }
};

#endif
