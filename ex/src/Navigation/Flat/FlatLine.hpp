#ifndef FLATLINE_HPP
#define FLATLINE_HPP

#include "FlatPoint.hpp"

/**
 * Defines a line in real-valued cartesian coordinates, with intersection
 * methods.
 */
struct FlatLine 
{
/** 
 * Constructor given known start/end points
 * 
 * @param _p1 Start of line
 * @param _p2 End of line
 * 
 * @return Initialised object
 */
  FlatLine(const FlatPoint _p1,
           const FlatPoint _p2):p1(_p1),p2(_p2) {};

/** 
 * Constructor default
 * 
 * @return Initialised object at origin
 */
  FlatLine() {};

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

/** 
 * Find center point of this line
 * 
 * @return Center point
 */
  FlatPoint ave() const;

/** 
 * Find angle of this line
 * 
 * @return Angle (deg)
 */
  double angle() const;

/** 
 * Calculate squared length of line
 * 
 * @return Squared length
 */
  double dsq() const;

/** 
 * Calculate length of line
 * 
 * @return Length
 */
  double d() const;

/** 
 * Subtract a delta from the line (both start and end points)
 * 
 * @param p Point to subtract
 */
  void sub(const FlatPoint&p);

/** 
 * Add a delta to the line (both start and end points)
 * 
 * @param p Point to add
 */
  void add(const FlatPoint&p);

/** 
 * Rotate line clockwise around origin
 * 
 * @param angle Angle (deg) to rotate line clockwise
 */
  void rotate(const double angle);

/** 
 * Scale line in Y direction
 * 
 * @param a Scale ratio
 */
  void mul_y(const double a);

private:
  FlatPoint p1;
  FlatPoint p2;

  double dx() const;
  double dy() const;
  double cross() const;
};

#endif
