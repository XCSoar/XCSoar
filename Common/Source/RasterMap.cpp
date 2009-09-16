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

#include "RasterMap.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "UtilsSystem.hpp"

#ifdef __MINGW32__
#define int_fast8_t jas_int_fast8_t
#endif

// JMW experimental
#include "jasper/jasper.h"
#include "jasper/jpc_rtc.h"
#include "wcecompat/ts_string.h"


// static variables shared between rasterterrains because can only
// have file opened by one reader

// export methods to global, take care!
void RasterMap::LockRead() { lock.readLock(); };
void RasterMap::Unlock() { lock.unlock(); };

////// Rounding control ////////////////////////////////////////////////////



bool RasterMap::GetMapCenter(GEOPOINT *loc) {
  if(!isMapLoaded())
    return false;

  loc->Longitude = (TerrainInfo.Left + TerrainInfo.Right)/2;
  loc->Latitude = (TerrainInfo.Top + TerrainInfo.Bottom)/2;
  return true;
}


float RasterMap::GetFieldStepSize() {
  if (!isMapLoaded()) {
    return 0;
  }
  // this is approximate of course..
  float fstepsize = (float)(250.0/0.0025*TerrainInfo.StepSize);
  return fstepsize;
}


// accurate method
int RasterMap::GetEffectivePixelSize(double *pixel_D,
                                     const GEOPOINT &location)
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


int RasterMap::GetEffectivePixelSize(double dist) {
  int grounding;
  grounding = iround(2.0*(GetFieldStepSize()/1000.0)/dist);
  if (grounding<1) {
    grounding = 1;
  }
  return grounding;
}


void RasterMap::SetFieldRounding(const double xr, 
                                 const double yr,
                                 RasterRounding &rounding) 
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


void RasterMapJPG2000::SetFieldRounding(const double xr, 
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


////// Field access ////////////////////////////////////////////////////


short RasterMapJPG2000::_GetFieldAtXY(unsigned int lx,
                                      unsigned int ly) {

  return raster_tile_cache.GetField(lx,ly);
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


short RasterMapCache::_GetFieldAtXY(unsigned int lx,
                                    unsigned int ly) {

  //  unsigned int ix = CombinedDivAndMod(lx);
  //  unsigned int iy = CombinedDivAndMod(ly);

  if ((ly>=(unsigned int)TerrainInfo.Rows)
      ||(lx>=(unsigned int)TerrainInfo.Columns)) {
    return TERRAIN_INVALID;
  }
  return LookupTerrainCache((ly*TerrainInfo.Columns+lx)*2
                            +sizeof(TERRAIN_INFO));
}

void RasterMapCache::LockRead() 
{ 
  // all lookups for this class potentially change the
  // object so must have write (exclusive) lock even 
  // when doing terrain queries.
  lock.writeLock(); 
};

////////// Map general /////////////////////////////////////////////


// JMW rounding further reduces data as required to speed up terrain
// display on low zoom levels
short RasterMap::GetField(const GEOPOINT &location,
  const RasterRounding &rounding)
{
  if(isMapLoaded()) {
    if (rounding.DirectFine) {
      return _GetFieldAtXY((int)(location.Longitude*rounding.fXroundingFine)
                           -rounding.xlleft,
                           rounding.xlltop-
                           (int)(location.Latitude*rounding.fYroundingFine));
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

//////////// Cached load on demand ////////////////////////////////

//////////// Cache map ////////////////////////////////////////////////
int RasterMapCache::ref_count = 0;

ZZIP_FILE *RasterMapCache::fpTerrain;

void RasterMapCache::ServiceCache(void) {
  Poco::ScopedRWLock protect(lock, true);

  if (terraincachemisses > 0){
    OptimiseCache();
  }
  SetCacheTime();
}


void RasterMapCache::SetCacheTime() {
  terraincachehits = 1;
  terraincachemisses = 0;
  cachetime++;
}


void RasterMapCache::ClearTerrainCache() {
  int i;
  for (i=0; i<MAXTERRAINCACHE; i++) {
    TerrainCache[i].index= -1;
    TerrainCache[i].recency= 0;
    TerrainCache[i].h= 0;
  }
  SortThresold = MAXTERRAINCACHE-1;
}

static int _cdecl TerrainCacheCompare(const void *elem1, const void *elem2 ){
#ifdef PARANOID
  if (!elem1 && !elem2) {
    return(0);
  }
  if (elem1 && !elem2) {
    return(-1);
  }
  if (!elem1 && elem2) {
    return(1);
  }
#endif
  if (((TERRAIN_CACHE *)elem1)->recency > ((TERRAIN_CACHE *)elem2)->recency)
    return (-1);
  if (((TERRAIN_CACHE *)elem1)->recency < ((TERRAIN_CACHE *)elem2)->recency)
    return (+1);
  if (((TERRAIN_CACHE *)elem1)->index > ((TERRAIN_CACHE *)elem2)->index)
    return (-1);
  if (((TERRAIN_CACHE *)elem1)->index < ((TERRAIN_CACHE *)elem2)->index)
    return (+1);
  return (0);
}

void RasterMapCache::OptimiseCache(void){
  qsort(&TerrainCache, MAXTERRAINCACHE,
        sizeof(_TERRAIN_CACHE), TerrainCacheCompare);
  SortThresold = MAXTERRAINCACHE-1;
}


short RasterMapCache::LookupTerrainCacheFile(const long &SeekPos) {
  // put new value in slot tcpmin

  __int16 NewAlt = 0;
  long SeekRes;
  short Alt;

  if(!isMapLoaded())
    return TERRAIN_INVALID;

  SeekRes = zzip_seek(fpTerrain, SeekPos, SEEK_SET);
  if(SeekRes != SeekPos) {
    // error, not found!
    Alt = TERRAIN_INVALID;
  } else {
    if (zzip_fread(&NewAlt, 1, sizeof(__int16), fpTerrain) != sizeof(__int16))
      Alt = TERRAIN_INVALID;
    else {
      Alt = max(0,NewAlt);
    }
  }

  return Alt;
}


int TerrainCacheSearch(const void *key, const void *elem2 ){
#ifdef PARANOID
  if (!elem2) return (0);
#endif
  if ((long)key > ((TERRAIN_CACHE *)elem2)->index)
    return (-1);
  if ((long)key < ((TERRAIN_CACHE *)elem2)->index)
    return (+1);
  return (0);
}

short RasterMapCache::LookupTerrainCache(const long &SeekPos) {
  _TERRAIN_CACHE* tcp, *tcpmin, *tcplim;

  if(fpTerrain == NULL || TerrainInfo.StepSize == 0)
    return TERRAIN_INVALID;

  // search to see if it is found in the cache
  tcp = (_TERRAIN_CACHE *)bsearch((void *)SeekPos, &TerrainCache,
                                  SortThresold, sizeof(_TERRAIN_CACHE),
                                  TerrainCacheSearch);

  if (tcp != NULL){
    tcp->recency = cachetime;
    terraincachehits++;
    return(tcp->h);
  }

  // bsearch failed, so try exhaustive search

  tcp = &TerrainCache[SortThresold];
  tcplim = tcp+MAXTERRAINCACHE-SortThresold;
  while (tcp< tcplim) {
    if (tcp->index == SeekPos) {
      tcp->recency = cachetime;
      terraincachehits++;
      return (tcp->h);
    }
    tcp++;
  }

  // if not found..
  terraincachemisses++;

  if (SortThresold>= MAXTERRAINCACHE) {
    SortThresold= MAXTERRAINCACHE-1;
  } else if (SortThresold<0) {
    SortThresold = 0;
  }

  tcpmin = &TerrainCache[SortThresold];

  short Alt = LookupTerrainCacheFile(SeekPos);

  tcpmin->recency = cachetime;
  tcpmin->h = Alt;
  tcpmin->index = SeekPos;

  SortThresold--;
  if (SortThresold<0) {
    SortThresold = 0;
  }

  return (Alt);
}


//////////// JPG2000 //////////////////////////////////////////////////

void RasterMapJPG2000::ServiceFullReload(const GEOPOINT &location) {
  ReloadJPG2000Full(location);
}


RasterMapJPG2000::RasterMapJPG2000() {
  TriggerJPGReload = false;
  jp2_filename[0] = '\0';
  DirectAccess = true;
  Paged = true;
  if (ref_count==0) {
    jas_init();
  }
  ref_count++;
}

RasterMapJPG2000::~RasterMapJPG2000() {
  ref_count--;
  if (ref_count==0) {
    jas_cleanup();
  }
}


int RasterMapJPG2000::ref_count = 0;


void RasterMapJPG2000::ReloadJPG2000Full(const GEOPOINT &location) {
  // load all 16 tiles...
  for (int i=0; i<MAX_ACTIVE_TILES; i++) {
    TriggerJPGReload = true;
    SetViewCenter(location);
  }
}


void RasterMapJPG2000::ReloadJPG2000(void) {
  if (TriggerJPGReload) {

    Poco::ScopedRWLock protect(lock, true);
    TriggerJPGReload = false;

    raster_tile_cache.LoadJPG2000(jp2_filename);
    if (raster_tile_cache.GetInitialised()) {
      TerrainInfo.Left = raster_tile_cache.lon_min;
      TerrainInfo.Right = raster_tile_cache.lon_max;
      TerrainInfo.Top = raster_tile_cache.lat_max;
      TerrainInfo.Bottom = raster_tile_cache.lat_min;
      TerrainInfo.Columns = raster_tile_cache.GetWidth();
      TerrainInfo.Rows = raster_tile_cache.GetHeight();
      TerrainInfo.StepSize = (raster_tile_cache.lon_max -
                              raster_tile_cache.lon_min)
        /raster_tile_cache.GetWidth();
    }
  }
}


void RasterMapJPG2000::SetViewCenter(const GEOPOINT &location)
{
  {
    Poco::ScopedRWLock protect(lock, true);
    if (raster_tile_cache.GetInitialised()) {
      int x = lround((location.Longitude-TerrainInfo.Left)*TerrainInfo.Columns
                     /(TerrainInfo.Right-TerrainInfo.Left));
      int y = lround((TerrainInfo.Top-location.Latitude)*TerrainInfo.Rows
                     /(TerrainInfo.Top-TerrainInfo.Bottom));
      TriggerJPGReload |= raster_tile_cache.PollTiles(x, y);
    }
  }
  if (TriggerJPGReload) {
    ReloadJPG2000();
  }
}


///////// Specialised open/close routines ///////////////////

bool RasterMapCache::Open(char* zfilename) {

  terrain_valid = false;
  if (strlen(zfilename)<=0) {
    return false;
  }
  if (!fpTerrain) {
    fpTerrain = zzip_fopen(zfilename, "rb");
    if (!fpTerrain) {
      return false;
    }
    if (!zzip_file_real(fpTerrain)) {
      // don't allow cache mode on files in zip, because way too slow
      zzip_fclose(fpTerrain);
      fpTerrain = NULL;   // was false
      return false;
    };
  }

  DWORD dwBytesRead;
  dwBytesRead = zzip_fread(&TerrainInfo, 1, sizeof(TERRAIN_INFO),
                           fpTerrain);

  if (dwBytesRead != sizeof(TERRAIN_INFO)) {
    Close();
    return false;
  }

  if (!TerrainInfo.StepSize) {
    Close();
    return false;
  }
  terrain_valid = true;
  ClearTerrainCache();
  return terrain_valid;
}


bool RasterMapRaw::Open(char* zfilename) {
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
    Close();
  }
  return terrain_valid;
}


bool RasterMapJPG2000::Open(char* zfilename) {
  strcpy(jp2_filename,zfilename);

  // force first-time load
  TriggerJPGReload = true;
  ReloadJPG2000();

  terrain_valid = raster_tile_cache.GetInitialised();
  if (!terrain_valid) {
    raster_tile_cache.Reset();
    max_field_value = 0;
  } else {
    max_field_value = raster_tile_cache.GetMaxElevation();
  }
  return terrain_valid;
}


///////////////// Close routines /////////////////////////////////////

void RasterMapJPG2000::Close(void) {
  Poco::ScopedRWLock protect(lock, true);
  if (terrain_valid) {
    raster_tile_cache.Reset();
    terrain_valid = false;
  }
}


void RasterMapRaw::Close(void) {
  Poco::ScopedRWLock protect(lock, true);
  terrain_valid = false;
  if (TerrainMem) {
    free(TerrainMem); TerrainMem = NULL;
  }
}


void RasterMapCache::Close(void) {
  Poco::ScopedRWLock protect(lock, true);
  terrain_valid = false;
  if(fpTerrain) {
    if (ref_count==1) {
      zzip_fclose(fpTerrain);
      fpTerrain = NULL;
    }
  }
}


