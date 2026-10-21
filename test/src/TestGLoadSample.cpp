// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Config.hpp"

#include "TestUtil.hpp"

static double
MeanGLoad(DeviceConfig::InstrumentAlignment alignment,
          const double *samples, unsigned count) noexcept
{
  double sum = 0.;
  for (unsigned i = 0; i < count; ++i)
    sum += DeviceConfig::GLoadSample(alignment, samples[i]);
  return sum / count;
}

static void
TestSignKeptWhenAligned() noexcept
{
  using Alignment = DeviceConfig::InstrumentAlignment;
  const double samples[] = {-1.4, 0.6};

  ok1(equals(DeviceConfig::GLoadSample(Alignment::FIXED_AND_ALIGNED, -1.4),
             -1.4));
  ok1(equals(MeanGLoad(Alignment::FIXED_AND_ALIGNED, samples, 2), -0.4));
}

static void
TestSignFoldedWhenNotAligned() noexcept
{
  using Alignment = DeviceConfig::InstrumentAlignment;
  const double samples[] = {-1.4, 0.6};

  ok1(equals(DeviceConfig::GLoadSample(Alignment::NOT_ALIGNED, -1.4), 1.4));
  ok1(equals(MeanGLoad(Alignment::NOT_ALIGNED, samples, 2), 1.));
}

int
main()
{
  plan_tests(4);

  TestSignKeptWhenAligned();
  TestSignFoldedWhenNotAligned();
  return exit_status();
}
