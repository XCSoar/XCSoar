#ifndef TERRAIN_H
#define TERRAIN_H
#include "stdafx.h"
#include "Topology.h"

typedef struct _COLORRAMP
{
  short h;
  unsigned char r;
  unsigned char g;
  unsigned char b;
} COLORRAMP;

extern short TerrainContrast;
extern short TerrainBrightness;

void ColorRampLookup(short h, BYTE &r, BYTE &g, BYTE &b,
		     COLORRAMP* ramp_colors, const int numramp);

void SetTopologyBounds(const RECT rcin, const bool force=false);
void ReadTopology();
void OpenTopology();
void CloseTopology();
void DrawTopology(const HDC hdc, const RECT rc);
void DrawTerrain(const HDC hdc, const RECT rc, const double sunazimuth, const double sunelevation);
void DrawMarks(const HDC hdc, const RECT rc);
void MarkLocation(const double lon, const double lat);
rectObj GetRectBounds(const RECT rc);
void OptimizeTerrainCache();
void CloseTerrainRenderer();
#endif
