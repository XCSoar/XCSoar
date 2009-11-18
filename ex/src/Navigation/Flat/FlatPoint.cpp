#include "FlatPoint.hpp"
#include "Math/FastMath.h"
#include <algorithm>
#include <math.h>
#define sqr(x) ((x)*(x))

double FlatPoint::cross(const FlatPoint& p2) const {
  return x*p2.y-p2.x*y;
}

void FlatPoint::mul_y(const double a) {
  y*= a;
}
 
void FlatPoint::sub(const FlatPoint&p2) {
  x -= p2.x;
  y -= p2.y;
}

void FlatPoint::add(const FlatPoint&p2) {
  x += p2.x;
  y += p2.y;
}

void FlatPoint::rotate(const double angle) {
  const double _x = x;
  const double _y = y;
  const double ca = fastcosine(angle);
  const double sa = fastsine(angle);
  x = _x*ca-_y*sa;
  y = _x*sa+_y*ca;
}

double FlatPoint::d(const FlatPoint &p) const {
  return sqrt(sqr(p.x-x)+sqr(p.y-y));
}
