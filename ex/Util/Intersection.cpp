
// http://mathworld.wolfram.com/Circle-FlatLineIntersection.html
// line two points (x1,y1), (x2,y2)
// Circle radius r at (0,0)

#include "Intersection.hpp"
#include "Math/FastMath.h"
#include "Math/NavFunctions.hpp"

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
  p.x/= 2.0;
  p.y/= 2.0;
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
FlatLine::angle() 
{
  const double _dx = dx();
  const double _dy = dy();
  return atan2(_dy,_dx);
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


FlatEllipse::FlatEllipse(const FlatPoint &f1,
                         const FlatPoint &f2,
                         const FlatPoint &ap) 
{
  FlatLine f12;
  f12.p1 = f1;
  f12.p2 = f2;
  p = f12.ave();
  theta = RAD_TO_DEG*(f12.angle());
  const double c = f12.d()/2.0;
  a = (f1.d(ap)+f2.d(ap))/2.0;
  b = sqrt(a*a-c*c);
}

double 
FlatEllipse::er() const {
  return a/b;
}

FlatPoint 
FlatEllipse::parametric(const double t) const {
  const double at = 360.0*t;
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
  const double _er = er();
  FlatLine s_line = line;  
  
  s_line.sub(p);
  s_line.rotate(-theta);
  s_line.mul_y(_er);
  
  if (s_line.intersect_czero(a, i1, i2)) {
    
    i1.mul_y(1.0/_er);
    i1.rotate(theta);
    i1.add(p);
    
    i2.mul_y(1.0/_er);
    i2.rotate(theta);
    i2.add(p);
    
    return true;
  } else {
    return false;
  }
}


bool intersect_general_ellipse(const FlatLine &line,
                               const FlatPoint &f1,
                               const FlatPoint &f2,
                               const FlatPoint &p,
                               FlatPoint &i1,
                               FlatPoint &i2) 
{
  FlatEllipse e(f1,f2,p);
  if (e.intersect(line, i1, i2)) {
    return true;
  } else {
    return false;
  }
}


#include <stdio.h>

void test_ellipse() {
  FlatPoint f1(0.5,0.0);
  FlatPoint f2(1.0, 0.5);
  FlatPoint p(0.25,0.2);

  FlatLine l;
  l.p1.x = -1.5;
  l.p1.y = 0.2;
  l.p2.x = 1.7;
  l.p2.y = 0.7;

  FlatEllipse e(f1,f2,p);
  for (double t=0; t<=1.0; t+= 0.01) {
    FlatPoint a = e.parametric(t);
    printf("%g %g\n",a.x,a.y);
  }
  printf("\n");

  printf("%g %g\n",l.p1.x,l.p1.y);
  printf("\n");
  printf("%g %g\n",l.p2.x,l.p2.y);
  printf("\n");
  printf("%g %g\n",f1.x,f1.y);
  printf("\n");
  printf("%g %g\n",f2.x,f2.y);
  printf("\n");

  FlatPoint i1, i2;
  if (e.intersect(l,i1,i2)) {
    printf("%g %g\n",i1.x,i1.y);
    printf("%g %g\n",i2.x,i2.y);
    printf("\n");
  }
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

