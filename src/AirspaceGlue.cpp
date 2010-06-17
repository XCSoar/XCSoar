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

#include "AirspaceGlue.hpp"
#include "AirspaceParser.hpp"
#include "AirspaceClientUI.hpp"
#include "ProfileKeys.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "LogFile.hpp"
#include "IO/ConfiguredFile.hpp"

void
ReadAirspace(AirspaceClientUI &airspace, 
             RasterTerrain *terrain,
             const AtmosphericPressure &press)
{
  // TODO bug: add exception handler to protect parser code against crashes
  // TODO bug: second file should be opened even if first was not okay

  bool airspace_ok = false;

  // Read the airspace filenames from the registry
  TLineReader *reader =
    OpenConfiguredTextFile(szProfileAirspaceFile, _T("airspace.txt"));
  if (reader != NULL) {
    if (!airspace.read(*reader)) {
      LogStartUp(_T("No airspace file 1"));
    } else {
      airspace_ok =  true;
    }

    delete reader;
  }

  reader = OpenConfiguredTextFile(szProfileAdditionalAirspaceFile);
  if (reader != NULL) {
    if (!airspace.read(*reader)) {
      LogStartUp(_T("No airspace file 2"));
    } else {
      airspace_ok = true;
    }

    delete reader;
  }

  if (airspace_ok) {
    airspace.finalise_after_loading(terrain, press);
  } else {
    airspace.clear(); // there was a problem
  }
}

void 
CloseAirspace(AirspaceClientUI &airspace) 
{
  airspace.clear();
}
