#ifndef INTERSECTION_HPP
#define INTERSECTION_HPP


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

struct FlatLine 
{
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
  double angle();
  void rotate(const double theta);
  bool intersect_czero(const double r,
                       FlatPoint &i1, FlatPoint &i2) const;
};

struct FlatEllipse 
{
  FlatEllipse(const FlatPoint &f1,
              const FlatPoint &f2,
              const FlatPoint &ap);

  FlatPoint p;
  double a;
  double b;
  double theta;

  double er() const;

  FlatPoint parametric(const double t) const;

  bool intersect(const FlatLine &line, 
                 FlatPoint &i1, 
                 FlatPoint &i2) const;
};


#endif
