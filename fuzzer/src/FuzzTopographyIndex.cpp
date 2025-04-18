// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/Index.hpp"
#include "util/ScopeExit.hxx"

#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  std::string line{(const char *)data, size};
  ParseTopographyIndexLine(line.c_str());
  return EXIT_SUCCESS;
}
