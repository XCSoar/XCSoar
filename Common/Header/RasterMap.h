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

#ifndef RASTERMAP_H
#define RASTERMAP_H

#include "Sizes.h"
#include <zzip/lib.h>
#include "jasper/RasterTile.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct _TERRAIN_INFO
{
  double Left;
  double Right;
  double Top;
  double Bottom;
  double StepSize;
  long Rows;
  long Columns;
} TERRAIN_INFO;


typedef struct _TERRAIN_CACHE
{
  short h;
  long index;
  unsigned int recency;
} TERRAIN_CACHE;


class RasterMap {
 public:
  RasterMap() {
    terrain_valid = false;
    max_field_value = 0;
    DirectFine = false;
    DirectAccess = false;
    Paged = false;
  }
  virtual ~RasterMap() {};

  inline bool isMapLoaded() {
    return terrain_valid;
  }

  short max_field_value;
  TERRAIN_INFO TerrainInfo;

  virtual void SetViewCenter(const double &Latitude,
                             const double &Longitude) {};

  bool GetMapCenter(double *lon, double *lat);

  float GetFieldStepSize();

  // inaccurate method
  int GetEffectivePixelSize(double pixelsize);

  // accurate method
  int GetEffectivePixelSize(double *pixel_D,
                            double latitude, double longitude);

  virtual void SetFieldRounding(double xr, double yr);

  short GetField(const double &Latitude,
                 const double &Longitude);

  virtual bool Open(char* filename) = 0;
  virtual void Close() = 0;
  virtual void Lock() = 0;
  virtual void Unlock() = 0;
  virtual void ServiceCache() {};
  virtual void ServiceFullReload(double lat, double lon) {};
  bool IsDirectAccess(void) { return DirectAccess; };
  bool IsPaged(void) { return Paged; };

 protected:
  int xlleft;
  int xlltop;
  bool terrain_valid;
  bool DirectFine;
  bool Paged;
  bool DirectAccess;
  double fXrounding, fYrounding;
  double fXroundingFine, fYroundingFine;
  int Xrounding, Yrounding;

  virtual short _GetFieldAtXY(unsigned int lx,
                              unsigned int ly) = 0;
};


class RasterMapCache: public RasterMap {
 public:
  RasterMapCache() {
    terraincacheefficiency=0;
    terraincachehits = 1;
    terraincachemisses = 1;
    cachetime = 0;
    DirectAccess = false;
    if (ref_count==0) {
      fpTerrain = NULL;
      InitializeCriticalSection(&CritSec_TerrainFile);
    }
    ref_count++;
  }

  ~RasterMapCache() {
    ref_count--;
    if (ref_count==0) {
      DeleteCriticalSection(&CritSec_TerrainFile);
    }
  }

  // shared!
  static ZZIP_FILE *fpTerrain;
  static int ref_count;

  void ServiceCache();

  void SetCacheTime();
  void ClearTerrainCache();
  short LookupTerrainCache(const long &SeekPos);
  short LookupTerrainCacheFile(const long &SeekPos);
  void OptimizeCash(void);

  virtual bool Open(char* filename);
  virtual void Close();
  void Lock();
  void Unlock();
 protected:
  static CRITICAL_SECTION CritSec_TerrainFile;
  TERRAIN_CACHE TerrainCache[MAXTERRAINCACHE];

  int terraincacheefficiency;
  long terraincachehits;
  long terraincachemisses;
  unsigned int cachetime;
  int SortThresold;

  short _GetFieldAtXY(unsigned int lx,
                      unsigned int ly);
  //
};


class RasterMapRaw: public RasterMap {
 public:
  RasterMapRaw() {
    TerrainMem = NULL;
    DirectAccess = true;
    InitializeCriticalSection(&CritSec_TerrainFile);
  }
  ~RasterMapRaw() {
    DeleteCriticalSection(&CritSec_TerrainFile);
  }
  short *TerrainMem;
  virtual void SetFieldRounding(double xr, double yr);
  virtual bool Open(char* filename);
  virtual void Close();
  void Lock();
  void Unlock();
 protected:
  virtual short _GetFieldAtXY(unsigned int lx,
                              unsigned int ly);
  CRITICAL_SECTION  CritSec_TerrainFile;
};


class RasterMapJPG2000: public RasterMap {
 public:
  RasterMapJPG2000();
  ~RasterMapJPG2000();

  void ReloadJPG2000(void);
  void ReloadJPG2000Full(double latitude, double longitude);

  void SetViewCenter(const double &Latitude,
                     const double &Longitude);
  virtual void SetFieldRounding(double xr, double yr);
  virtual bool Open(char* filename);
  virtual void Close();
  void Lock();
  void Unlock();
  void ServiceFullReload(double lat, double lon);

 protected:
  char jp2_filename[MAX_PATH];
  virtual short _GetFieldAtXY(unsigned int lx,
                              unsigned int ly);
  bool TriggerJPGReload;
  static CRITICAL_SECTION  CritSec_TerrainFile;
  static int ref_count;
  RasterTileCache raster_tile_cache;
};

#endif
