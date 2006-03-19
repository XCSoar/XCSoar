#ifndef RASTERTERRAIN_H
#define RASTERTERRAIN_H

#include "stdafx.h"
#include "Utils.h"
#include "Sizes.h"

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





class RasterTerrain {
public:

  RasterTerrain() {
    terraincacheefficiency=0;
    terraincachehits = 1;
    terraincachemisses = 1;
    cachetime = 0;
  }

  TERRAIN_CACHE TerrainCache[MAXTERRAINCACHE];

  int terraincacheefficiency;
  long terraincachehits;
  long terraincachemisses;
  unsigned int cachetime;
  int SortThresold;

  void SetCacheTime();
  void ClearTerrainCache();
  short LookupTerrainCache(const long &SeekPos);
  short LookupTerrainCacheFile(const long &SeekPos);
  short GetTerrainHeight(const double &Lattitude,
			 const double &Longditude);

  void OptimizeCash(void);

  static void OpenTerrain();
  static void CloseTerrain();
  static TERRAIN_INFO TerrainInfo;
  static FILE *fpTerrain;

  double fXrounding, fYrounding;
  int Xrounding, Yrounding;

  float GetTerrainStepSize();
  //  float GetTerrainSlopeStep();

  int GetEffectivePixelSize(double pixelsize);

  void SetTerrainRounding(double xr, double yr);

  BOOL isTerrainLoaded(){
    return(fpTerrain != NULL && TerrainInfo.StepSize != 0);
  }

};

extern RasterTerrain terrain_dem_graphics;
extern RasterTerrain terrain_dem_calculations;

#endif
