#ifndef XCSOAR_SCREEN_RAMP_HPP
#define XCSOAR_SCREEN_RAMP_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct _COLORRAMP
{
  short h;
  unsigned char r;
  unsigned char g;
  unsigned char b;
} COLORRAMP;

void ColorRampLookup(short h, BYTE &r, BYTE &g, BYTE &b,
                     const COLORRAMP* ramp_colors,
                     const int numramp,
                     const unsigned char interp_bits=6);

#endif
