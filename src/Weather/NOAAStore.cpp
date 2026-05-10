// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOAAStore.hpp"
#include "util/CharUtil.hxx"

bool
NOAAStore::IsValidCode(const char *code)
{
  for (unsigned i = 0; i < 4; ++i)
    if (!IsAlphaNumericASCII(code[i]))
      return false;

  if (code[4] != 0)
    return false;

  return true;
}

NOAAStore::iterator
NOAAStore::AddStation(const char *code)
{
  assert(IsValidCode(code));

  Item item;

  for (unsigned i = 0; i < 4; ++i)
    item.code[i] = ToUpperASCII(code[i]);
  item.code[4] = 0;

  // Reset available flags
  item.metar_available = false;
  item.parsed_metar_available = false;
  item.taf_available = false;

  stations.push_back(item);
  return --end();
}
