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

#include "TaskAutoPilot.hpp"
#include "TaskAccessor.hpp"
#include "Task/Stats/ElementStat.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Util/Clamp.hpp"

#include <stdlib.h>

void
AutopilotParameters::SetIdeal()
{
  bearing_noise = 0;
  turn_speed = 90.0;
}

void
AutopilotParameters::SetRealistic()
{
  bearing_noise = 20;
  turn_speed = 12;
}

TaskAutoPilot::TaskAutoPilot(const AutopilotParameters &_parms)
  :parms(_parms), heading_filter(8), climb_rate(2.0), speed_factor(1.0) {}

void
TaskAutoPilot::Start(const TaskAccessor& task)
{
  // construct list of places we will fly to.
  // we dont do this dynamically so it is remembered even if
  // the task is advanced/retreated.

  if (task.IsOrdered()) {
    // construct list

    // this pilot is inaccurate, he flies to a random point in the OZ,
    // and starts in the start OZ.

    w[1] = task.GetRandomOZPoint(0, parms.target_noise);
    w[0] = task.GetRandomOZPoint(1, parms.target_noise);

  } else {
    // for non-ordered tasks, start at the default location

    w[1] = location_start;
    if (task.IsEmpty())
      // go somewhere nearby
      w[0] = location_previous;
    else
      // go directly to the target
      w[0] = task.GetRandomOZPoint(0, parms.target_noise);
  }

  // pick up the locations from the task to be used to initialise
  // the aircraft simulator

  location_start = GetStartLocation(task);
  location_previous = GetStartLocation(task, false);

  awp = 0;

  // reset the heading
  heading = Angle::Zero();
  heading_filter.Reset(0);

  acstate = Cruise;
}

GeoPoint
TaskAutoPilot::GetStartLocation(const TaskAccessor& task, bool previous)
{
  if (!previous && (task.IsOrdered())) {
    // set start location to 200 meters directly behind start
    // (otherwise start may fail to trigger)
    Angle brg = w[0].Bearing(w[1]);
    return GeoVector(200, brg).EndPoint(w[1]);
  }

  return w[0];
}

bool
TaskAutoPilot::HasTarget(const TaskAccessor& task) const
{
  return parms.goto_target && awp > 0;
}

GeoPoint
TaskAutoPilot::GetTarget(const TaskAccessor& task) const
{
  if (HasTarget(task))
    // in this mode, we go directly to the target
    return task.GetActiveTaskPointLocation();
  else
    // head towards the rough location
    return w[0];
}


Angle
TaskAutoPilot::GetHeadingDeviation()
{
  auto noise_mag = acstate == Climb
    ? parms.bearing_noise / 2.
    : parms.bearing_noise;
  auto r = (2 * rand() / RAND_MAX) - 1;
  auto deviation = heading_filter.Update(noise_mag * r);
  return Angle::Degrees(deviation).AsDelta();
}

double
TaskAutoPilot::GetTargetHeight(const TaskAccessor& task) const
{
  return task.GetTargetHeight();
}

void
TaskAutoPilot::UpdateMode(const TaskAccessor& task, const AircraftState& state)
{
  switch (acstate) {
  case Cruise:
    /* XXX this condition is probably broken */
    if (awp > 0 && task.GetRemainingAltitudeDifference() >= 0) {
      acstate = FinalGlide;
      OnModeChange();
    } else if (state.altitude <= GetTargetHeight(task)) {
      acstate = Climb;
      OnModeChange();
    }
    break;
  case FinalGlide:
    if (task.GetRemainingAltitudeDifference() < -20) {
      acstate = Climb;
      OnModeChange();
    }
    break;
  case Climb:
    /* XXX this condition is probably broken */
    if (awp > 0 && task.GetRemainingAltitudeDifference() >= 0) {
      acstate = FinalGlide;
      OnModeChange();
    } else if (state.altitude >= 1500) {
      acstate = Cruise;
      OnModeChange();
    }
    break;
  };
}

void
TaskAutoPilot::UpdateCruiseBearing(const TaskAccessor& task,
                                   const AircraftState& state,
                                   const double timestep)
{
  const ElementStat &stat = task.GetLegStats();
  Angle bct = stat.solution_remaining.cruise_track_bearing;
  Angle bearing;

  if (HasTarget(task)) {
    bearing = stat.solution_remaining.vector.bearing;

    if (parms.enable_bestcruisetrack &&
        stat.solution_remaining.vector.distance > 1000)
      bearing = bct;

  } else {
    bearing = state.location.Bearing(GetTarget(task));
  }

  if (state.wind.norm > 0 && state.true_airspeed > 0) {
    const auto sintheta = (state.wind.bearing - bearing).sin();
    if (fabs(sintheta) > 0.0001)
      bearing +=
        Angle::asin(sintheta * state.wind.norm / state.true_airspeed);
  }

  auto diff = (bearing - heading).AsDelta();
  auto d = diff.Degrees();
  auto max_turn = parms.turn_speed * timestep;
  heading += Angle::Degrees(Clamp(d, -max_turn, max_turn));
  if (parms.bearing_noise > 0)
    heading += GetHeadingDeviation() * timestep;

  heading = heading.AsBearing();
}

void
TaskAutoPilot::UpdateState(const TaskAccessor& task, AircraftState& state,
                           const double timestep)
{
  const GlidePolar &glide_polar = task.GetGlidePolar();

  switch (acstate) {
  case Cruise:
  case FinalGlide:
  {
    const ElementStat &stat = task.GetLegStats();
    if (stat.solution_remaining.v_opt > 0)
      state.true_airspeed = stat.solution_remaining.v_opt * speed_factor;
    else
      state.true_airspeed = glide_polar.GetVBestLD();

    state.vario = -glide_polar.SinkRate(state.true_airspeed) * parms.sink_factor;
    UpdateCruiseBearing(task, state, timestep);
    break;
  }
  case Climb: {
    state.true_airspeed = glide_polar.GetVMin();
    auto d = parms.turn_speed * timestep;
    if (d < 360)
      heading += Angle::Degrees(d);

    if (parms.bearing_noise > 0)
      heading += GetHeadingDeviation() * timestep;

    heading = heading.AsBearing();
    state.vario = climb_rate * parms.climb_factor;
    break;
  }
  }

  state.netto_vario = state.vario + glide_polar.SinkRate(state.true_airspeed);
}

bool
TaskAutoPilot::IsFarFromTarget(const TaskAccessor& task,
                               const AircraftState& state)
{
  // are we considered close to the target?

  if (task.IsEmpty() || !task.GetLegStats().remaining.IsDefined())
    return w[0].DistanceS(state.location) > state.ground_speed;

  bool d_far = task.GetLegStats().remaining.GetDistance() > 100;

  if (!task.IsOrdered())
    // cheat for non-ordered tasks
    return d_far;

  bool entered = awp >= task.size() || task.HasEntered(awp);

  if (HasTarget(task))
    return d_far || !entered;

  auto dc = w[0].DistanceS(state.location);
  if (awp == 0)
    return (dc > state.ground_speed);

  return dc > state.ground_speed || !entered;
}

bool
TaskAutoPilot::DoAdvance(TaskAccessor& task)
{
  if (task.IsOrdered() && awp == 0)
    awp++;

  awp++;

  if (HasFinished(task))
    return false;

  task.SetActiveTaskPoint(awp);
  GetAWP(task);

  return true;
}

bool
TaskAutoPilot::HasFinished(const TaskAccessor &task) const
{
  if (task.IsFinished())
    return true;

  if (task.IsOrdered())
    return awp >= task.size();
  else
    return awp >= 1;
}

void
TaskAutoPilot::AdvanceIfRequired(TaskAccessor& task)
{
  bool manual_start = false;

  if (task.IsStarted() && (task.GetActiveTaskPointIndex() == 0)) {
    manual_start = true;
    awp++;
  }

  if (HasTarget(task) || manual_start) {
    if (task.GetActiveTaskPointIndex() < awp) {
      // manual advance
      task.SetActiveTaskPoint(awp);
      OnManualAdvance();
      GetAWP(task);
    }
  }

  if (task.GetActiveTaskPointIndex() > awp) {
    awp = task.GetActiveTaskPointIndex();
    GetAWP(task);
  }
}

void
TaskAutoPilot::GetAWP(const TaskAccessor &task)
{
  w[0] = task.GetRandomOZPoint(awp, parms.target_noise);
}

bool
TaskAutoPilot::UpdateComputer(TaskAccessor& task, const AircraftState& state)
{
  if (!IsFarFromTarget(task, state)) {
    OnClose();
    return DoAdvance(task);
  }

  AdvanceIfRequired(task);

  return !HasFinished(task);
}

bool
TaskAutoPilot::UpdateAutopilot(TaskAccessor &task, const AircraftState &state)
{
  UpdateMode(task, state);
  return UpdateComputer(task, state);
}

