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

#ifndef FLARM_TRAFFIC_WINDOW_LOOK_HPP
#define FLARM_TRAFFIC_WINDOW_LOOK_HPP

#include "Screen/Color.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Font.hpp"

struct FlarmTrafficLook {
  Color hcWarning;
  Color hcAlarm;
  Color hcStandard;
  Color hcPassive;
  Color hcSelection;
  Color hcBackground;
  Color hcRadar;

  Brush hbWarning;
  Brush hbAlarm;
  Brush hbSelection;
  Brush hbRadar;
  Brush hbTeamGreen;
  Brush hbTeamBlue;
  Brush hbTeamYellow;
  Brush hbTeamMagenta;

  Pen hpWarning;
  Pen hpAlarm;
  Pen hpStandard;
  Pen hpPassive;
  Pen hpSelection;
  Pen hpTeamGreen;
  Pen hpTeamBlue;
  Pen hpTeamYellow;
  Pen hpTeamMagenta;

  Pen hpPlane, hpRadar;

  Font hfLabels, hfSideInfo, hfNoTraffic;
  Font hfInfoValues, hfInfoLabels, hfCallSign;

  void Initialise(bool small);
  void Deinitialise();
};

#endif
