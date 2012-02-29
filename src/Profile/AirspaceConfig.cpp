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

#include "Profile/AirspaceConfig.hpp"
#include "Profile/Profile.hpp"
#include "Screen/Features.hpp"
#include "Formatter/HexColor.hpp"
#include "Look/AirspaceLook.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Util/Macros.hpp"
#include "Sizes.h"

void
Profile::Load(AirspaceRendererSettings &renderer)
{
  Get(szProfileAirspaceBlackOutline, renderer.black_outline);
  GetEnum(szProfileAltMode, renderer.altitude_mode);
  Get(szProfileClipAlt, renderer.clip_altitude);

#ifndef ENABLE_OPENGL
  Get(szProfileAirspaceTransparency, renderer.transparency);
#endif

  GetEnum(szProfileAirspaceFillMode, renderer.fill_mode);

  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; i++)
    Load(i, renderer.classes[i]);
}

void
Profile::Load(unsigned i, AirspaceClassRendererSettings &settings)
{
  unsigned value;
  if (Get(szProfileAirspaceMode[i], value))
    settings.display = (value & 0x1) != 0;

#ifdef HAVE_HATCHED_BRUSH
  Get(szProfileBrush[i], settings.brush);
  if (settings.brush >= NUMAIRSPACEBRUSHES)
    settings.brush = 0;
#endif

  GetAirspaceColor(i, settings.color);
}

void
Profile::Load(AirspaceComputerSettings &computer)
{
  Get(szProfileAirspaceWarning, computer.enable_warnings);
  Get(szProfileAltMargin, computer.warnings.altitude_warning_margin);
  Get(szProfileWarningTime, computer.warnings.warning_time);
  Get(szProfileAcknowledgementTime, computer.warnings.acknowledgement_time);

  unsigned Temp;
  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; i++)
    if (Get(szProfileAirspaceMode[i], Temp))
      computer.warnings.class_warnings[i] = (Temp & 0x2) != 0;
}

void
Profile::SetAirspaceMode(int i, bool display, bool warning)
{
  int val = 0;
  if (display)
    val |= 0x1;
  if (warning)
    val |= 0x2;

  Set(szProfileAirspaceMode[i], val);
}

void
Profile::SetAirspaceColor(int i, const Color &color)
{
  TCHAR buffer[16];
  FormatHexColor(buffer, ARRAY_SIZE(buffer), color);
  Set(szProfileColour[i], buffer);
}

bool
Profile::GetAirspaceColor(int i, Color &color)
{
  const TCHAR *color_string = Get(szProfileColour[i]);
  if (!color_string)
    return false;

  if (ParseHexColor(color_string, color))
    return true;

  unsigned index;
  if (!Get(szProfileColour[i], index))
    return false;

  if (index >= NUMAIRSPACECOLORS)
    index = 0;

  color = AirspaceLook::preset_colors[index];
  return true;
}

void
Profile::SetAirspaceBrush(int i, int c)
{
  Set(szProfileBrush[i], c);
}
