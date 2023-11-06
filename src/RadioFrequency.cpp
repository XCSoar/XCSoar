// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RadioFrequency.hpp"
#include "Math/Util.hpp"
#include "util/CharUtil.hxx"
#include "util/DecimalParser.hxx"
#include "util/StringFormat.hpp"
#include "util/NumberParser.hpp"

TCHAR *
RadioFrequency::Format(TCHAR *buffer, size_t max_size) const noexcept
{
  if (!IsDefined())
    return nullptr;

  unsigned khz = GetKiloHertz();
  unsigned mhz = khz / 1000;
  khz %= 1000;

  StringFormat(buffer, max_size, _T("%u.%03u"), mhz, khz);
  return buffer;
}

RadioFrequency
RadioFrequency::Parse(const TCHAR *p) noexcept
{
  TCHAR *endptr;
  double mhz = ParseDouble(p, &endptr);

  RadioFrequency frequency;
  if (mhz >= MIN_KHZ / 1000. && mhz <= MAX_KHZ / 1000. &&
      IsWhitespaceOrNull(*endptr))
    frequency.SetKiloHertz(uround(mhz * 1000));
  else
    frequency.Clear();
  return frequency;
}

RadioFrequency
RadioFrequency::Parse(std::string_view src) noexcept
{
  double mhz;

  if (auto value = ParseDecimal(src))
    mhz = *value;
  else
    return Null();

  if (mhz < MIN_KHZ / 1000. && mhz > MAX_KHZ / 1000.)
    return Null();

  return FromKiloHertz(uround(mhz * 1000));
}
