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

#include "Profile/AirspaceConfig.hpp"

#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Sizes.h"

void
Profile::LoadAirspaceConfig()
{
  unsigned Temp = 0;

  AirspaceComputerSettings &computer =
    CommonInterface::SetSettingsComputer().airspace;
  AirspaceRendererSettings &renderer =
    CommonInterface::SetSettingsMap().airspace;

  Get(szProfileAirspaceWarning, computer.enable_warnings);
  Get(szProfileAltMargin, computer.warnings.AltWarningMargin);
  Get(szProfileWarningTime, computer.warnings.WarningTime);
  Get(szProfileAcknowledgementTime, computer.warnings.AcknowledgementTime);

  Get(szProfileAirspaceBlackOutline, renderer.black_outline);
  GetEnum(szProfileAltMode, renderer.altitude_mode);
  Get(szProfileClipAlt, renderer.clip_altitude);

#ifndef ENABLE_OPENGL
  Get(szProfileAirspaceTransparency, renderer.transparency);
  GetEnum(szProfileAirspaceFillMode, renderer.fill_mode);
#endif

  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; i++) {
    if (Get(szProfileAirspaceMode[i], Temp)) {
      renderer.display[i] = (Temp & 0x1) != 0;
      computer.warnings.class_warnings[i] = (Temp & 0x2) != 0;
    }

    Get(szProfileBrush[i], renderer.brushes[i]);
    Get(szProfileColour[i], renderer.colours[i]);
    if (renderer.colours[i] >= NUMAIRSPACECOLORS)
      renderer.colours[i] = 0;
    if (renderer.brushes[i] >= NUMAIRSPACEBRUSHES)
      renderer.brushes[i] = 0;
  }
}

void
Profile::SetAirspaceMode(int i)
{
  const AirspaceComputerSettings &computer =
    CommonInterface::SettingsComputer().airspace;
  const AirspaceRendererSettings &renderer =
    CommonInterface::SettingsMap().airspace;

  int val = 0;
  if (renderer.display[i])
    val |= 0x1;
  if (computer.warnings.class_warnings[i])
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
