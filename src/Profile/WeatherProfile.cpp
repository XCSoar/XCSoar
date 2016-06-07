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

#include "WeatherProfile.hpp"
#include "Map.hpp"
#include "ProfileKeys.hpp"
#include "Weather/Settings.hpp"

#ifdef HAVE_PCMET

namespace Profile {
  static void Load(const ProfileMap &map, PCMetSettings &settings) {
    map.Get(ProfileKeys::PCMetUsername, settings.www_credentials.username);
    map.Get(ProfileKeys::PCMetPassword, settings.www_credentials.password);
    map.Get(ProfileKeys::PCMetFtpUsername, settings.ftp_credentials.username);
    map.Get(ProfileKeys::PCMetFtpPassword, settings.ftp_credentials.password);
  }
}

#endif

void
Profile::Load(const ProfileMap &map, WeatherSettings &settings)
{
#ifdef HAVE_PCMET
  Load(map, settings.pcmet);
#endif
}
