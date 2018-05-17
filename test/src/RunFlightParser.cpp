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

#include "OS/Args.hpp"
#include "IO/FileLineReader.hpp"
#include "Logger/FlightParser.hpp"
#include "FlightInfo.hpp"
#include "Util/PrintException.hxx"

static void
Print(const FlightInfo &flight)
{
  if (flight.date.IsPlausible())
    printf("%04u-%02u-%02u ", flight.date.year,
           flight.date.month, flight.date.day);
  else
    printf("xxxx-xx-xx ");

  if (flight.start_time.IsPlausible())
    printf("%02u:%02u ", flight.start_time.hour, flight.start_time.minute);
  else
    printf("xx-xx ");

  if (flight.end_time.IsPlausible())
    printf("%02u:%02u", flight.end_time.hour, flight.end_time.minute);
  else
    printf("xx-xx");

  printf("\n");
}

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "PATH");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  FileLineReaderA file(path);
  FlightParser parser(file);
  FlightInfo flight;
  while (parser.Read(flight))
    Print(flight);

  return EXIT_SUCCESS;
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
