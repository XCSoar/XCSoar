// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Map.hpp"
#include "util/NumberParser.hpp"

bool
ProfileMap::Get(std::string_view key, int &value) const noexcept
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == nullptr)
    return false;

  // Parse the string for a number
  char *endptr;
  int tmp = ParseInt(str, &endptr, 0);
  if (endptr == str)
    return false;

  // Save parsed value to output parameter value and return success
  value = tmp;
  return true;
}

bool
ProfileMap::Get(std::string_view key, short &value) const noexcept
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == nullptr)
    return false;

  // Parse the string for a number
  char *endptr;
  short tmp = ParseInt(str, &endptr, 0);
  if (endptr == str)
    return false;

  // Save parsed value to output parameter value and return success
  value = tmp;
  return true;
}

bool
ProfileMap::Get(std::string_view key, bool &value) const noexcept
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == nullptr)
    return false;

  // Save value to output parameter value and return success
  value = (str[0] != '0');
  return true;
}

bool
ProfileMap::Get(std::string_view key, unsigned &value) const noexcept
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == nullptr)
    return false;

  // Parse the string for a unsigned number
  char *endptr;
  unsigned tmp = ParseUnsigned(str, &endptr, 0);
  if (endptr == str)
    return false;

  // Save parsed value to output parameter value and return success
  value = tmp;
  return true;
}

bool
ProfileMap::Get(std::string_view key, uint16_t &value) const noexcept
{
  unsigned value32;
  if (!Get(key, value32) || value32 >= 0x10000)
    return false;

  value = (uint16_t)value32;
  return true;
}

bool
ProfileMap::Get(std::string_view key, uint8_t &value) const noexcept
{
  unsigned value32;
  if (!Get(key, value32) || value32 >= 0x100)
    return false;

  value = (uint8_t)value32;
  return true;
}

bool
ProfileMap::Get(std::string_view key, double &value) const noexcept
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == nullptr)
    return false;

  // Parse the string for a floating point number
  char *endptr;
  double tmp = ParseDouble(str, &endptr);
  if (endptr == str)
    return false;

  // Save parsed value to output parameter value and return success
  value = tmp;
  return true;
}

void
ProfileMap::Set(std::string_view key, int value) noexcept
{
  char tmp[50];
  sprintf(tmp, "%d", value);
  Set(key, tmp);
}

void
ProfileMap::Set(std::string_view key, long value) noexcept
{
  char tmp[50];
  sprintf(tmp, "%ld", value);
  Set(key, tmp);
}

void
ProfileMap::Set(std::string_view key, unsigned value) noexcept
{
  char tmp[50];
  sprintf(tmp, "%u", value);
  Set(key, tmp);
}

void
ProfileMap::Set(std::string_view key, double value) noexcept
{
  char tmp[50];
  sprintf(tmp, "%f", value);
  Set(key, tmp);
}
