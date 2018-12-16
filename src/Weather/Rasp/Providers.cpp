/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Providers.hpp"

const RaspProvider rasp_providers[] = {
  { _T("Germany"),
    "http://rasp.linta.de/GERMANY/xcsoar-rasp.dat" },
  { _T("Germany Schwarzwald"),
    "http://rasp.linta.de/BLACKFOREST_WAVE/xcsoar-rasp.dat" },
  { _T("Germany Nordrhein-Westfalen"),
    "http://rasp.segelflugschule-oerlinghausen.de/NRW/FCST/xcsoar-rasp.dat" },
  { _T("Germany Niedersachsen"),
    "http://rasp.linta.de/NIEDERSACHSEN_WAVE/xcsoar-rasp.dat" },
  { _T("Scandinavia"),
    "http://rasp.linta.de/SCANDINAVIA/xcsoar-rasp.dat" },
  { _T("United Kingdom"),
    "http://rasp-uk.uk/XCSoar/xcsoar-rasp.dat" },

  { nullptr, nullptr }
};
