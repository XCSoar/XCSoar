#include "Math/Earth.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

int main(int argc, char** argv) {
  plan_tests(19);

  /* check the division operator */
  ok((fixed_one / fixed_one) * fixed(1000) == fixed(1000), "1/1", 0);
  ok((fixed_two / fixed_two) * fixed(1000) == fixed(1000), "2/2", 0);
  ok((fixed_one / fixed_two) * fixed(1000) == fixed(500), "1/2", 0);
  ok((fixed(1000) / fixed(100)) * fixed(1000) == fixed(10000), "1000/100", 0);
  ok((fixed(100) / fixed(20)) * fixed(1000) == fixed(5000), "100/20", 0);
  ok((fixed(1000000) / fixed(2)) * fixed(1000) == fixed(500000000), "1M/2", 0);
  ok((fixed_minus_one / fixed_one) * fixed(1000) == -fixed(1000), "-1/1", 0);
  ok((fixed_one / fixed_minus_one) * fixed(1000) == -fixed(1000), "1/-1", 0);
  ok((fixed_minus_one / fixed_minus_one) * fixed(1000) == fixed(1000), "-1/-1", 0);
  ok((fixed(-1000000) / fixed(2)) * fixed(1000) == -fixed(500000000), "-1M/2", 0);
  ok((long)((fixed_one / (fixed_one / fixed(10))) * fixed(1000)) == (10000), "1/0.1", 0);
  ok((long)((fixed_one / (fixed_one / fixed(-10))) * fixed(1000)) == -(10000) ||
     (long)((fixed_one / (fixed_one / fixed(-10))) * fixed(1000)) == -(10001), "1/-0.1", 0);

  ok(equals(fixed_one / fixed_half, 2), "1/0.5", 0);
  ok(equals(fixed(1000) / fixed_half, 2000), "1/0.5", 0);
  ok(equals(fixed(1000) / (fixed_one / 5), 5000), "1/0.5", 0);

  ok(equals(fixed(1000000) / (fixed_one / 5), 5000000), "1/0.5", 0);
  ok(equals(fixed(10000000) / (fixed_one / 5), 50000000), "1/0.5", 0);

  double da = 20.0;
  double dsina = sin(da);

  fixed a(da);
  fixed sina(sin(a));

  printf("a=%g, sin(a)=%g\n",FIXED_DOUBLE(a),FIXED_DOUBLE(sina));
  printf("a=%g, sin(a)=%g\n",da, dsina);

  ok(fabs(sina-fixed(dsina))<fixed(1.0e5),"sin(a)",0);

  double dx=-0.3;
  double dy=0.6;
  double dt=atan2(dy,dx);

  fixed x(dx);
  fixed y(dy);
  fixed t(atan2(y, x));

  printf("x=%g, y=%g atan(y,x)=%g\n",FIXED_DOUBLE(x),FIXED_DOUBLE(y),FIXED_DOUBLE(t));
  printf("x=%g, y=%g atan(y,x)=%g\n",dx,dy,dt);

  ok(fabs(t-fixed(dt))<fixed(1.0e5),"atan(y,x)",0);

  GeoPoint l1; 
  l1.Longitude = Angle::degrees(fixed(0.0)); 
  l1.Latitude= Angle::degrees(fixed(0.0));
  GeoPoint l2; 
  l2.Longitude = Angle::degrees(fixed(-0.3)); 
  l2.Latitude= Angle::degrees(fixed(1.0));
  fixed d; Angle b;
  ::DistanceBearing(l1,l2,&d,&b);
  printf("Dist %g bearing %d\n",FIXED_DOUBLE(d),FIXED_INT(b.value_degrees()));

  return exit_status();
}
