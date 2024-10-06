// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LiftDatabaseComputer.hpp"
#include "Engine/Navigation/TraceHistory.hpp"
#include "NMEA/LiftDatabase.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/CirclingInfo.hpp"

#include <algorithm> // for std::clamp()
#include <numeric>
#include <chrono>

void
LiftDatabaseComputer::Clear(LiftDatabase &lift_database,
                            TraceVariableHistory &circling_average_trace)
{
  lift_database.Clear();
  circling_average_trace.clear();
}

void
LiftDatabaseComputer::Clear(TraceVariableHistory &TurnAverage)
{
  TurnAverage.clear();
}

void
LiftDatabaseComputer::Reset(LiftDatabase &lift_database,
                            TraceVariableHistory &circling_average_trace,
                            TraceVariableHistory &TurnAverage)
{
  last_circling = false;
  last_heading = Angle::Zero();

  Clear(lift_database, circling_average_trace);
  Clear(TurnAverage);
}

/**
 * This function converts a heading into an unsigned index for the LiftDatabase.
 *
 * This is calculated with Angles to deal with the 360 degree limit.
 *
 * 357 = 0
 * 4 = 0
 * 5 = 1
 * 14 = 1
 * 15 = 2
 * ...
 * @param heading The heading to convert
 * @return The index for the LiftDatabase array
 */
static unsigned
heading_to_index(const Angle heading)
{
  static constexpr Angle afive = Angle::Degrees(5);

  unsigned index = (unsigned)
      ((heading + afive).AsBearing().Degrees() / 10);

  return std::clamp(index, 0u, 35u);
}

void
LiftDatabaseComputer::Compute(LiftDatabase &lift_database,
                              TraceVariableHistory &circling_average_trace,
                              const MoreData &basic,
                              const CirclingInfo &circling_info)
{
  // If we just started circling
  // -> reset the database because this is a new thermal
  if (!circling_info.circling && last_circling)
    Clear(lift_database, circling_average_trace);

  // Determine the direction in which we are circling
  bool left = circling_info.TurningLeft();

  // Depending on the direction set the step size sign for the
  // following loop
  Angle heading_step = left ? Angle::Degrees(-10) : Angle::Degrees(10);

  const Angle heading = basic.attitude.heading;

  // Start at the last heading and add heading_step until the current heading
  // is reached. For each heading save the current lift value into the
  // LiftDatabase. Last and current heading are included since they are
  // a part of the ten degree interval most of the time.
  //
  // This is done with Angles to deal with the 360 degrees limit.
  // e.g. last heading 348 degrees, current heading 21 degrees
  //
  // The loop condition stops until the current heading is reached.
  // Depending on the circling direction the current heading will be
  // smaller or bigger then the last one, because of that IsNegative() is
  // tested against the left variable.
  for (Angle h = last_heading;
       left == (heading - h).AsDelta().IsNegative();
       h += heading_step) {
    unsigned index = heading_to_index(h);
    lift_database[index] = basic.brutto_vario;
  }

  // detect zero crossing
  if ((heading < Angle::QuarterCircle() &&
       last_heading.Degrees() > 270) ||
      (last_heading < Angle::QuarterCircle() &&
       heading.Degrees() > 270)) {

    double h_av = 0;
    for (auto i : lift_database)
      h_av += i;

    h_av /= lift_database.size();
    circling_average_trace.push(h_av);
  }

  last_circling = circling_info.circling;
  last_heading = basic.attitude.heading;
}

void LiftDatabaseComputer::UpdateLastTurn(const Angle &current_heading,
                                          double current_time,
                                          double current_brutto_vario,
                                          const Angle &smoothed_turn_rate)
{
  // Calculate heading delta
  Angle heading_delta;
  if (heading_buffer.empty())
  {
    heading_delta = Angle::Zero();
  }
  else
  {
    Angle last_heading = heading_buffer.back();
    double delta = current_heading.Native() - last_heading.Native();
    // Adjust delta to be between -pi and pi
    if (delta > M_PI)
    {
      delta -= 2 * M_PI;
    }
    else if (delta < -M_PI)
    {
      delta += 2 * M_PI;
    }
    heading_delta = Angle::Native(delta); // Create Angle object directly
  }

  printf("current_heading = %f\n", current_heading.Degrees());
  printf("heading_delta = %f\n", heading_delta.Degrees());

  // Add current values to buffers
  time_buffer.push_back(current_time);
  heading_buffer.push_back(current_heading); // Store the current heading
  heading_delta_buffer.push_back(heading_delta);
  brutto_vario_buffer.push_back(current_brutto_vario);
  smoothed_turn_rate_buffer.push_back(smoothed_turn_rate);

 while (!time_buffer.empty() && (time_buffer.back() - time_buffer.front() > 40.0)) {
    time_buffer.pop_front();
    heading_delta_buffer.pop_front();
    brutto_vario_buffer.pop_front();
    smoothed_turn_rate_buffer.pop_front();
    heading_buffer.pop_front();
}

  // Check if a full turn has been made
  double total_heading_delta = std::accumulate(heading_delta_buffer.begin(), heading_delta_buffer.end(), Angle::Zero()).Degrees();
  if (total_heading_delta >= 360.0 || total_heading_delta <= -360.0)
  {

    // Remove oldest values from buffers until less than a full turn remains
    while (total_heading_delta >= 360.0 || total_heading_delta <= -360.0)
    {
      if(total_heading_delta >= 360.0)
        total_heading_delta -= heading_delta_buffer.front().Degrees();
      else
        total_heading_delta += heading_delta_buffer.front().Degrees();

      time_buffer.pop_front();
      heading_delta_buffer.pop_front();
      brutto_vario_buffer.pop_front();
      smoothed_turn_rate_buffer.pop_front();
      heading_buffer.pop_front();
    }
  }
  else
  {
    while (!time_buffer.empty() && smoothed_turn_rate_buffer.front().Degrees() < 10 && (time_buffer.back() - time_buffer.front() > 8.0)) {
    time_buffer.pop_front();
    heading_delta_buffer.pop_front();
    brutto_vario_buffer.pop_front();
    smoothed_turn_rate_buffer.pop_front();
    heading_buffer.pop_front();
    }
  }

 double recent_heading_delta = 0.0;
  for (auto it = time_buffer.rbegin(); it != time_buffer.rend() && *it > current_time - 5.0; ++it) {
      size_t index = std::distance(it, time_buffer.rend()) - 1;
      recent_heading_delta += heading_delta_buffer[index].Degrees();
  }

  // If the total heading delta is less than 20 degrees, clear the buffers to 8 seconds long
  if (recent_heading_delta < 20.0) {
      while (!time_buffer.empty() && (current_time - time_buffer.front() > 8.0)) {
          time_buffer.pop_front();
          heading_delta_buffer.pop_front();
          brutto_vario_buffer.pop_front();
          smoothed_turn_rate_buffer.pop_front();
          heading_buffer.pop_front();
      }
  }

  printf("smoothed_turn_rate = %f\n", smoothed_turn_rate.Degrees());
  printf("total_heading_delta = %f\n", total_heading_delta);
  printf("recent_heading_delta = %f\n", recent_heading_delta);
}

void LiftDatabaseComputer::Compute(int &turn_time,
                                   TraceVariableHistory &TurnAverage,
                                   const MoreData &basic,
                                   const CirclingInfo &circling_info)
{
  // If we just started circling
  // -> reset the database because this is a new thermal
  if (!circling_info.circling && last_circling)
  {
    Clear(TurnAverage);
    
  }

  Angle heading = basic.attitude.heading;

  // Get current time in seconds
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  double current_time = std::chrono::duration<double>(duration).count();

  UpdateLastTurn(heading, current_time, basic.brutto_vario, circling_info.turn_rate_heading_smoothed);

  double last_turn_time = time_buffer.back() - time_buffer.front();
  double average_brutto_vario = std::accumulate(brutto_vario_buffer.begin(), brutto_vario_buffer.end(), 0.0) / brutto_vario_buffer.size();

  printf("time_buffer.size() = %d\n", time_buffer.size());
  printf("last turn time = %f\n", last_turn_time);
  printf("average brutto vario = %f\n", average_brutto_vario);

  TurnAverage.push(average_brutto_vario);
  turn_time = last_turn_time;
}
