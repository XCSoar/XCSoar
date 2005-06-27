#ifndef TERRAIN_H
#define TERRAIN_H
#include "stdafx.h"

typedef struct _COLORRAMP
{
  short h;
  unsigned char r;
  unsigned char g;
  unsigned char b;
} COLORRAMP;

void ColorRampLookup(short h, BYTE *r, BYTE *g, BYTE *b,
		     COLORRAMP* ramp_colors, int numramp);

void SetTopologyBounds(RECT rcin);
void ReadTopology();
void OpenTopology();
void CloseTopology();
void DrawTopology( HDC hdc, RECT rc);
void DrawTerrain(HDC hdc, RECT rc, double sunazimuth, double sunelevation);
void DrawMarks(HDC hdc, RECT rc);
void MarkLocation(double lon, double lat);
#endif
