// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
    BufferedReader br{mr};

    ParseAirspaceFile(airspaces, br);
  } catch (...) {
    return EXIT_FAILURE;
  }

  airspaces.Optimise();

  return EXIT_SUCCESS;
}
