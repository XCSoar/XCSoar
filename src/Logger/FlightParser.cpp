/*
Copyright_License {

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

#include "FlightParser.hpp"
#include "IO/LineReader.hpp"
#include "FlightInfo.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Util/StringAPI.hxx"

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
        int duration = dt - BrokenDateTime(flight.date, flight.start_time);
        if (duration >= 0 && duration <= 14 * 60 * 60) {
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
