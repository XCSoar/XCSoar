// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "KeyValueFileWriter.hpp"
#include "BufferedOutputStream.hxx"
#include "util/Macros.hpp"

#include <cassert>
#include <string.h>

#ifdef _UNICODE
#include <stringapiset.h>
#endif

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

#ifdef _UNICODE

void
KeyValueFileWriter::Write(const char *key, const TCHAR *value)
{
  char buffer[1024];
  int result = WideCharToMultiByte(CP_UTF8, 0, value, -1,
                                   buffer, ARRAY_SIZE(buffer),
                                   nullptr, nullptr);
  if (result > 0)
    Write(key, buffer);
}

#endif
