
// http://mathworld.wolfram.com/Circle-LineIntersection.html
// line two points (x1,y1), (x2,y2)
// Circle radius r at (0,0)

#include <stdio.h>
#include "Math/FastMath.h"
#include "Math/NavFunctions.hpp"

#define sqr(x) ((x)*(x))
#define sgn(x) (x<0? -1:1)

struct Point {
  double x;
  double y;
  double cross(const Point& p2) const {
    return x*p2.y-p2.x*y;
  };
  void mul_y(const double a) {
    y*= a;
  };
  void sub(const Point&p2) {
    x -= p2.x;
    y -= p2.y;
  };
  void add(const Point&p2) {
    x += p2.x;
    y += p2.y;
  };
  void rotate(const double angle) {
    const double _x = x;
    const double _y = y;
    const double ca = fastcosine(angle);
    const double sa = fastsine(angle);
    x = _x*ca-_y*sa;
    y = _x*sa+_y*ca;
  };
  double d(const Point &p) const {
    return sqrt(sqr(p.x-x)+sqr(p.y-y));
  }
};

struct Line {
  Point p1;
  Point p2;

  Point ave() {
    Point p = p1;
    p.add(p2);
    p.x/= 2.0;
    p.y/= 2.0;
    return p;
  }
  double dx() const {
    return p2.x-p1.x;
  };
  double dy() const {
    return p2.y-p1.y;
  };
  double cross() const {
    return p1.cross(p2);
  };
  void mul_y(const double a) {
    p1.mul_y(a);
    p2.mul_y(a);
  };
  double d() const {
    return sqrt(dsq());
  };
  double dsq() const {
    const double _dx = dx();
    const double _dy = dy();
    return sqr(_dx)+sqr(_dy);
  };
  void sub(const Point&p) {
    p1.sub(p);
    p2.sub(p);
  };
  void add(const Point&p) {
    p1.add(p);
    p2.add(p);
  };
  double angle() {
    const double _dx = dx();
    const double _dy = dy();
    return atan2(_dy,_dx);
  };
  void rotate(const double theta) {
    p1.rotate(theta);
    p2.rotate(theta);
  }
};

struct Ellipse {
  Ellipse(const Point &f1,
          const Point &f2,
          const Point &ap) {

    Line f12;
    f12.p1 = f1;
    f12.p2 = f2;
    p = f12.ave();
    theta = RAD_TO_DEG*(f12.angle());
    const double c = f12.d()/2.0;
    a = (f1.d(ap)+f2.d(ap))/2.0;
    b = sqrt(a*a-c*c);
  }
  double theta;
  Point p;
  double a;
  double b;

  double er() const {
    return a/b;
  };
  Point parametric(const double t) {
    Point res;
    const double at = 360.0*t;
    res.x = a*fastcosine(at);
    res.y = b*fastsine(at);
    res.rotate(theta);
    res.add(p);
    return res;
  };
};

bool intersect_czero(const Line &line, 
                     const double r,
                     Point &i1, Point &i2) {

  const double dx = line.dx();
  const double dy = line.dy();
  const double dr = line.dsq();
  const double D = line.cross();

  double det = sqr(r)*dr-D*D;
  if (det<0) {
    // no solution
    return false;
  }
  det = sqrt(det);
  i1.x = (D*dy+sgn(dy)*dx*det)/dr;
  i2.x = (D*dy-sgn(dy)*dx*det)/dr;
  i1.y = (-D*dx+fabs(dy)*det)/dr;
  i2.y = (-D*dx-fabs(dy)*det)/dr;
  return true;
}


bool intersect_ellipse(const Line &line, 
                       const Ellipse& ell,
                       Point &i1, Point &i2) 
{
  const double er = ell.er();
  Line s_line = line;  

  s_line.sub(ell.p);
  s_line.rotate(-ell.theta);
  s_line.mul_y(er);

  if (intersect_czero(s_line, ell.a, i1, i2)) {

    i1.mul_y(1.0/er);
    i1.rotate(ell.theta);
    i1.add(ell.p);

    i2.mul_y(1.0/er);
    i2.rotate(ell.theta);
    i2.add(ell.p);

    return true;
  } else {
    return false;
  }
}

bool intersect_general_ellipse(const Line &line,
                               const Point &f1,
                               const Point &f2,
                               const Point &p,
                               Point &i1,
                               Point &i2) 
{
  Ellipse e(f1,f2,p);
  if (intersect_ellipse(line, e, i1, i2)) {
    return true;
  } else {
    return false;
  }
}

void test_ellipse() {
  Point f1, f2, p;
  f1.x = 0.5;
  f1.y = 0.0;
  f2.x = 1.0;
  f2.y = 0.5;
  p.x = 0.25;
  p.y = 0.2;
  Line l;
  l.p1.x = -1.5;
  l.p1.y = 0.2;
  l.p2.x = 1.7;
  l.p2.y = 0.7;

  Ellipse e(f1,f2,p);
  for (double t=0; t<=1.0; t+= 0.01) {
    Point a = e.parametric(t);
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

  Point i1, i2;
  if (intersect_ellipse(l,e,i1,i2)) {
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

