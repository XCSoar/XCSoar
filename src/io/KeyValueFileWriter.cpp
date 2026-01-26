// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "KeyValueFileWriter.hpp"
#include "BufferedOutputStream.hxx"
#include "util/Macros.hpp"

#include <cassert>
#include <string.h>

void
KeyValueFileWriter::Write(const char *key, const char *value)
{
  assert(key != nullptr);
  assert(value != nullptr);

  // does it contain invalid characters?
  if (strpbrk(value, "\r\n\"") != nullptr)
    // -> write ="" to the output file an continue with the next subkey
    value = "";

  // write the value to the output file
  os.Fmt("{}=\"{}\"\n", key, value);
}
