
// http://mathworld.wolfram.com/Circle-FlatLineIntersection.html
// line two points (x1,y1), (x2,y2)
// Circle radius r at (0,0)

#include "Intersection.hpp"
#include "Math/FastMath.h"
#include "Math/NavFunctions.hpp"
#include "Math/Geometry.hpp"
#include <algorithm>

#define sqr(x) ((x)*(x))
#define sgn(x) (x<0? -1:1)

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

FlatEllipse::FlatEllipse(const FlatPoint &_f1,
                         const FlatPoint &_f2,
                         const FlatPoint &_ap):
  f1(_f1),f2(_f2),ap(_ap)
{
  const FlatLine f12(f1,f2);
  p = f12.ave();
  theta = f12.angle();
  const double csq = f12.dsq();
  a = (f1.d(ap)+f2.d(ap));
  b = sqrt(a*a-csq)*0.5;
  a *= 0.5;

  // a.sin(t)=ap.x
  // b.cos(t)=ap.y

  FlatPoint op = ap;
  op.sub(p);
  op.rotate(-theta);
  theta_initial = RAD_TO_DEG*atan2(op.y*a, op.x*b);
}

double 
FlatEllipse::ab() const {
  return a/b;
}

double 
FlatEllipse::ba() const {
  return b/a;
}

FlatPoint 
FlatEllipse::parametric(const double t) const {
  const double at = 360.0*t+theta_initial;
  FlatPoint res(a*fastcosine(at),b*fastsine(at));
  res.rotate(theta);
  res.add(p);
  return res;
}

bool 
FlatEllipse::intersect(const FlatLine &line, 
                       FlatPoint &i1, 
                       FlatPoint &i2) const 
{
  const double er = ab();
  const double ier = ba();
  FlatLine s_line = line;  
  
  s_line.sub(p);
  s_line.rotate(-theta);
  s_line.mul_y(er);
  
  if (s_line.intersect_czero(a, i1, i2)) {
    
    i1.mul_y(ier);
    i1.rotate(theta);
    i1.add(p);
    
    i2.mul_y(ier);
    i2.rotate(theta);
    i2.add(p);
    
    return true;
  } else {
    return false;
  }
}


bool FlatEllipse::intersect_extended(const FlatPoint &p,
                                     FlatPoint &i1,
                                     FlatPoint &i2) const
{
  const FlatLine l_f1p(f1,p);
  const FlatLine l_pf2(p,f2);
  const double ang = l_f1p.angle();

  double d = l_pf2.d()+std::max(a,b); // max line length

  FlatLine e_l(p,FlatPoint(p.x+d*fastcosine(ang),
                           p.y+d*fastsine(ang)));
  // e_l is the line extended from p in direction of f1-p 
  
  return intersect(e_l, i1, i2);
}

// define an ellipse by three points,
// edge point, focus f1, focus f2

// r1+r2 = 2.a
// 

// e = sqrt(1.0-sqr(b/a)) = c/a
// .: c = a. sqrt(1.0-sqr(b/a))
//    c = sqrt(a*a-b*b)
//    c^2 = a*a-b*b

// .: 2.c = dist between f1 and f2

// given df12 = dist between f1 and f2
//        r1 = dist between f1 and p
//        r2 = dist between f2 and p
//
//    c = df12/2.0
//    a = (r1+r2)/2.0
//    b = sqrt(a*a-c*c)


// distances don't need to be shifted (a,b ok to calc)
// only line does

// w = (f2+f1)/2
// theta = atan2(f2-f1)

// shift line by -w
// rotate line by -theta
// solve intersection
// rotate result by theta
// shift by w

