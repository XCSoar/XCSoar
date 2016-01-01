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

#include "TrackingSettings.hpp"

#ifdef HAVE_TRACKING

#ifdef HAVE_LIVETRACK24

#include <tchar.h>

void
LiveTrack24Settings::SetDefaults()
{
  enabled = false;
  server = _T("www.livetrack24.com");
  username.clear();
  password.clear();
}

#endif

void
TrackingSettings::SetDefaults()
{
#ifdef HAVE_LIVETRACK24
  interval = 60;
  vehicleType = VehicleType::GLIDER;
#endif

#ifdef HAVE_SKYLINES_TRACKING
  skylines.SetDefaults();
#endif

#ifdef HAVE_LIVETRACK24
  livetrack24.SetDefaults();
#endif
}

#endif /* HAVE_TRACKING */
