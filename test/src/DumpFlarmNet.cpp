// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/FlarmNetReader.hpp"
#include "FLARM/FlarmNetDatabase.hpp"
#include "system/Args.hpp"

#include <stdlib.h>

int main(int argc, char **argv)
{
  Args args(argc, argv, "FILE");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  FlarmNetDatabase database;
  FlarmNetReader::LoadFile(path, database);

  for (auto i = database.begin(), end = database.end(); i != end; ++i) {
    const FlarmNetRecord &record = i->second;

    _tprintf(_T("%s\t%s\t%s\t%s\n"),
             record.id.c_str(), record.pilot.c_str(),
             record.registration.c_str(), record.callsign.c_str());
  }

  return EXIT_SUCCESS;
}
