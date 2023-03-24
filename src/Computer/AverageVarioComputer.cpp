// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AverageVarioComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/VarioInfo.hpp"

void
AverageVarioComputer::Reset()
{
  delta_time.Reset();
  vario_30s_filter.Reset();
  netto_30s_filter.Reset();
}

void
AverageVarioComputer::Compute(const MoreData &basic,
                              bool circling, bool last_circling,
                              VarioInfo &vario_info)
{
  const auto dt = delta_time.Update(basic.time, std::chrono::seconds{1}, {});
  if (dt.count() < 0 || circling != last_circling) {
    Reset();
    vario_info.average = basic.brutto_vario;
    vario_info.netto_average = basic.netto_vario;
    return;
  }

  if (dt.count() <= 0)
    return;

  const unsigned Elapsed = std::chrono::round<std::chrono::seconds>(dt).count();
  if (Elapsed == 0)
    return;

  for (unsigned i = 0; i < Elapsed; ++i) {
    vario_30s_filter.Update(basic.brutto_vario);
    netto_30s_filter.Update(basic.netto_vario);
  }

  vario_info.average = vario_30s_filter.Average();
  vario_info.netto_average = netto_30s_filter.Average();
}
