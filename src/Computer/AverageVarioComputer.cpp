/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "AverageVarioComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

void
AverageVarioComputer::Reset()
{
  vario_30s_filter.Reset();
  netto_30s_filter.Reset();
}

void
AverageVarioComputer::Compute(const MoreData &basic,
                              const NMEAInfo &last_basic,
                              bool circling, bool last_circling,
                              VarioInfo &vario_info)
{
  const bool time_advanced = basic.HasTimeAdvancedSince(last_basic);
  if (!time_advanced || circling != last_circling) {
    Reset();
    vario_info.average = basic.brutto_vario;
    vario_info.netto_average = basic.netto_vario;
  }

  if (!time_advanced)
    return;

  const unsigned Elapsed(basic.time - last_basic.time);
  if (Elapsed == 0)
    return;

  for (unsigned i = 0; i < Elapsed; ++i) {
    vario_30s_filter.Update(basic.brutto_vario);
    netto_30s_filter.Update(basic.netto_vario);
  }

  vario_info.average = vario_30s_filter.Average();
  vario_info.netto_average = netto_30s_filter.Average();
}
