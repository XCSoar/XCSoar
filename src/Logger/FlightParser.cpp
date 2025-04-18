// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightParser.hpp"
#include "io/LineReader.hpp"
#include "FlightInfo.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StringAPI.hxx"

#include <stdio.h>

bool
FlightParser::Read(FlightInfo &flight)
{
  flight.date = BrokenDate::Invalid();
  flight.start_time = BrokenTime::Invalid();
  flight.end_time = BrokenTime::Invalid();

  while (true) {
    BrokenDateTime dt;
    char *line = ReadLine(dt);
    if (line == nullptr)
      return flight.start_time.IsPlausible();

    if (StringIsEqual(line, "start")) {
      if (flight.start_time.IsPlausible()) {
        /* this is the next flight: unread this line, return */
        last = current_line;
        return true;
      }

      flight.date = dt;
      flight.start_time = dt;
    } else if (StringIsEqual(line, "landing")) {
      if (flight.date.IsPlausible()) {
        // we have a start date/time
        const auto duration = dt - BrokenDateTime(flight.date, flight.start_time);
        if (duration.count() >= 0 && duration <= std::chrono::hours{14}) {
          // landing entry is likely belonging to start entry
          flight.end_time = dt;
        } else {
          // landing time only since duration is improbable
          last = current_line;
        }
        return true;
      } else
        flight.date = dt;

      flight.end_time = dt;
      return true;
    }
  }
}

inline char *
FlightParser::ReadLine()
{
  if (last != nullptr) {
    char *result = last;
    last = nullptr;
    return result;
  } else
    return reader.ReadLine();
}

inline char *
FlightParser::ReadLine(BrokenDateTime &dt)
{
  char *line;
  while ((line = ReadLine()) != nullptr) {
    current_line = line;
    char *space = strchr(line, ' ');
    if (space == nullptr)
      continue;

    unsigned year, month, day, hour, minute, second;
    int result = sscanf(line, "%04u-%02u-%02uT%02u:%02u:%02u",
                        &year, &month, &day, &hour, &minute, &second);
    if (result == 6) {
      dt.year = year;
      dt.month = month;
      dt.day = day;
      dt.hour = hour;
      dt.minute = minute;
      dt.second = second;

      if (dt.IsPlausible())
        return space + 1;
    }
  }

  return nullptr;
}
