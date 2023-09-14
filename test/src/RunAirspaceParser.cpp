// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace/AirspaceParser.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "system/Args.hpp"
#include "io/FileLineReader.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <tchar.h>

int main(int argc, char **argv)
try {
  Args args(argc, argv, "PATH");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  FileLineReader reader(path, Charset::AUTO);

  Airspaces airspaces;

  ConsoleOperationEnvironment operation;
  ParseAirspaceFile(airspaces, reader, operation);

  airspaces.Optimise();

  printf("OK\n");

  return EXIT_SUCCESS;
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
