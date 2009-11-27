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

#include "RasterMap.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "UtilsSystem.hpp"

// export methods to global, take care!
void RasterMap::LockRead() { lock.readLock(); };
void RasterMap::Unlock() { lock.unlock(); };

// Rounding control


bool
RasterMap::GetMapCenter(GEOPOINT *loc) const
{
  if(!isMapLoaded())
    return false;

  loc->Longitude = (TerrainInfo.Left + TerrainInfo.Right)/2;
  loc->Latitude = (TerrainInfo.Top + TerrainInfo.Bottom)/2;
  return true;
}


float RasterMap::GetFieldStepSize() const {
  if (!isMapLoaded()) {
    return 0;
  }
  // this is approximate of course..
  float fstepsize = (float)(250.0/0.0025*TerrainInfo.StepSize);
  return fstepsize;
}


// accurate method
int
RasterMap::GetEffectivePixelSize(double *pixel_D,
                                 const GEOPOINT &location) const
{
  double terrain_step_x, terrain_step_y;
  double step_size = TerrainInfo.StepSize*sqrt(2.0);

  if ((*pixel_D<=0) || (step_size==0)) {
    *pixel_D = 1.0;
    return 1;
  }
  GEOPOINT dloc;

  // how many steps are in the pixel size
  dloc = location; dloc.Latitude += step_size;
  terrain_step_x = Distance(location, dloc);

  dloc = location; dloc.Longitude += step_size;
  terrain_step_y = Distance(location, dloc);

  double rfact = max(terrain_step_x,terrain_step_y)/(*pixel_D);

  int epx = (int)(max(1,ceil(rfact)));
  //  *pixel_D = (*pixel_D)*rfact/epx;

  return epx;
}


int
RasterMap::GetEffectivePixelSize(double dist) const
{
  int grounding;
  grounding = iround(2.0*(GetFieldStepSize()/1000.0)/dist);
  if (grounding<1) {
    grounding = 1;
  }
  return grounding;
}


void
RasterMap::SetFieldRounding(const double xr,
                            const double yr,
                            RasterRounding &rounding) const
{
  if (!isMapLoaded()) {
    return;
  }

  rounding.Xrounding = iround(xr/TerrainInfo.StepSize);
  rounding.Yrounding = iround(yr/TerrainInfo.StepSize);

  if (rounding.Xrounding<1) {
    rounding.Xrounding = 1;
  }
  rounding.fXrounding = 1.0/(rounding.Xrounding*TerrainInfo.StepSize);
  rounding.fXroundingFine = rounding.fXrounding*256.0;
  if (rounding.Yrounding<1) {
    rounding.Yrounding = 1;
  }
  rounding.fYrounding = 1.0/(rounding.Yrounding*TerrainInfo.StepSize);
  rounding.fYroundingFine = rounding.fYrounding*256.0;

  rounding.DirectFine = false;
}

// Map general


// JMW rounding further reduces data as required to speed up terrain
// display on low zoom levels
short RasterMap::GetField(const GEOPOINT &location,
  const RasterRounding &rounding)
{
  if(isMapLoaded()) {
    if (rounding.DirectFine) {
      return _GetFieldAtXY(
          (int)(location.Longitude*rounding.fXroundingFine) - rounding.xlleft,
          rounding.xlltop - (int)(location.Latitude*rounding.fYroundingFine));
    } else {
      unsigned int ix =
        Real2Int((location.Longitude-TerrainInfo.Left)*rounding.fXrounding)*rounding.Xrounding;
      unsigned int iy =
        Real2Int((TerrainInfo.Top-location.Latitude)*rounding.fYrounding)*rounding.Yrounding;

      return _GetFieldAtXY(ix<<8, iy<<8);
    }
  } else {
    return TERRAIN_INVALID;
  }
}
