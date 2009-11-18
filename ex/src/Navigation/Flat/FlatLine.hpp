#ifndef FLATLINE_HPP
#define FLATLINE_HPP

#include "FlatPoint.hpp"

struct FlatLine 
{
  FlatLine(const FlatPoint _p1,
           const FlatPoint _p2):p1(_p1),p2(_p2) {};
  FlatLine() {};

  FlatPoint p1;
  FlatPoint p2;

  FlatPoint ave() const;
  double dx() const;
  double dy() const;
  double cross() const;
  void mul_y(const double a);
  double d() const;
  double dsq() const;
  void sub(const FlatPoint&p);
  void add(const FlatPoint&p);
  double angle() const;
  void rotate(const double theta);
  bool intersect_czero(const double r,
                       FlatPoint &i1, FlatPoint &i2) const;
};

#endif
