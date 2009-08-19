#include "Screen/Ramp.hpp"

void ColorRampLookup(const short h,
                     BYTE &r, BYTE &g, BYTE &b,
                     const COLORRAMP* ramp_colors,
                     const int numramp,
                     const unsigned char interp_levels) {

  unsigned short f, of;
  unsigned short is = 1<<interp_levels;

  /* Monochrome

     #ifndef NDEBUG
     r = 0xDA;
     g = 0xDA;
     b = 0xDA;
     return;
     #endif

  */


  // gone past end, so use last color
  if (h>=ramp_colors[numramp-1].h) {
    r = ramp_colors[numramp-1].r;
    g = ramp_colors[numramp-1].g;
    b = ramp_colors[numramp-1].b;
    return;
  }
  for (unsigned int i=numramp-2; i--; ) {
    if (h>=ramp_colors[i].h) {
      f = (unsigned short)(h-ramp_colors[i].h)*is/
        (unsigned short)(ramp_colors[i+1].h-ramp_colors[i].h);
      of = is-f;

      r = (f*ramp_colors[i+1].r+of*ramp_colors[i].r) >> interp_levels;
      g = (f*ramp_colors[i+1].g+of*ramp_colors[i].g) >> interp_levels;
      b = (f*ramp_colors[i+1].b+of*ramp_colors[i].b) >> interp_levels;
      return;
    }
  }

  // check if h lower than lowest
  if (h<=ramp_colors[0].h) {
    r = ramp_colors[0].r;
    g = ramp_colors[0].g;
    b = ramp_colors[0].b;
    return;
  }
}
