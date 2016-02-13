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
  virtual bool CheckTask() const override;
  virtual bool CheckTransitions(const AircraftState &state_now,
                                const AircraftState &state_last) override;
  virtual bool CalcBestMC(const AircraftState &state_now,
                          const GlidePolar &glide_polar,
                          double& best) const override;
  virtual double CalcRequiredGlide(const AircraftState &state_now,
                                   const GlidePolar &glide_polar) const override;
  virtual double CalcGradient(const AircraftState &state_now) const override;
  virtual double ScanTotalStartTime() override;
  virtual double ScanLegStartTime() override;
  virtual double ScanDistanceNominal() override;
  virtual double ScanDistancePlanned() override;
  virtual double ScanDistanceRemaining(const GeoPoint &ref) override;
  virtual double ScanDistanceScored(const GeoPoint &ref) override;
  virtual double ScanDistanceTravelled(const GeoPoint &ref) override;
  virtual void ScanDistanceMinMax(const GeoPoint &ref, bool full,
                                  double *dmin, double *dmax) override;
  virtual void GlideSolutionRemaining(const AircraftState &state_now,
                                      const GlidePolar &polar,
                                      GlideResult &total, GlideResult &leg) override;
  virtual void GlideSolutionTravelled(const AircraftState &state_now,
                                      const GlidePolar &glide_polar,
                                      GlideResult &total, GlideResult &leg) override;
  virtual void GlideSolutionPlanned(const AircraftState &state_now,
                                    const GlidePolar &glide_polar,
                                    GlideResult &total,
                                    GlideResult &leg,
                                    DistanceStat &total_remaining_effective,
                                    DistanceStat &leg_remaining_effective,
                                    const GlideResult &solution_remaining_total,
                                    const GlideResult &solution_remaining_leg) override;
  virtual bool IsScored() const override { return false; }
};

#endif
