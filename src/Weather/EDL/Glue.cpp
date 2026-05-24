// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"

#include "StateController.hpp"
#include "NMEA/MoreData.hpp"

namespace EDL {

void
Glue::OnGPSUpdate(const MoreData &basic)
{
  ProcessGpsUpdate(basic.date_time_utc);
}

} // namespace EDL
