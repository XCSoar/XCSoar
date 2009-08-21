#ifndef XCSOAR_SNAIL_TRAIL_HPP
#define XCSOAR_SNAIL_TRAIL_HPP

#include "Sizes.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct _SNAIL_POINT
{
  float Latitude;
  float Longitude;
  float Vario;
  double Time;
  POINT Screen;
  short Colour;
  BOOL Circling;
  bool FarVisible;
  double DriftFactor;
} SNAIL_POINT;

extern SNAIL_POINT SnailTrail[TRAILSIZE];
extern	int SnailNext;

#endif
