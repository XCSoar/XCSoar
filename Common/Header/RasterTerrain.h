#ifndef RASTERTERRAIN_H
#define RASTERTERRAIN_H

#include "StdAfx.h"
#include "Utils.h"
#include "Sizes.h"
#include <zzip/lib.h>
#include "jasper/RasterTile.h"

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

  double fXrounding, fYrounding;
  double fXroundingFine, fYroundingFine;
  int Xrounding, Yrounding;
  int xlleft;
  int xlltop;

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

  bool DirectAccess;
  bool Paged;

 protected:
  bool terrain_valid;
  bool DirectFine;

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

  TERRAIN_CACHE TerrainCache[MAXTERRAINCACHE];

  void ServiceCache();

  int terraincacheefficiency;
  long terraincachehits;
  long terraincachemisses;
  unsigned int cachetime;
  int SortThresold;

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

  char jp2_filename[MAX_PATH];
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
  virtual short _GetFieldAtXY(unsigned int lx,
                              unsigned int ly);
  bool TriggerJPGReload;
  static CRITICAL_SECTION  CritSec_TerrainFile;
  static int ref_count;
  RasterTileCache raster_tile_cache;
};


class RasterTerrain {
public:

  RasterTerrain() {
    terrain_initialised = false;
  }

  static void SetViewCenter(const double &Latitude,
                            const double &Longitude);
  static void OpenTerrain();
  static void CloseTerrain();
  static bool terrain_initialised;
  static bool isTerrainLoaded() {
    return terrain_initialised;
  }
  static RasterMap* TerrainMap;
  static bool CreateTerrainMap(char *zfilename);

 public:
  static void Lock(void);
  static void Unlock(void);
  static short GetTerrainHeight(const double &Latitude,
                                const double &Longitude);
  static bool IsDirectAccess(void);
  static bool IsPaged(void);
  static void SetTerrainRounding(double x, double y);
  static void ServiceCache();
  static void ServiceTerrainCenter(double latitude, double longitude);
  static void ServiceFullReload(double latitude, double longitude);
  static int GetEffectivePixelSize(double *pixel_D,
                                   double latitude, double longitude);
  static bool WaypointIsInTerrainRange(double latitude, double longitude);
  static bool GetTerrainCenter(double *latitude,
                               double *longitude);
  static int render_weather;
};

#define MAX_WEATHER_MAP 16
#define MAX_WEATHER_TIMES 48

class RasterWeather {
public:
  RasterWeather() {
    int i;
    bsratio = false;
    for (i=0; i<MAX_WEATHER_MAP; i++) {
      weather_map[i]= 0;
    }
    for (i=0; i<MAX_WEATHER_TIMES; i++) {
      weather_available[i]= false;
    }
    weather_time = 0;
  }
  ~RasterWeather() {
    Close();
  }
 public:
  void Close();
  void Reload(double lat, double lon);
  int weather_time;
  RasterMap* weather_map[MAX_WEATHER_MAP];
  void RASP_filename(char* rasp_filename, const TCHAR* name);
  bool LoadItem(int item, const TCHAR* name);
  void SetViewCenter(double lat, double lon);
  void ServiceFullReload(double lat, double lon);
  void ValueToText(TCHAR* Buffer, short val);
  void ItemLabel(int i, TCHAR* Buffer);
  void Scan(double lat, double lon);
  bool weather_available[MAX_WEATHER_TIMES];
  int IndexToTime(int x);
 private:
  bool bsratio;
};

extern RasterWeather RASP;

#endif
