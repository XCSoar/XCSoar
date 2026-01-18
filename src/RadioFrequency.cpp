// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RadioFrequency.hpp"
#include "Math/Util.hpp"
#include "util/CharUtil.hxx"
#include "util/DecimalParser.hxx"
#include "util/NumberParser.hpp"

#include <cstdio>

std::string
RadioFrequency::Format() const noexcept
{
  if (!IsDefined())
    return {};

  unsigned khz = GetKiloHertz();
  unsigned mhz = khz / 1000;
  khz %= 1000;

  char buffer[16];
  std::snprintf(buffer, sizeof(buffer), "%u.%03u", mhz, khz);
  return buffer;
}

RadioFrequency
RadioFrequency::Parse(std::string_view src) noexcept
{
  double mhz;

  if (auto value = ParseDecimal(src))
    mhz = *value;
  else
    return Null();

  if (mhz < MIN_KHZ / 1000. || mhz > MAX_KHZ / 1000.)
    return Null();

  return FromKiloHertz(uround(mhz * 1000));
}
