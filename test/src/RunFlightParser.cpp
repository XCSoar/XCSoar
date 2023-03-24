// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/Args.hpp"
#include "io/FileLineReader.hpp"
#include "Logger/FlightParser.hpp"
#include "FlightInfo.hpp"
#include "util/PrintException.hxx"

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
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
