// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FilteredVarioComputer.hpp"
#include "NMEA/MoreData.hpp"

static double
GetVarioFilterPeriod(const MoreData &data) noexcept
{
  if (!data.settings.vario_filter_period_available)
    return 0;

  return data.settings.vario_filter_period;
}

void
FilteredVarioComputer::Compute(MoreData &data, double sink_rate) noexcept
{
  sample_updated = false;

  if (!data.brutto_vario_available) {
    data.filtered_brutto_vario_available.Clear();
    data.filtered_netto_vario_available.Clear();
    return;
  }

  const double period = GetVarioFilterPeriod(data);
  if (period != last_period) {
    filter.Design(period);
    filter.Reset(data.brutto_vario);
    last_period = period;

    if (period <= 0) {
      data.filtered_brutto_vario_available.Clear();
      data.filtered_netto_vario_available.Clear();
      sample_updated = true;
      return;
    }

    sample_updated = true;
  } else if (period <= 0) {
    sample_updated = true;
    return;
  } else
    sample_updated = filter.Update(data.brutto_vario);

  data.filtered_brutto_vario = filter.GetOutput();
  data.filtered_brutto_vario_available = data.brutto_vario_available;

  data.filtered_netto_vario = data.filtered_brutto_vario - sink_rate;
  data.filtered_netto_vario_available = data.brutto_vario_available;
}
