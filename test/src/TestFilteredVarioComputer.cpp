// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/FilteredVarioComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "TestUtil.hpp"

#include <thread>

static void
SetupVario(MoreData &data, double brutto, double netto) noexcept
{
  data.Reset();
  data.clock = TimeStamp{FloatDuration{1}};
  data.brutto_vario = brutto;
  data.brutto_vario_available.Update(data.clock);
  data.netto_vario = netto;
  data.netto_vario_available.Update(data.clock);
}

static void
TestPassthroughWithoutVariofil() noexcept
{
  MoreData data;
  SetupVario(data, 2.5, 1.5);

  FilteredVarioComputer computer;
  computer.Compute(data, 1.0);

  ok1(!data.VarioOutputFilterActive());
  ok1(!computer.FilterActive());
  ok1(!data.filtered_brutto_vario_available);
  ok1(!data.filtered_netto_vario_available);
  ok1(equals(data.FilteredBruttoVario(), 2.5));
  ok1(equals(data.FilteredNettoVario(), 1.5));
}

static void
TestPassthroughWithZeroVariofil() noexcept
{
  MoreData data;
  SetupVario(data, 1.2, 0.4);
  ok1(data.settings.ProvideVarioFilterPeriod(0, data.clock));

  FilteredVarioComputer computer;
  computer.Compute(data, 0.8);

  ok1(!data.VarioOutputFilterActive());
  ok1(!computer.FilterActive());
  ok1(!data.filtered_brutto_vario_available);
  ok1(!data.filtered_netto_vario_available);
  ok1(equals(data.FilteredBruttoVario(), 1.2));
  ok1(equals(data.FilteredNettoVario(), 0.4));
}

static void
TestActiveFilter() noexcept
{
  MoreData data;
  SetupVario(data, 1.0, 0.5);
  ok1(data.settings.ProvideVarioFilterPeriod(2.0, data.clock));

  FilteredVarioComputer computer;
  constexpr double sink_rate = 0.3;

  computer.Compute(data, sink_rate);
  ok1(data.VarioOutputFilterActive());
  ok1(computer.FilterActive());
  ok1(data.filtered_brutto_vario_available);
  ok1(data.filtered_netto_vario_available);
  ok1(equals(data.filtered_brutto_vario, 1.0));
  ok1(equals(data.filtered_netto_vario, 1.0 - sink_rate));
  ok1(equals(data.FilteredBruttoVario(), 1.0));
  ok1(equals(data.FilteredNettoVario(), 1.0 - sink_rate));

  /* Establish wall-clock timing before testing smoothing. */
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  computer.Compute(data, sink_rate);

  data.brutto_vario = 3.0;
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  computer.Compute(data, sink_rate);

  ok1(data.filtered_brutto_vario_available);
  ok1(data.filtered_brutto_vario > 1.0);
  ok1(data.filtered_brutto_vario < 3.0);
  ok1(equals(data.filtered_netto_vario,
             data.filtered_brutto_vario - sink_rate));
}

int
main()
{
  plan_tests(26);

  TestPassthroughWithoutVariofil();
  TestPassthroughWithZeroVariofil();
  TestActiveFilter();

  return exit_status();
}
