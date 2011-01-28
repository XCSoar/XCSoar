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

#include "Polar/Loader.hpp"
#include "Polar/WinPilot.hpp"
#include "Polar/BuiltIn.hpp"
#include "LogFile.hpp"
#include "Language.hpp"
#include "Dialogs/Message.hpp"
#include "GlideSolvers/GlidePolar.hpp"

static bool
LoadPolarById2(unsigned id, SimplePolar &polar)
{
  if (id < POLARUSEWINPILOTFILE)
    // polar data from historical table
    return false;
  else if (id == POLARUSEWINPILOTFILE)
    // polar data from winpilot file
    return polar.ReadFileFromProfile();
  else
    // polar data from built-in table
    return ReadWinPilotPolarInternal(id - POLARUSEWINPILOTFILE - 1, polar);
}

bool
LoadPolarById(unsigned id, GlidePolar &polar)
{
  LogStartUp(_T("Load polar"));

  SimplePolar s_polar;
  if (!LoadPolarById2(id, s_polar)) {
    MessageBoxX(_("Error loading Polar file!\nUsing LS8 Polar."),
                _("Warning"), MB_OK | MB_ICONERROR);

    if (!ReadWinPilotPolarInternal(56, s_polar))
      return false;
  }

  s_polar.ConvertToGlidePolar(polar);
  return true;
}
