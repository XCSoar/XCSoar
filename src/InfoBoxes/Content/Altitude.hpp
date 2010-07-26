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

#ifndef XCSOAR_INFOBOX_CONTENT_ALTITUDE_HPP
#define XCSOAR_INFOBOX_CONTENT_ALTITUDE_HPP

#include "InfoBoxes/Content/Base.hpp"

class InfoBoxContentAltitudeGPS : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
  virtual bool HandleKey(const InfoBoxKeyCodes keycode);
  virtual const TCHAR* GetHelpText() {
    return _T("[Height GPS]\r\nThis is the height above mean sea level reported by the GPS.\r\n Touchscreen/PC only: in simulation mode, this value is adjustable with the up/down arrow keys and the right/left arrow keys also cause the glider to turn.");
  }
};

class InfoBoxContentAltitudeAGL : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
  virtual const TCHAR* GetHelpText() {
    return _T("[Height AGL]\r\nThis is the navigation altitude minus the terrain height obtained from the terrain file.  The value is coloured red when the glider is below the terrain safety clearance height.");
  }
};

class InfoBoxContentAltitudeBaro : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
  virtual const TCHAR* GetHelpText() {
    return _T("[Pressure Altitude]\r\nThis is the barometric altitude obtained from a GPS equipped with pressure sensor, or a supported external intelligent vario.");
  }
};

class InfoBoxContentAltitudeQFE : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
  virtual const TCHAR* GetHelpText() {
    return _T("[QFE]\r\nAutomatic QFE. This altitude value is constantly reset to 0 on ground BEFORE taking off. After takeoff, it is no more reset automatically even if on ground. During flight you can change QFE with up and down keys. Bottom line shows QNH altitude. \r\nChanging QFE does not affect QNH altitude.");
  }
};

class InfoBoxContentTerrainHeight : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
  virtual const TCHAR* GetHelpText() {
    return _T("[Terrain Elevation]\r\nThis is the elevation of the terrain above mean sea level, obtained from the terrain file at the current GPS location.");
  }
};

#endif
