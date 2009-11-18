#include "FlatLine.hpp"
#include "Math/NavFunctions.hpp"

#define sqr(x) ((x)*(x))
#define sgn(x) (x<0? -1:1)

FlatPoint FlatLine::ave() const {
  FlatPoint p = p1;
  p.add(p2);
  p.x*= 0.5;
  p.y*= 0.5;
  return p;
}

double FlatLine::dx() const {
  return p2.x-p1.x;
}

double FlatLine::dy() const {
  return p2.y-p1.y;
}

double FlatLine::cross() const {
  return p1.cross(p2);
}

void FlatLine::mul_y(const double a) {
  p1.mul_y(a);
  p2.mul_y(a);
}

double FlatLine::d() const {
  return sqrt(dsq());
}

double FlatLine::dsq() const {
  const double _dx = dx();
  const double _dy = dy();
  return sqr(_dx)+sqr(_dy);
}

void FlatLine::sub(const FlatPoint&p) {
  p1.sub(p);
  p2.sub(p);
}

void 
FlatLine::add(const FlatPoint&p) {
  p1.add(p);
  p2.add(p);
}

double 
FlatLine::angle() const
{
  const double _dx = dx();
  const double _dy = dy();
  return RAD_TO_DEG*atan2(_dy,_dx);
}

void 
FlatLine::rotate(const double theta) 
{
  p1.rotate(theta);
  p2.rotate(theta);
}

bool 
FlatLine::intersect_czero(const double r,
                          FlatPoint &i1, FlatPoint &i2) const 
{
  const double _dx = dx();
  const double _dy = dy();
  const double dr = dsq();
  const double D = cross();
  
  double det = sqr(r)*dr-D*D;
  if (det<0) {
    // no solution
    return false;
  }
  det = sqrt(det);
  i1.x = (D*_dy+sgn(_dy)*_dx*det)/dr;
  i2.x = (D*_dy-sgn(_dy)*_dx*det)/dr;
  i1.y = (-D*_dx+fabs(_dy)*det)/dr;
  i2.y = (-D*_dx-fabs(_dy)*det)/dr;
  return true;
}
