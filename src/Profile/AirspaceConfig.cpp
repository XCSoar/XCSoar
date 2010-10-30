/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Profile/AirspaceConfig.hpp"

#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Sizes.h"

void
Profile::LoadAirspaceConfig()
{
  unsigned Temp = 0;

  SETTINGS_MAP &settings_map = XCSoarInterface::SetSettingsMap();
  SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SetSettingsComputer();

  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; i++) {
#ifdef OLD_TASK // airspace priority
    Get(szProfileAirspacePriority[i], AirspacePriority[i]);
#endif

    if (Get(szProfileAirspaceMode[i], Temp)) {
      settings_computer.DisplayAirspaces[i] = (Temp & 0x1) != 0;
      settings_computer.airspace_warnings.class_warnings[i] = (Temp & 0x2) != 0;
    }

    Get(szProfileBrush[i], settings_map.iAirspaceBrush[i]);
    Get(szProfileColour[i], settings_map.iAirspaceColour[i]);
    if (settings_map.iAirspaceColour[i] >= NUMAIRSPACECOLORS)
      settings_map.iAirspaceColour[i] = 0;
    if (settings_map.iAirspaceBrush[i] >= NUMAIRSPACEBRUSHES)
      settings_map.iAirspaceBrush[i] = 0;
  }
}

void
Profile::SetAirspaceMode(int i)
{
  const SETTINGS_AIRSPACE &settings_airspace =
    XCSoarInterface::SettingsComputer();

  int val = 0;
  if (settings_airspace.DisplayAirspaces[i])
    val |= 0x1;
  if (settings_airspace.airspace_warnings.class_warnings[i])
    val |= 0x2;

  Set(szProfileAirspaceMode[i], val);
}

void
Profile::SetAirspaceColor(int i, int c)
{
  Set(szProfileColour[i], c);
}

void
Profile::SetAirspaceBrush(int i, int c)
{
  Set(szProfileBrush[i], c);
}
