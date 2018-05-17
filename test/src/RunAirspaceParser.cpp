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

#include "Airspace/AirspaceParser.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "OS/Args.hpp"
#include "IO/FileLineReader.hpp"
#include "Operation/Operation.hpp"
#include "Util/PrintException.hxx"

#include <stdio.h>
#include <tchar.h>

int main(int argc, char **argv)
try {
  Args args(argc, argv, "PATH");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  FileLineReader reader(path, Charset::AUTO);

  Airspaces airspaces;
  AirspaceParser parser(airspaces);

  NullOperationEnvironment operation;
  if (!parser.Parse(reader, operation)) {
    fprintf(stderr, "Failed to parse input file\n");
    return 1;
  }

  airspaces.Optimise();

  printf("OK\n");

  return EXIT_SUCCESS;
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
