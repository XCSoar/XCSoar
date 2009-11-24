#ifndef FLATLINE_HPP
#define FLATLINE_HPP

#include "FlatPoint.hpp"

/**
 * Defines a line in real-valued cartesian coordinates, with intersection
 * methods.
 */
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
  
  /** 
   * Calculate intersections between this line
   * and a circle of specified radius centered at the origin.
   * 
   * @param r Radius of circle
   * @param i1 Returned intersection point 1
   * @param i2 Returned intersection point 2
   * 
   * @return True if more than one intersection is found
   */
  bool intersect_czero(const double r,
                       FlatPoint &i1, FlatPoint &i2) const;
};

#endif
