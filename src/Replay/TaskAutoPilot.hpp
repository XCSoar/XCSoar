// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"
#include "Math/Filter.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/FloatDuration.hxx"

#include <chrono>

struct AircraftState;

struct AutopilotParameters {
  double bearing_noise;
  double target_noise = 0.1;
  double turn_speed;
  double sink_factor = 1;
  double climb_factor = 1;
  double start_alt = 1500;
  bool enable_bestcruisetrack = false;
  bool goto_target = false;

  AutopilotParameters() noexcept
    {
      SetRealistic();
    };

  void SetIdeal();
  void SetRealistic();
};

class AbstractAutoPilot {
public:
  GeoPoint location_start;
  GeoPoint location_previous;
  Angle heading;

  void SetDefaultLocation(const GeoPoint& default_location) {
    location_start = default_location;
    location_previous = default_location;
    location_previous.latitude -= Angle::Degrees(1);
  }

protected:
  virtual void OnManualAdvance() {};
  virtual void OnModeChange() {};
  virtual void OnClose() {};
};

class TaskAccessor;

class TaskAutoPilot: public AbstractAutoPilot {
public:
  enum AcState {
    Climb = 0,
    Cruise,
    FinalGlide
  };

protected:
  AcState acstate;
  unsigned awp;

private:
  const AutopilotParameters &parms;
  Filter heading_filter;
  double climb_rate;
  double speed_factor;
  GeoPoint w[2];

public:
  TaskAutoPilot(const AutopilotParameters &_parms);

  void Start(const TaskAccessor& task);
  void UpdateMode(const TaskAccessor& task,
                  const AircraftState& state);

  void UpdateState(const TaskAccessor& task,
                   AircraftState &state,
                   FloatDuration timestep=std::chrono::seconds{1}) noexcept;

  bool UpdateAutopilot(TaskAccessor &task,
                       const AircraftState &state);

  [[gnu::pure]]
  GeoPoint GetTarget(const TaskAccessor& task) const;

  void SetSpeedFactor(double f) {
    speed_factor = f;
  }

private:
  bool DoAdvance(TaskAccessor& task);
  void AdvanceIfRequired(TaskAccessor& task);

  [[gnu::pure]]
  bool HasFinished(const TaskAccessor &task) const;

  void GetAWP(const TaskAccessor &task);

  [[gnu::pure]]
  bool HasTarget(const TaskAccessor& task) const;

  GeoPoint GetStartLocation(const TaskAccessor& task,
                            bool previous = false);

  void UpdateCruiseBearing(const TaskAccessor& task, const AircraftState& state,
                           FloatDuration timestep) noexcept;

  double GetTargetHeight(const TaskAccessor &task) const;
  Angle GetHeadingDeviation();
  bool UpdateComputer(TaskAccessor &task, const AircraftState& state);
  bool IsFarFromTarget(const TaskAccessor& task,
                       const AircraftState& state);
};


