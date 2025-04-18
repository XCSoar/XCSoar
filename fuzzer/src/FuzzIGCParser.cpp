// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IGC/IGCParser.hpp"
#include "IGC/IGCExtensions.hpp"
#include "IGC/IGCHeader.hpp"
#include "IGC/IGCFix.hpp"
#include "IGC/IGCDeclaration.hpp"
#include "io/MemoryReader.hxx"
#include "io/BufferedLineReader.hpp"

#include <cstdint>

#include <stdio.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  try {
    MemoryReader mr{{(const std::byte *)data, size}};
    BufferedLineReader lr(mr);

    IGCExtensions extensions{};
    while (const char *line = lr.ReadLine()) {
      IGCHeader header;
      IGCParseHeader(line, header);

      BrokenDate date;
      IGCParseDateRecord(line, date);

      IGCParseExtensions(line, extensions);

      GeoPoint location;
      IGCParseLocation(line, location);

      IGCFix fix;
      IGCParseFix(line, extensions, fix);

      BrokenTime time;
      IGCParseTime(line, time);

      IGCDeclarationHeader declaration_header;
      IGCParseDeclarationHeader(line, declaration_header);

      IGCDeclarationTurnpoint tp;
      IGCParseDeclarationTurnpoint(line, tp);
    }
  } catch (...) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
