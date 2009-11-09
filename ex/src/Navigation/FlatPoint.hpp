#ifndef FLATPOINT_HPP
#define FLATPOINT_HPP

struct FlatPoint 
{
  FlatPoint(const double _x, const double _y): x(_x),y(_y) {};
  FlatPoint(): x(0.0),y(0.0) {};

  double x;
  double y;
  double cross(const FlatPoint& p2) const;
  void mul_y(const double a);
  void sub(const FlatPoint&p2);
  void add(const FlatPoint&p2);
  void rotate(const double angle);
  double d(const FlatPoint &p) const;
};

#endif
