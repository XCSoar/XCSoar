#include "Math/Earth.hpp"

#include <stdio.h>


int main() {

  double da = 20.0;
  double dsina = sin(da);

  fixed a = da;
  fixed sina = sin(a);

  printf("a=%g, sin(a)=%g\n",FIXED_DOUBLE(a),FIXED_DOUBLE(sina));
  printf("a=%g, sin(a)=%g\n",da, dsina);

  double dx=-0.3;
  double dy=0.6;
  double dt=atan2(dy,dx);

  fixed x=dx;
  fixed y=dy;
  fixed t=atan2(y,x);

  printf("x=%g, y=%g atan(y,x)=%g\n",FIXED_DOUBLE(x),FIXED_DOUBLE(y),FIXED_DOUBLE(t));
  printf("x=%g, y=%g atan(y,x)=%g\n",dx,dy,dt);


  GEOPOINT l1; l1.Longitude = 0.0; l1.Latitude=0.0;
  GEOPOINT l2; l2.Longitude = -0.3; l2.Latitude=1.0;
  fixed d, b;
  DistanceBearing(l1,l2,&d,&b);
  printf("Dist %g bearing %d\n",FIXED_DOUBLE(d),FIXED_INT(b));
}
