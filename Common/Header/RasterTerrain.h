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

  void SetCacheTime();
  void ClearTerrainCache();
  short LookupTerrainCache(long SeekPos);
  short LookupTerrainCacheFile(long SeekPos);
  short GetTerrainHeight(double Lattitude, 
			 double Longditude);

  static void OpenTerrain();
  static void CloseTerrain();
  static TERRAIN_INFO TerrainInfo;
  static HANDLE hTerrain;

  int rounding;

  float GetTerrainStepSize();
  float GetTerrainSlopeStep();

  void SetTerrainRounding(double dist);

};

extern RasterTerrain terrain_dem_graphics;
extern RasterTerrain terrain_dem_calculations;

#endif
