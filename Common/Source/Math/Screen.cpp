#include "Math/Screen.hpp"
#include "Math/Geometry.hpp"
#include "Math/FastMath.h"
#include "Utils.h"
#include "InfoBoxLayout.h"

#include <math.h>

void ScreenClosestPoint(const POINT &p1, const POINT &p2,
                        const POINT &p3, POINT *p4, int offset)
{
  int v12x, v12y, v13x, v13y;

  v12x = p2.x-p1.x; v12y = p2.y-p1.y;
  v13x = p3.x-p1.x; v13y = p3.y-p1.y;

  int mag12 = isqrt4(v12x*v12x+v12y*v12y);
  if (mag12>1) {
    // projection of v13 along v12 = v12.v13/|v12|
    int proj = (v12x*v13x+v12y*v13y)/mag12;
    // fractional distance
    double f;
    if (offset>0) {
      if (offset*2<mag12) {
        proj = max(0, min(proj, mag12));
        proj = max(offset, min(mag12-offset, proj+offset));
      } else {
        proj = mag12/2;
      }
    }
    f = min(1.0,max(0.0,(double)proj/mag12));

    // location of 'closest' point
    p4->x = lround(v12x*f)+p1.x;
    p4->y = lround(v12y*f)+p1.y;
  } else {
    p4->x = p1.x;
    p4->y = p1.y;
  }
}

void PolygonRotateShift(POINT* poly, const int n, const int xs, const int ys, const double angle) {
  static double lastangle = -1;
  static int cost=1024, sint=0;

  if(angle != lastangle) {
    lastangle = angle;
    int deg = DEG_TO_INT(AngleLimit360(angle));
    cost = ICOSTABLE[deg]*InfoBoxLayout::scale;
    sint = ISINETABLE[deg]*InfoBoxLayout::scale;
  }
  const int xxs = xs*1024+512;
  const int yys = ys*1024+512;
  POINT *p = poly;
  const POINT *pe = poly+n;

  while (p<pe) {
    int x= p->x;
    int y= p->y;
    p->x = (x*cost - y*sint + xxs)/1024;
    p->y = (y*cost + x*sint + yys)/1024;
    p++;
  }
}
