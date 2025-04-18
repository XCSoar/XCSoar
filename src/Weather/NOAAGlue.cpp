// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOAAGlue.hpp"
#include "NOAAStore.hpp"
#include "Profile/Profile.hpp"

#include <algorithm>

NOAAStore *noaa_store;

bool
NOAAStore::LoadFromString(const char *string)
{
  const char *s = string;
  while (s != NULL && *s) {
    const char *next = strchr(s, _T(','));
    if ((next != NULL && next - s == 4) || (next == NULL && strlen(s) == 4)) {
      char code[5];
      std::copy_n(s, 4, code);
      code[4] = '\0';
      if (IsValidCode(code))
        AddStation(code);
    }
    s = (next == NULL) ? NULL : next + 1;
  }
  return true;
}

bool
NOAAStore::LoadFromProfile()
{
  const char *stations = Profile::Get(ProfileKeys::WeatherStations);
  if (stations == NULL)
    return false;

  return LoadFromString(stations);
}

void
NOAAStore::SaveToProfile()
{
  char buffer[120], *p = buffer;
  for (auto i = begin(), e = end(); i != e; ++i) {
    const char *code = i->code;
    p = std::copy_n(code, strlen(code), p);
    *p++ = _T(',');
  }

  *p = _T('\0');

  Profile::Set(ProfileKeys::WeatherStations, buffer);
}
