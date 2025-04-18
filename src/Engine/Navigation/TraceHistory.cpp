// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#include "TraceHistory.hpp"
#include "NMEA/MoreData.hpp"

void
TraceHistory::append(const MoreData &basic) noexcept
{
  BruttoVario.push(basic.brutto_vario);
  NettoVario.push(basic.netto_vario);
  vario_available.Update(basic.clock);
}

void
TraceHistory::clear() noexcept
{
  BruttoVario.clear();
  NettoVario.clear();
  CirclingAverage.clear();

  vario_available.Clear();
  circling_available.Clear();
}
