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

#ifndef RASTERMAPJPG2000_H
#define RASTERMAPJPG2000_H

#include "RasterMap.h"
#include <zzip/lib.h>
#include "jasper/RasterTile.h"

class RasterMapJPG2000: public RasterMap {
 public:
  RasterMapJPG2000();
  ~RasterMapJPG2000();

  void ReloadJPG2000(void);
  void ReloadJPG2000Full(const GEOPOINT &location);

  void SetViewCenter(const GEOPOINT &location);
  virtual void SetFieldRounding(const double xr, const double yr,
    RasterRounding &rounding);
  virtual bool Open(const char *path);
  void ServiceFullReload(const GEOPOINT &location);

  static RasterMapJPG2000 *LoadFile(const char *path);

 protected:
  char jp2_filename[MAX_PATH];
  virtual short _GetFieldAtXY(unsigned int lx,
                              unsigned int ly);
  bool TriggerJPGReload;
  static int ref_count;
  RasterTileCache raster_tile_cache;
  virtual void _ReloadJPG2000(void);
};


#endif
