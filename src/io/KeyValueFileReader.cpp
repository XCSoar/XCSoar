// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "KeyValueFileReader.hpp"
#include "util/StringCompare.hxx"
#include "LineReader.hpp"

#include <string.h>

bool
KeyValueFileReader::Read(KeyValuePair &pair)
{
  char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (StringIsEmpty(line) || *line == '#')
      continue;

    char *p = strchr(line, '=');
    if (p == line || p == nullptr)
      continue;

    *p = '\0';
    char *value = p + 1;

    if (*value == '"') {
      ++value;
      p = strchr(value, '"');
      if (p == nullptr)
        continue;

      *p = '\0';
    }

    pair.key = line;
    pair.value = value;
    return true;
  }

  return false;
}
