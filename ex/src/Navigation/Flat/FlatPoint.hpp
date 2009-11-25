#ifndef FLATPOINT_HPP
#define FLATPOINT_HPP

/**
 * 2-d Cartesian projected real-valued point
 */
struct FlatPoint 
{
/** 
 * Constructor given known location
 * 
 * @param _x X position
 * @param _y Y position
 * 
 * @return Initialised object
 */
  FlatPoint(const double _x, const double _y): x(_x),y(_y) {};

/** 
 * Constructor at origin
 * 
 * @return Initialised object
 */
  FlatPoint(): x(0.0),y(0.0) {};

  double x; /**< X location */
  double y; /**< Y location */

/** 
 * Calculate cross product of two points
 * 
 * @param p2 Other point
 * 
 * @return Cross product
 */
  double cross(const FlatPoint& p2) const;

/** 
 * Multiply Y value of point
 * 
 * @param a Value to multiply
 */
  void mul_y(const double a);

/** 
 * Subtract delta from this point
 * 
 * @param p2 Point to subtract
 */
  void sub(const FlatPoint&p2);
/** 
 * Add delta to this point
 * 
 * @param p2 Point to add
 */
  void add(const FlatPoint&p2);

/** 
 * Rotate point clockwise around origin
 * 
 * @param angle Angle (deg) to rotate point clockwise
 */
  void rotate(const double angle);

/** 
 * Calculate distance between two points
 * 
 * @param p Other point
 * 
 * @return Distance
 */
  double d(const FlatPoint &p) const;
};

#endif
