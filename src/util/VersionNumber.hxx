// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StaticString.hxx"
#include <stdlib.h>

/**
 * @brief Convert a string to a version number and compare it to other version number
 */
struct VersionNumber
{
  unsigned int major;
  unsigned int minor;
  unsigned int patch;

  VersionNumber(const std::string_view str)
  {
    major = 0;
    minor = 0;
    patch = 0;

    const char *p = str.data();
    major = std::strtoul(p, const_cast<char **>(&p), 10);
    if (*p == '.')
    {
      ++p;
      minor = std::strtoul(p, const_cast<char **>(&p), 10);
      if (*p == '.')
      {
        ++p;
        patch = std::strtoul(p, const_cast<char **>(&p), 10);
      }
    }
  }

  constexpr VersionNumber(unsigned int major, unsigned int minor,
                          unsigned int patch = 0)
      : major(major), minor(minor), patch(patch)
  {
  }

  /**
   * @brief Convert the version number to a string.
   * @param include_patch If true, include the patch version ("1.2.3", otherwise "1.2").
   */
  StaticString<16>
  toString(bool include_patch = true) const
  {
    StaticString<16> result;
    if (include_patch)
      result.Format("%u.%u.%u", major, minor, patch);
    else
      result.Format("%u.%u", major, minor);
    return result;
  }

  constexpr bool
  operator==(const VersionNumber &other) const
  {
    return major == other.major &&
           minor == other.minor &&
           patch == other.patch;
  }

  constexpr bool
  operator<(const VersionNumber &other) const
  {
    if (major < other.major)
      return true;
    if (major > other.major)
      return false;
    if (minor < other.minor)
      return true;
    if (minor > other.minor)
      return false;
    if (patch < other.patch)
      return true;
    if (patch > other.patch)
      return false;
    return false;
  }

  constexpr bool
  operator!=(const VersionNumber &other) const
  {
    return !(*this == other);
  }

  constexpr bool
  operator>(const VersionNumber &other) const
  {
    return other < *this;
  }

  constexpr bool
  operator<=(const VersionNumber &other) const
  {
    return !(other < *this);
  }

  constexpr bool
  operator>=(const VersionNumber &other) const
  {
    return !(*this < other);
  }

  constexpr unsigned int
  getMajor() const
  {
    return major;
  }

  constexpr unsigned int
  getMinor() const
  {
    return minor;
  }

  constexpr unsigned int
  getPatch() const
  {
    return patch;
  }
};
