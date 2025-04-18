// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Logger/FlightLogger.hpp"
#include "system/Args.hpp"
#include "DebugReplay.hpp"

#include <stdio.h>

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE OUTFILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  FlightLogger logger;
  logger.SetPath(path);
  logger.Reset();

  while (replay->Next())
    logger.Tick(replay->Basic(), replay->Calculated());

  delete replay;
}
