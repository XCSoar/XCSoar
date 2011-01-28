/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Polar/Historical.hpp"
#include "Polar/Polar.hpp"

#include <assert.h>

const TCHAR *PolarLabels[] = {
  _T("Vintage - Ka6"),
  _T("Club - ASW19"),
  _T("Standard - LS8"),
  _T("15M - ASW27"),
  _T("18M - LS6C"),
  _T("Open - ASW22"),
  _T("External Polar File"),
  NULL
};

static const Polar historical_polars[] = {
  { -0.0033413128, 0.1323114348, -2.0532378149,
    70, 190, 1,
    12.4,
  },
  { -0.002976521, 0.1509454717, -2.63731637789569,
    70, 250, 100,
    11.0,
  },
  { -0.0033981549, 0.1896480967, -3.3159695477,
    70, 240, 285,
    10.5,
  },
  { -0.0016042718, 0.0771466019, -1.510553398,
    70, 287, 165,
    9.0,
  },
  { -0.001028299, 0.0318771616, -0.66756743656287,
    70, 400, 120,
    11.4,
  },
  { -0.0017632653, 0.0746938776, -1.1906122449,
    70, 527, 303,
    16.31,
  },
};

bool
LoadHistoricalPolar(unsigned id, Polar &polar)
{
  assert(id < sizeof(historical_polars) / sizeof(historical_polars[0]));

  polar = historical_polars[id];

  return true;
}
