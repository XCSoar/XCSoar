// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
//
// Regression tests for NMEA merge and MWV parsing (#2210, #2209).

#include "Device/Parser.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"

#include "TestUtil.hpp"

#include "time/Stamp.hpp"

#include <cstdio>
#include <string_view>

static bool
ParseNmeaLine(const char *payload_after_dollar, NMEAInfo &info) noexcept
{
  char buf[120];
  const uint8_t cs =
      NMEAChecksum(std::string_view(payload_after_dollar));
  std::snprintf(buf, sizeof buf, "$%s*%02X", payload_after_dollar,
                (unsigned)cs);
  NMEAParser parser;
  return parser.ParseLine(buf, info);
}

static void
TestComplementCopiesStallRatio()
{
  NMEAInfo dst, src;
  dst.Reset();
  src.Reset();

  const TimeStamp ts{FloatDuration{50}};
  dst.clock = src.clock = ts;
  dst.alive.Update(ts);
  src.alive.Update(ts);

  src.stall_ratio = 0.35;
  src.stall_ratio_available.Update(ts);

  ok1(!dst.stall_ratio_available);
  dst.Complement(src);
  ok1(dst.stall_ratio_available);
  ok1(equals(dst.stall_ratio, 0.35));
}

static void
TestMvwTrueWindAccepted()
{
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};
  ok1(ParseNmeaLine("IIMWV,270,T,12,N,A", info));
  ok1(info.external_wind_available);
}

static void
TestMvwStatusInvalidIgnored()
{
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};
  ok1(ParseNmeaLine("IIMWV,270,T,12,N,V", info));
  ok1(!info.external_wind_available);
}

static void
TestMvwRelativeReferenceIgnored()
{
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};
  ok1(ParseNmeaLine("IIMWV,270,R,12,N,A", info));
  ok1(!info.external_wind_available);
}

int
main()
{
  plan_tests(9);

  TestComplementCopiesStallRatio();
  TestMvwTrueWindAccepted();
  TestMvwStatusInvalidIgnored();
  TestMvwRelativeReferenceIgnored();

  return exit_status();
}
