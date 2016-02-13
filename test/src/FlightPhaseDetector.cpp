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

#include "FlightPhaseDetector.hpp"
#include "NMEA/CirclingInfo.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

static constexpr double MIN_PHASE_TIME(30);
static constexpr double FP_TOLERANCE(0.001);

/**
 * Update circling directon for given phase with given new direction
 *
 * If direction change is detected, phase direction will become MIXED.
 * Note, that direction will be set for circling phases only.
 */
static void
UpdateCirclingDirection(Phase &phase, const Phase::CirclingDirection dir)
{
  if (phase.phase_type != Phase::Type::CIRCLING) {
    phase.circling_direction = Phase::CirclingDirection::NO_DIRECTION;
    return;
  }

  if (dir == Phase::CirclingDirection::NO_DIRECTION) {
    return;
  }
  if (phase.circling_direction != dir) {
    if (phase.circling_direction == Phase::CirclingDirection::NO_DIRECTION) {
      phase.circling_direction = dir;
    }
    else {
      phase.circling_direction = Phase::CirclingDirection::MIXED;
    }
  }
}

static void
CombinePhases(Phase &base, const Phase &addition)
{
  UpdateCirclingDirection(base, addition.circling_direction);
  base.end_datetime = addition.end_datetime;
  base.end_time = addition.end_time;
  base.end_alt = addition.end_alt;
  base.end_loc = addition.end_loc;
  base.distance += addition.distance;
  base.duration += addition.duration;
  base.alt_diff += addition.alt_diff;
  base.merges++;
}

static Phase::Type
GetPhaseType(const DerivedInfo &calculated)
{
  switch (calculated.turn_mode) {
  case CirclingMode::CRUISE:
    return Phase::Type::CRUISE;
  case CirclingMode::CLIMB:
    return Phase::Type::CIRCLING;
  default:
    return Phase::Type::NO_PHASE;
  }
}

/**
 * Return true if current flight is appears to be powered, false otherwise
 */
static bool
IsPoweredFlight(const DerivedInfo &calculated)
{
  return calculated.flight.IsTowing();
}

static Phase::CirclingDirection
CalcCirclingDirection(const DerivedInfo &calculated)
{
  if (!calculated.circling) {
    return Phase::CirclingDirection::NO_DIRECTION;
  }
  return calculated.TurningLeft() ?
         Phase::CirclingDirection::LEFT : Phase::CirclingDirection::RIGHT;
}

double
Phase::GetSpeed() const {
  if (duration < FP_TOLERANCE) {
    return 0;
  }
  return distance / duration;
}

double
Phase::GetVario() const {
  if (duration < FP_TOLERANCE) {
    return 0;
  }
  return alt_diff / duration;
}

double
Phase::GetGlideRate() const {
  if (fabs(alt_diff) < FP_TOLERANCE) {
    return 0;
  }
  return distance / -alt_diff;
}

FlightPhaseDetector::FlightPhaseDetector() 
{
  previous_phase.Clear();
  current_phase.Clear();
  phase_count = 0;
}

void
FlightPhaseDetector::Update(const MoreData &basic, const DerivedInfo &calculated)
{
  bool mode_changed = last_turn_mode != calculated.turn_mode;
  bool lost_power = current_phase.phase_type == Phase::Type::POWERED &&
                    !IsPoweredFlight(calculated);

  bool phase_changed = mode_changed || lost_power;

  if (phase_changed) {
    if (current_phase.phase_type != Phase::Type::NO_PHASE) {
      PushPhase();
    }
    else {
      /* uncertain turn mode, try to determine it now and continue filling this
       * phase. */
      current_phase.phase_type = GetPhaseType(calculated);
    }
  }

  if (!current_phase.start_datetime.IsPlausible()) {
    // Initialize new phase.
    current_phase.phase_type = GetPhaseType(calculated);
    current_phase.end_loc = basic.location;
    /* First phase' start_attributes are initialized with factual data,
     * subsequent phases starts where previous one ended. */
    if (phase_count == 0) {
      current_phase.start_datetime = basic.date_time_utc;
      current_phase.start_time = basic.time;
      current_phase.start_loc = basic.location;
      current_phase.start_alt = basic.nav_altitude;
    }
    else {
      current_phase.start_datetime = previous_phase.end_datetime;
      current_phase.start_time = previous_phase.end_time;
      current_phase.start_loc = previous_phase.end_loc;
      current_phase.start_alt = previous_phase.end_alt;
    }
  }
  // Update the current phase with additional data
  current_phase.duration = basic.time - current_phase.start_time;
  current_phase.alt_diff = basic.nav_altitude - current_phase.start_alt;
  current_phase.distance += current_phase.end_loc.Distance(basic.location);
  current_phase.end_datetime = basic.date_time_utc;
  current_phase.end_time = basic.time;
  current_phase.end_loc = basic.location;
  current_phase.end_alt = basic.nav_altitude;
  UpdateCirclingDirection(current_phase, CalcCirclingDirection(calculated));
  // When powered flight is detected, current phase becomes "powered"
  if (IsPoweredFlight(calculated)) {
    current_phase.phase_type = Phase::Type::POWERED;
  }

  // detect powered flight

  // save last turn mode seen
  last_turn_mode = calculated.turn_mode;
}

void
FlightPhaseDetector::Finish()
{
  // Push last phases (we still have two latest phases in state)
  if (current_phase.phase_type == Phase::Type::NO_PHASE) {
    if (previous_phase.phase_type == Phase::Type::NO_PHASE)
      return;

    current_phase.phase_type = previous_phase.phase_type;
  }
  PushPhase();
  phases.push_back(previous_phase);

  // Calculate total statistics
  for (Phase ph : phases) {
    switch (ph.phase_type) {
    case Phase::Type::CIRCLING:
      CombinePhases(totals.total_circstats, ph);
      switch (ph.circling_direction) {
      case Phase::CirclingDirection::LEFT:
        CombinePhases(totals.left_circstats, ph);
        break;
      case Phase::CirclingDirection::RIGHT:
        CombinePhases(totals.right_circstats, ph);
        break;
      case Phase::CirclingDirection::MIXED:
        CombinePhases(totals.mixed_circstats, ph);
        break;
      case Phase::CirclingDirection::NO_DIRECTION:
        break;
      }
      break;
    case Phase::Type::CRUISE:
      CombinePhases(totals.total_cruisestats, ph);
      break;
    case Phase::Type::POWERED:
    case Phase::Type::NO_PHASE:
      break;
    }
  }

  // Calculate fractions
  auto total_circling = totals.total_circstats.duration;
  auto total_cruise = totals.total_cruisestats.duration;
  auto total_duration = total_circling + total_cruise;
  if (total_duration > 0) {
      totals.total_circstats.fraction = total_circling / total_duration;
      totals.total_cruisestats.fraction = total_cruise / total_duration;

      if (total_circling > 0) {
          totals.left_circstats.fraction =
              totals.left_circstats.duration / total_circling;
          totals.right_circstats.fraction =
              totals.right_circstats.duration / total_circling;
          totals.mixed_circstats.fraction =
              totals.mixed_circstats.duration / total_circling;
      }
  }
}

void
FlightPhaseDetector::PushPhase()
{
  assert(current_phase.phase_type != Phase::Type::NO_PHASE);

  if (phase_count == 0) {
    previous_phase = current_phase;
    phase_count++;
  }
  else {
    bool same_type = previous_phase.phase_type == current_phase.phase_type;
    bool too_short = current_phase.duration < MIN_PHASE_TIME;
    if (same_type || too_short) {
      CombinePhases(previous_phase, current_phase);
    }
    else {
      phases.push_back(previous_phase);
      phase_count++;
      previous_phase = current_phase;
    }
  }
  current_phase.Clear();
}
