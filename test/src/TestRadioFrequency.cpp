// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
//
// Unit tests for RadioFrequency: 8.33 kHz channel grid (#2225).

#include "RadioFrequency.hpp"

#include "TestUtil.hpp"

static void
TestValid833KhzGrid()
{
  /* One 25 kHz block: only +0, +5, +10 kHz offsets are valid channels. */
  ok1(RadioFrequency::FromKiloHertz(118'000).IsDefined());
  ok1(RadioFrequency::FromKiloHertz(118'005).IsDefined());
  ok1(RadioFrequency::FromKiloHertz(118'010).IsDefined());
  ok1(!RadioFrequency::FromKiloHertz(118'015).IsDefined());
  ok1(!RadioFrequency::FromKiloHertz(118'020).IsDefined());
}

static void
TestOffsetSkipsInvalidChannels()
{
  RadioFrequency f = RadioFrequency::FromKiloHertz(118'010);
  /* +5 kHz would land on 118015 (invalid); expect nudge to 118025. */
  f.OffsetKiloHertz(5);
  ok1(f.IsDefined());
  ok1(equals(static_cast<double>(f.GetKiloHertz()), 118'025.));
}

static void
TestParseRoundTrip()
{
  const RadioFrequency f = RadioFrequency::Parse("118.025");
  ok1(f.IsDefined());
  ok1(equals(static_cast<double>(f.GetKiloHertz()), 118'025.));
}

int
main()
{
  plan_tests(9);

  TestValid833KhzGrid();
  TestOffsetSkipsInvalidChannels();
  TestParseRoundTrip();

  return exit_status();
}
