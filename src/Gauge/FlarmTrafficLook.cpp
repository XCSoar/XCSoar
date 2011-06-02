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

#include "FlarmTrafficLook.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"

void
FlarmTrafficLook::Initialise(bool small)
{
  hcPassive = Color(0x99, 0x99, 0x99);
  hcWarning = Graphics::cAlertWarning;
  hcAlarm = Graphics::cAlertAlarm;
  hcStandard = COLOR_BLACK;
  hcSelection = COLOR_BLUE;
  hcBackground = COLOR_WHITE;
  hcRadar = COLOR_LIGHT_GRAY;

  hbWarning.set(hcWarning);
  hbAlarm.set(hcAlarm);
  hbSelection.set(hcSelection);
  hbRadar.set(hcRadar);
  hbTeamGreen.set(COLOR_GREEN);
  hbTeamBlue.set(COLOR_BLUE);
  hbTeamYellow.set(COLOR_YELLOW);
  hbTeamMagenta.set(COLOR_MAGENTA);

  int width = Layout::FastScale(small ? 1 : 2);
  hpWarning.set(width, hcWarning);
  hpAlarm.set(width, hcAlarm);
  hpStandard.set(width, hcStandard);
  hpPassive.set(width, hcPassive);
  hpSelection.set(width, hcSelection);
  hpTeamGreen.set(width, COLOR_GREEN);
  hpTeamBlue.set(width, COLOR_BLUE);
  hpTeamYellow.set(width, COLOR_YELLOW);
  hpTeamMagenta.set(width, COLOR_MAGENTA);

  hpPlane.set(width, hcRadar);
  hpRadar.set(1, hcRadar);

  hfNoTraffic.set(Fonts::GetStandardFontFace(), Layout::FastScale(24));
  hfLabels.set(Fonts::GetStandardFontFace(), Layout::FastScale(14));
  hfSideInfo.set(Fonts::GetStandardFontFace(),
                 Layout::FastScale(small ? 12 : 18), true);

  hfInfoLabels.set(Fonts::GetStandardFontFace(), Layout::FastScale(10), true);
  hfInfoValues.set(Fonts::GetStandardFontFace(), Layout::FastScale(20));
  hfCallSign.set(Fonts::GetStandardFontFace(), Layout::FastScale(28), true);
}

void
FlarmTrafficLook::Deinitialise()
{
  hbWarning.reset();
  hbAlarm.reset();
  hbSelection.reset();
  hbRadar.reset();
  hbTeamGreen.reset();
  hbTeamBlue.reset();
  hbTeamYellow.reset();
  hbTeamMagenta.reset();

  hpWarning.reset();
  hpAlarm.reset();
  hpStandard.reset();
  hpPassive.reset();
  hpSelection.reset();
  hpTeamGreen.reset();
  hpTeamBlue.reset();
  hpTeamYellow.reset();
  hpTeamMagenta.reset();

  hpPlane.reset();
  hpRadar.reset();

  hfNoTraffic.reset();
  hfLabels.reset();
  hfSideInfo.reset();

  hfInfoLabels.reset();
  hfInfoValues.reset();
  hfCallSign.reset();
}
