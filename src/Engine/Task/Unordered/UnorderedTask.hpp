// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

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
  double ScanDistanceNominal() const noexcept override;
  double ScanDistancePlanned() noexcept override;
  double ScanDistanceRemaining(const GeoPoint &ref) noexcept override;
  double ScanDistanceScored(const GeoPoint &ref) noexcept override;
  double ScanDistanceTravelled(const GeoPoint &ref) noexcept override;
  void ScanDistanceMinMax(const GeoPoint &ref, bool full,
                          double *dmin, double *dmax) noexcept override;
  double ScanDistanceMaxTotal() noexcept override;
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
