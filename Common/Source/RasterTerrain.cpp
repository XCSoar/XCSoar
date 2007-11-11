/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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


#include "stdafx.h"
#include "RasterTerrain.h"
#include "XCSoar.h"
#include "Dialogs.h"
#include "Units.h"
#include "Process.h"
#include "externs.h"


// JMW experimental
#include "jasper/jasper.h"
#include "jasper/jpc_rtc.h"
#include "wcecompat/ts_string.h"



// static variables shared between rasterterrains because can only
// have file opened by one reader



////// Rounding control ////////////////////////////////////////////////////


bool RasterMap::GetMapCenter(double *lon, double *lat) {
  if(!isMapLoaded())
    return false;

  *lon = (TerrainInfo.Left + TerrainInfo.Right)/2;
  *lat = (TerrainInfo.Top + TerrainInfo.Bottom)/2;
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
                                     double latitude, double longitude)
{
  double terrain_step_x, terrain_step_y;
  double step_size = TerrainInfo.StepSize*sqrt(2.0);

  if ((*pixel_D<=0) || (step_size==0)) {
    *pixel_D = 1.0;
    return 1;
  }

  // how many steps are in the pixel size
  DistanceBearing(latitude, longitude, latitude+step_size,
                  longitude, &terrain_step_x, NULL);
  terrain_step_x = fabs(terrain_step_x);

  DistanceBearing(latitude, longitude, latitude,
                  longitude+step_size, &terrain_step_y, NULL);
  terrain_step_y = fabs(terrain_step_y);

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


void RasterMap::SetFieldRounding(double xr, double yr) {
  if (!isMapLoaded()) {
    return;
  }

  Xrounding = iround(xr/TerrainInfo.StepSize);
  Yrounding = iround(yr/TerrainInfo.StepSize);

  if (Xrounding<1) {
    Xrounding = 1;
  }
  fXrounding = 1.0/(Xrounding*TerrainInfo.StepSize);
  fXroundingFine = fXrounding*256.0;
  if (Yrounding<1) {
    Yrounding = 1;
  }
  fYrounding = 1.0/(Yrounding*TerrainInfo.StepSize);
  fYroundingFine = fYrounding*256.0;

  DirectFine = false;
}


void RasterMapJPG2000::SetFieldRounding(double xr, double yr) {
  RasterMap::SetFieldRounding(xr, yr);
  if (!isMapLoaded()) {
    return;
  }
  if ((Xrounding==1)&&(Yrounding==1)) {
    DirectFine = true;
    xlleft = (int)(TerrainInfo.Left*fXroundingFine)+128;
    xlltop  = (int)(TerrainInfo.Top*fYroundingFine)-128;
  } else {
    DirectFine = false;
  }
}


void RasterMapRaw::SetFieldRounding(double xr, double yr) {
  RasterMap::SetFieldRounding(xr, yr);
  if (!isMapLoaded()) {
    return;
  }
  if ((Xrounding==1)&&(Yrounding==1)) {
    DirectFine = true;
    xlleft = (int)(TerrainInfo.Left*fXroundingFine)+128;
    xlltop  = (int)(TerrainInfo.Top*fYroundingFine)-128;
  } else {
    DirectFine = false;
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
    return -1;
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

  unsigned int ix = CombinedDivAndMod(lx);
  unsigned int iy = CombinedDivAndMod(ly);

  if ((ly>=(unsigned int)TerrainInfo.Rows)
      ||(lx>=(unsigned int)TerrainInfo.Columns)) {
    return -1;
  }
  return LookupTerrainCache((ly*TerrainInfo.Columns+lx)*2
                            +sizeof(TERRAIN_INFO));
}


////////// Map general /////////////////////////////////////////////


// JMW rounding further reduces data as required to speed up terrain
// display on low zoom levels
short RasterMap::GetField(const double &Lattitude,
                          const double &Longditude)
{
  if(isMapLoaded()) {
    if (DirectFine) {
      return _GetFieldAtXY((int)(Longditude*fXroundingFine)-xlleft,
                           xlltop- (int)(Lattitude*fYroundingFine));
    } else {
      unsigned int ix =
        Real2Int((Longditude-TerrainInfo.Left)*fXrounding)*Xrounding;
      unsigned int iy =
        Real2Int((TerrainInfo.Top-Lattitude)*fYrounding)*Yrounding;

      return _GetFieldAtXY(ix<<8, iy<<8);
    }
  } else {
    return -1;
  }
}

//////////// Cached load on demand ////////////////////////////////

//////////// Cache map ////////////////////////////////////////////////
CRITICAL_SECTION RasterMapCache::CritSec_TerrainFile;

int RasterMapCache::ref_count = 0;

ZZIP_FILE *RasterMapCache::fpTerrain;

void RasterMapCache::ServiceCache(void) {
  Lock();

  if (terraincachemisses > 0){
    DWORD tm =GetTickCount();
    OptimizeCash();
    tm = GetTickCount()-tm;
    tm = GetTickCount();
  }
  SetCacheTime();

  Unlock();
}


void RasterMapCache::Lock(void) {
  EnterCriticalSection(&CritSec_TerrainFile);
}


void RasterMapCache::Unlock(void) {
  LeaveCriticalSection(&CritSec_TerrainFile);
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

void RasterMapCache::OptimizeCash(void){
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
    return -1;

  Lock();

  SeekRes = zzip_seek(fpTerrain, SeekPos, SEEK_SET);
  if(SeekRes != SeekPos) {
    // error, not found!
    Alt = -1;
  } else {
    if (zzip_fread(&NewAlt, 1, sizeof(__int16), fpTerrain) != sizeof(__int16))
      Alt = -1;
    else {
      Alt = NewAlt;
      if(Alt<0) Alt = -1;
    }
  }
  Unlock();

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
    return -1;

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

//////////// Raw //////////////////////////////////////////////////


void RasterMapRaw::Lock(void) {
  EnterCriticalSection(&CritSec_TerrainFile);
}

void RasterMapRaw::Unlock(void) {
  LeaveCriticalSection(&CritSec_TerrainFile);
}



//////////// JPG2000 //////////////////////////////////////////////////

CRITICAL_SECTION RasterMapJPG2000::CritSec_TerrainFile;

void RasterMapJPG2000::ServiceFullReload(double lat, double lon) {
  ReloadJPG2000Full(lat, lon);
}


RasterMapJPG2000::RasterMapJPG2000() {
  TriggerJPGReload = false;
  jp2_filename[0] = '\0';
  DirectAccess = true;
  Paged = true;
  if (ref_count==0) {
    jas_init();
    InitializeCriticalSection(&CritSec_TerrainFile);
  }
  ref_count++;
}

RasterMapJPG2000::~RasterMapJPG2000() {
  ref_count--;
  if (ref_count==0) {
    jas_cleanup();
    DeleteCriticalSection(&CritSec_TerrainFile);
  }
}


int RasterMapJPG2000::ref_count = 0;

void RasterMapJPG2000::Lock(void) {
  EnterCriticalSection(&CritSec_TerrainFile);
}

void RasterMapJPG2000::Unlock(void) {
  LeaveCriticalSection(&CritSec_TerrainFile);
}


void RasterMapJPG2000::ReloadJPG2000Full(double latitude,
                                         double longitude) {
  // load all 16 tiles...
  for (int i=0; i<MAX_ACTIVE_TILES; i++) {
    TriggerJPGReload = true;
    SetViewCenter(latitude, longitude);
  }
}


void RasterMapJPG2000::ReloadJPG2000(void) {
  if (TriggerJPGReload) {

    Lock();
    TriggerJPGReload = false;

    raster_tile_cache.LoadJPG2000(jp2_filename);
    if (raster_tile_cache.GetInitialised()) {
      TerrainInfo.Left = raster_tile_cache.lon_min;
      TerrainInfo.Right = raster_tile_cache.lon_max;
      TerrainInfo.Top = raster_tile_cache.lat_max;
      TerrainInfo.Bottom = raster_tile_cache.lat_min;
      TerrainInfo.Columns = raster_tile_cache.width;
      TerrainInfo.Rows = raster_tile_cache.height;
      TerrainInfo.StepSize = (raster_tile_cache.lon_max -
                              raster_tile_cache.lon_min)
        /raster_tile_cache.width;
    }
    Unlock();
  }
}


void RasterMapJPG2000::SetViewCenter(const double &Latitude,
                                     const double &Longitude) {
  Lock();
  if (raster_tile_cache.GetInitialised()) {
    int x = lround((Longitude-TerrainInfo.Left)*TerrainInfo.Columns
                   /(TerrainInfo.Right-TerrainInfo.Left));
    int y = lround((TerrainInfo.Top-Latitude)*TerrainInfo.Rows
                   /(TerrainInfo.Top-TerrainInfo.Bottom));
    TriggerJPGReload |= raster_tile_cache.PollTiles(x, y);
    ReloadJPG2000();
  }
  Unlock();
}


///////////////////// General, open/close ///////////////////////////////

RasterMap* RasterTerrain::TerrainMap = NULL;
bool RasterTerrain::terrain_initialised = false;
int RasterTerrain::render_weather = 0;

extern TCHAR szRegistryTerrainFile[];

void RasterTerrain::OpenTerrain(void)
{
  render_weather = 0;
  terrain_initialised = false;

  StartupStore(TEXT("OpenTerrain\n"));
  CreateProgressDialog(gettext(TEXT("Loading Terrain File...")));
  SetProgressStepSize(2);

  TCHAR  szFile[MAX_PATH] = TEXT("\0");

  GetRegistryString(szRegistryTerrainFile, szFile, MAX_PATH);

  TCHAR szOrigFile[MAX_PATH] = TEXT("\0");
  char zfilename[MAX_PATH];

  ExpandLocalPath(szFile);
  _tcscpy(szOrigFile, szFile);
  ContractLocalPath(szOrigFile);

  SetRegistryString(szRegistryTerrainFile, TEXT("\0"));
  unicode2ascii(szFile, zfilename, MAX_PATH);

  if (strlen(zfilename)==0) {
    static TCHAR  szMapFile[MAX_PATH] = TEXT("\0");
    GetRegistryString(szRegistryMapFile, szMapFile, MAX_PATH);
    ExpandLocalPath(szMapFile);
    _tcscpy(szFile,szMapFile);
    wcscat(szFile,TEXT("/terrain.jp2"));
    unicode2ascii(szFile, zfilename, MAX_PATH);
  }

  // TODO: Check locking, especially when reloading a file.
  // Fix cache method

  if (CreateTerrainMap(zfilename)) {
    SetRegistryString(szRegistryTerrainFile, szOrigFile);
    terrain_initialised = true;
  } else {
    if (TerrainMap) {
      TerrainMap->Close();
      delete TerrainMap;
      TerrainMap = NULL;
    }
    terrain_initialised = false;
  }
}


bool RasterTerrain::CreateTerrainMap(char *zfilename) {
  if (strstr(zfilename,".jp2")) {
    TerrainMap = new RasterMapJPG2000();
    if (!TerrainMap)
      return false;
    return TerrainMap->Open(zfilename);
  }
  TerrainMap = new RasterMapRaw();
  if (!TerrainMap) {
    return false;
  }
  if (TerrainMap->Open(zfilename)) {
    return true;
  }
  TerrainMap->Close();
  delete TerrainMap;

  TerrainMap = new RasterMapCache();
  if (!TerrainMap) {
    return false;
  }
  if (TerrainMap->Open(zfilename)) {
    return true;
  }
  return false;
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
      fpTerrain = false;
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

  int filesize = zzip_file_size(fpTerrain);

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
  if (terrain_valid) {
    Lock();
    raster_tile_cache.Reset();
    terrain_valid = false;
    Unlock();
  }
}


void RasterMapRaw::Close(void) {
  terrain_valid = false;
  if (TerrainMem) {
    free(TerrainMem); TerrainMem = NULL;
  }
}


void RasterMapCache::Close(void) {
  terrain_valid = false;
  if(fpTerrain) {
    if (ref_count==1) {
      zzip_fclose(fpTerrain);
      fpTerrain = NULL;
    }
  }
}


void RasterTerrain::CloseTerrain(void)
{
  StartupStore(TEXT("CloseTerrain\n"));

  render_weather = 0;

  // TODO: lock it first?

  if (terrain_initialised) {

    if (TerrainMap) {
      TerrainMap->Close();
      delete TerrainMap;
      TerrainMap = NULL;
    }
    terrain_initialised = false;
  }
}


////////

void RasterTerrain::Lock(void) {
  if (TerrainMap) {
    TerrainMap->Lock();
  }
}

void RasterTerrain::Unlock(void) {
  if (TerrainMap) {
    TerrainMap->Unlock();
  }
}

short RasterTerrain::GetTerrainHeight(const double &Latitude,
                                      const double &Longitude) {
  if (TerrainMap) {
    return TerrainMap->GetField(Latitude, Longitude);
  } else {
    return -1;
  }
}

bool RasterTerrain::IsDirectAccess(void) {
  if (TerrainMap) {
    return TerrainMap->DirectAccess;
  } else {
    return false;
  }
}


bool RasterTerrain::IsPaged(void) {
  if (TerrainMap) {
    return TerrainMap->Paged;
  } else {
    return false;
  }
}


void RasterTerrain::ServiceCache(void) {
  Lock();
  if (TerrainMap) {
    TerrainMap->ServiceCache();
  }
  Unlock();
}

void RasterTerrain::SetTerrainRounding(double x, double y) {
  if (TerrainMap) {
    TerrainMap->SetFieldRounding(x, y);
  }
}

void RasterTerrain::ServiceTerrainCenter(double lat, double lon) {
  Lock();

  RASP.SetViewCenter(lat, lon);

  if (TerrainMap) {
    TerrainMap->SetViewCenter(lat, lon);
  }
  Unlock();
}


void RasterTerrain::ServiceFullReload(double lat, double lon) {

  Lock();
  if (TerrainMap) {
    CreateProgressDialog(gettext(TEXT("Loading terrain tiles...")));
    TerrainMap->ServiceFullReload(lat, lon);
  }
  Unlock();
}


int RasterTerrain::GetEffectivePixelSize(double *pixel_D,
                                         double latitude, double longitude) {
  if (TerrainMap) {
    return TerrainMap->GetEffectivePixelSize(pixel_D, latitude, longitude);
  } else {
    return 1;
  }
}


bool RasterTerrain::WaypointIsInTerrainRange(double latitude,
                                             double longitude) {
  if (TerrainMap) {
    if ((latitude<= TerrainMap->TerrainInfo.Top)&&
        (latitude>= TerrainMap->TerrainInfo.Bottom)&&
        (longitude<= TerrainMap->TerrainInfo.Right)&&
        (latitude>= TerrainMap->TerrainInfo.Left)) {
      return true;
    } else {
      return false;
    }
  } else {
    return true;
  }
}


bool RasterTerrain::GetTerrainCenter(double *latitude,
                                     double *longitude) {
  if (TerrainMap) {
    *latitude = (TerrainMap->TerrainInfo.Top+
                 TerrainMap->TerrainInfo.Bottom)/2.0;
    *longitude = (TerrainMap->TerrainInfo.Left+
                  TerrainMap->TerrainInfo.Right)/2.0;
    return true;
  } else {
    return false;
  }
}


////////// Weather map ////////////////////////////////////////////

int RasterWeather::IndexToTime(int x) {
  if (x % 2 == 0) {
    return (x/2)*100;
  } else {
    return (x/2)*100+30;
  }
}


void RasterWeather::RASP_filename(char* rasp_filename,
                                  const TCHAR* name) {
  TCHAR fname[MAX_PATH];
  _stprintf(fname,
            TEXT("xcsoar-rasp.dat/%s.curr.%04dlst.d2.jp2"),
            name, IndexToTime(weather_time));
  LocalPathS(rasp_filename, fname);
}

bool RasterWeather::LoadItem(int item, const TCHAR* name) {
  char rasp_filename[MAX_PATH];
  RASP_filename(rasp_filename, name);
  weather_map[item] = new RasterMapJPG2000();
  weather_map[item]->Open(rasp_filename);
  if (!weather_map[item]->isMapLoaded()) {
    weather_map[item]->Close();
    delete weather_map[item];
    weather_map[item]= 0;
    return false;
  }
  return true;
};


void RasterWeather::Scan(double lat, double lon) {
  int i;
  for (i=0; i<MAX_WEATHER_TIMES; i++) {
    weather_time = i;
    weather_available[i] = LoadItem(0,TEXT("wstar"));
    if (!weather_available[i]) {
      weather_available[i] = LoadItem(0,TEXT("wstar_bsratio"));
      if (weather_available[i]) {
        bsratio = true;
      }
    }
    Close();
  }
  weather_time = 0;
}


void RasterWeather::Reload(double lat, double lon) {
  static last_weather_time = -1;
  bool found = false;
  bool now = false;

  if (RasterTerrain::render_weather == 0) {
    // will be drawing terrain
    return;
  }

  if (weather_time== 0) {
    // "Now" time, so find time in half hours
    int dsecs = (int)TimeLocal((long)GPS_INFO.Time);
    int half_hours = (dsecs/1800) % 48;
    weather_time = max(weather_time, half_hours);
    now = true;
  }

  // limit values, for safety
  weather_time = min(MAX_WEATHER_TIMES-1, max(0, weather_time));

  if (weather_time == last_weather_time) {
    // no change, quick exit.
    if (now) {
      // must return to 0 = Now time on exit
      weather_time = 0;
    }
    return;
  } else {
    last_weather_time = weather_time;
  }

  // scan forward to next valid time
  while ((weather_time<MAX_WEATHER_TIMES) && (!found)) {
    if (!weather_available[weather_time]) {
      weather_time++;
    } else {
      found = true;

      Close();
      if (bsratio) {
        LoadItem(0,TEXT("wstar_bsratio"));
      } else {
        LoadItem(0,TEXT("wstar"));
      }
      LoadItem(1,TEXT("blwindspd"));
      LoadItem(2,TEXT("hbl"));
      LoadItem(3,TEXT("dwcrit"));
      LoadItem(4,TEXT("blcloudpct"));
      LoadItem(5,TEXT("sfctemp"));
      LoadItem(6,TEXT("hwcrit"));
      LoadItem(7,TEXT("wblmaxmin"));
      LoadItem(8,TEXT("blcwbase"));
    }
  }

  // can't find valid time, so reset to zero
  if (!found || now) {
    weather_time = 0;
  }

  SetViewCenter(lat, lon);
  ServiceFullReload(lat, lon);
}


void RasterWeather::Close() {
  // todo: locking!
  int i;
  for (i=0; i<MAX_WEATHER_MAP; i++) {
    if (weather_map[i]) {
      weather_map[i]->Close();
      delete weather_map[i];
      weather_map[i]=0;
    }
  }
}


void RasterWeather::SetViewCenter(double lat, double lon) {
  for (int i=0; i<MAX_WEATHER_MAP; i++) {
    if (weather_map[i]) {
      weather_map[i]->SetViewCenter(lat, lon);
    }
  }
}


void RasterWeather::ServiceFullReload(double lat, double lon) {
  for (int i=0; i<MAX_WEATHER_MAP; i++) {
    if (weather_map[i]) {
      weather_map[i]->ServiceFullReload(lat, lon);
    }
  }
}


void RasterWeather::ItemLabel(int i, TCHAR* Buffer) {
  _stprintf(Buffer, TEXT("\0"));

  switch (i) {
  case 0:
    return;
  case 1: // wstar
    _stprintf(Buffer, gettext(TEXT("W*")));
    return;
  case 2: // blwindspd
    _stprintf(Buffer, gettext(TEXT("BL Wind spd")));
    return;
  case 3: // hbl
    _stprintf(Buffer, gettext(TEXT("H bl")));
    return;
  case 4: // dwcrit
    _stprintf(Buffer, gettext(TEXT("dwcrit")));
    return;
  case 5: // blcloudpct
    _stprintf(Buffer, gettext(TEXT("bl cloud")));
    return;
  case 6: // sfctemp
    _stprintf(Buffer, gettext(TEXT("Sfc temp")));
    return;
  case 7: // hwcrit
    _stprintf(Buffer, gettext(TEXT("hwcrit")));
    return;
  case 8: // wblmaxmin
    _stprintf(Buffer, gettext(TEXT("wblmaxmin")));
    return;
  case 9: // blcwbase
    _stprintf(Buffer, gettext(TEXT("blcwbase")));
    return;
  default:
    // error!
    break;
  }
}


void RasterWeather::ValueToText(TCHAR* Buffer, short val) {
  Buffer[0]=0;
  switch (RasterTerrain::render_weather) {
  case 0:
    return;
  case 1: // wstar
    _stprintf(Buffer, TEXT("%.1f%s"), ((val-200)/100.0)*LIFTMODIFY, Units::GetVerticalSpeedName());
    return;
  case 2: // blwindspd
    _stprintf(Buffer, TEXT("%.0f%s"), (val/100.0)*SPEEDMODIFY, Units::GetHorizontalSpeedName());
    return;
  case 3: // hbl
    _stprintf(Buffer, TEXT("%.0f%s"), val*ALTITUDEMODIFY, Units::GetAltitudeName());
    return;
  case 4: // dwcrit
    _stprintf(Buffer, TEXT("%.0f%s"), val*ALTITUDEMODIFY, Units::GetAltitudeName());
    return;
  case 5: // blcloudpct
    _stprintf(Buffer, TEXT("%d%%"), max(0,min(100,val)));
    return;
  case 6: // sfctemp
    _stprintf(Buffer, TEXT("%d°"), iround(val*0.5-20.0));
    return;
  case 7: // hwcrit
    _stprintf(Buffer, TEXT("%.0f%s"), val*ALTITUDEMODIFY, Units::GetAltitudeName());
    return;
  case 8: // wblmaxmin
    _stprintf(Buffer, TEXT("%.1f%s"), ((val-200)/100.0)*LIFTMODIFY, Units::GetVerticalSpeedName());
    return;
  case 9: // blcwbase
    _stprintf(Buffer, TEXT("%.0f%s"), val*ALTITUDEMODIFY, Units::GetAltitudeName());
    return;
  default:
    // error!
    break;
  }
}


///////////

RasterWeather RASP;

