// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CuComputer.hpp"
#include "Settings.hpp"
#include "Atmosphere/Temperature.hpp"

struct NMEAInfo;
struct DerivedInfo;

void
CuComputer::Reset()
{
  cu_sonde.Reset();
}

void
CuComputer::Compute(const NMEAInfo &basic, const DerivedInfo &calculated,
                    const ComputerSettings &settings)
{
  cu_sonde.SetForecastTemperature(settings.forecast_temperature);

  cu_sonde.UpdateMeasurements(basic, calculated);
}
