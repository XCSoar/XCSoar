#ifndef TERRAIN_H
#define TERRAIN_H
#include "StdAfx.h"
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
extern short TerrainRamp;
extern bool reset_marks;

void ColorRampLookup(short h, BYTE &r, BYTE &g, BYTE &b,
		     const COLORRAMP* ramp_colors,
                     const int numramp,
                     const unsigned char interp_bits=6);

void SetTopologyBounds(const RECT rcin, const bool force=false);
void TopologyInitialiseMarks();
void TopologyCloseMarks();
void OpenTopology();
void CloseTopology();
void DrawTopology(const HDC hdc, const RECT rc);
void DrawTerrain(const HDC hdc, const RECT rc, const double sunazimuth, const double sunelevation);
void DrawSpotHeights(const HDC hdc);
void DrawMarks(const HDC hdc, const RECT rc);
void MarkLocation(const double lon, const double lat);
void CloseTerrainRenderer();
#endif
