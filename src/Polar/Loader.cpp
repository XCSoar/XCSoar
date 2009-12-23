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

#include "Polar/Loader.hpp"
#include "Polar/Historical.hpp"
#include "Polar/WinPilot.hpp"
#include "Polar/BuiltIn.hpp"
#include "LogFile.hpp"
#include "Language.hpp"
#include "Dialogs/Message.hpp"

#include "GlideSolvers/GlidePolar.hpp"
#include "Polar/Polar.hpp"

static bool
LoadPolarById2(unsigned id, Polar &a_polar)
{
  if (id < POLARUSEWINPILOTFILE)
    // polar data from historical table
    return LoadHistoricalPolar(id, a_polar);
  else if (id == POLARUSEWINPILOTFILE)
    // polar data from winpilot file
    return ReadWinPilotPolar(a_polar);
  else
    // polar data from built-in table
    return ReadWinPilotPolarInternal(id - POLARUSEWINPILOTFILE - 1, a_polar);
}


extern Polar polar; // for now, this is owned by oldGlidePolar


void
setGlidePolar(GlidePolar& gp)
{
  gp.empty_mass = polar.WEIGHTS[0]+polar.WEIGHTS[1];
  gp.ballast_ratio = polar.WEIGHTS[2]/gp.empty_mass;
  gp.wing_area = polar.WingArea;

  gp.ideal_polar_a = -polar.POLAR[0]/sqrt(gp.empty_mass);
  gp.ideal_polar_b = -polar.POLAR[1];
  gp.ideal_polar_c = -polar.POLAR[2]*sqrt(gp.empty_mass);

  gp.update();
}


static bool
LoadPolarById_internal(const SETTINGS_POLAR &settings)
{
  StartupStore(_T("Load polar\n"));
  if (LoadPolarById2(settings.POLARID, polar))
    return true;

  MessageBoxX(gettext(_T("Error loading Polar file!\r\nUse LS8 Polar.")),
              gettext(_T("Warning")),
              MB_OK|MB_ICONERROR);
  return LoadHistoricalPolar(2, polar);
}


bool
LoadPolarById(const SETTINGS_POLAR &settings, GlidePolar& gp)
{
  if (LoadPolarById_internal(settings)) {
    setGlidePolar(gp);
    return true;
  } else {
    return false;
  }
}
