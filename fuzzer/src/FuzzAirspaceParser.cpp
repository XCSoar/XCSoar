/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "io/MemoryReader.hxx"
#include "io/BufferedLineReader.hpp"
#include "system/Args.hpp"
#include "Operation/Operation.hpp"

#include <stdio.h>
#include <tchar.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  Airspaces airspaces;

  try {
    MemoryReader mr{{(const std::byte *)data, size}};
    BufferedLineReader lr(mr);
    NullOperationEnvironment operation;

    if (!ParseAirspaceFile(airspaces, lr, operation))
      return EXIT_FAILURE;
  } catch (...) {
    return EXIT_FAILURE;
  }

  airspaces.Optimise();

  return EXIT_SUCCESS;
}
