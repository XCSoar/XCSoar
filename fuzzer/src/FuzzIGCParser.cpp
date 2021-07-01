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
