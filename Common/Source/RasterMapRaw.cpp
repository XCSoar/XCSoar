/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "RasterMapRaw.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "UtilsSystem.hpp"


void RasterMapRaw::SetFieldRounding(const double xr,
                                    const double yr,
                                    RasterRounding &rounding)
{
  RasterMap::SetFieldRounding(xr, yr, rounding);
  if (!isMapLoaded()) {
    return;
  }
  if ((rounding.Xrounding==1)&&(rounding.Yrounding==1)) {
    rounding.DirectFine = true;
    rounding.xlleft = (int)(TerrainInfo.Left*rounding.fXroundingFine)+128;
    rounding.xlltop  = (int)(TerrainInfo.Top*rounding.fYroundingFine)-128;
  } else {
    rounding.DirectFine = false;
  }
}


short RasterMapRaw::_GetFieldAtXY(unsigned int lx,
                                  unsigned int ly) {

  unsigned int ix = CombinedDivAndMod(lx);
  unsigned int iy = CombinedDivAndMod(ly);

  if ((ly>=(unsigned int)TerrainInfo.Rows)
      ||(lx>=(unsigned int)TerrainInfo.Columns)) {
    return TERRAIN_INVALID;
  }

  short *tm = TerrainMem+ly*TerrainInfo.Columns+lx;
  // perform piecewise linear interpolation
  int h1 = *tm; // (x,y)

  if (!ix && !iy) {
    return h1;
  }
  if (lx+1 >= (unsigned int)TerrainInfo.Columns) {
    return h1;
  }
  if (ly+1 >= (unsigned int)TerrainInfo.Rows) {
    return h1;
  }
  int h3 = tm[TerrainInfo.Columns+1]; // (x+1, y+1)
  if (ix>iy) {
    // lower triangle
    int h2 = tm[1]; // (x+1,y)
    return (short)(h1+((ix*(h2-h1)-iy*(h2-h3))>>8));
  } else {
    // upper triangle
    int h4 = tm[TerrainInfo.Columns]; // (x,y+1)
    return (short)(h1+((iy*(h4-h1)-ix*(h4-h3))>>8));
  }
}

// Specialised open/close routines

bool RasterMapRaw::Open(char* zfilename) {
  Poco::ScopedRWLock protect(lock, true);

  ZZIP_FILE *fpTerrain;

  max_field_value = 0;
  terrain_valid = false;

  if (strlen(zfilename)<=0)
    return false;

  fpTerrain = zzip_fopen(zfilename, "rb");
  if (fpTerrain == NULL) {
    return false;
  }

  DWORD dwBytesRead;
  dwBytesRead = zzip_fread(&TerrainInfo, 1, sizeof(TERRAIN_INFO),
                           fpTerrain);

  if (dwBytesRead != sizeof(TERRAIN_INFO)) {
    zzip_fclose(fpTerrain);
    return false;
  }

  long nsize = TerrainInfo.Rows*TerrainInfo.Columns;

  if (CheckFreeRam()>(long)(nsize*sizeof(short)+5000000)) {
    // make sure there is 5 meg of ram left after allocating space
    TerrainMem = (short*)malloc(sizeof(short)*nsize);
  } else {
    zzip_fclose(fpTerrain);
    TerrainMem = NULL;
    return false;
  }

  if (!TerrainMem) {
    zzip_fclose(fpTerrain);
    terrain_valid = false;
  } else {
    dwBytesRead = zzip_fread(TerrainMem, 1, nsize*sizeof(short),
                             fpTerrain);

    for (int i=0; i< nsize; i++) {
      max_field_value = max(TerrainMem[i], max_field_value);
    }
    zzip_fclose(fpTerrain);
    terrain_valid = true;
  }

  if (!TerrainInfo.StepSize) {
    terrain_valid = false;
    zzip_fclose(fpTerrain);
    _Close();
  }
  return terrain_valid;
}


void RasterMapRaw::Close(void) {
  Poco::ScopedRWLock protect(lock, true);
  _Close();
}

void RasterMapRaw::_Close(void) {
  terrain_valid = false;
  if (TerrainMem) {
    free(TerrainMem); TerrainMem = NULL;
  }
}

