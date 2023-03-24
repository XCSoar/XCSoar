// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Temperature.hpp"

#include <cmath>

[[gnu::const]]
inline Temperature
CalculateDewPoint(Temperature temperature, double humidity_percent)
{
    auto log_ex = 7.5 * temperature.ToCelsius() / (237.3 + temperature.ToCelsius()) +
      (std::log10(humidity_percent) - 2);
    return Temperature::FromCelsius(log_ex * 237.3 / (7.5 - log_ex));
}
