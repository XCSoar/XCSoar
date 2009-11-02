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

struct FlatEllipse 
{
  FlatEllipse(const FlatPoint &_f1,
              const FlatPoint &_f2,
              const FlatPoint &_ap);

  FlatEllipse():p(0.0,0.0),
                a(1.0),b(1.0),theta(0.0),
                theta_initial(0.0),
                f1(0.0,0.0),
                f2(0.0,0.0),
                ap(0.0,0.0) 
    {
    };

  FlatPoint f1, f2, ap;

  FlatPoint p;
  double a;
  double b;
  double theta;

  double theta_initial;

  double ab() const;
  double ba() const;

  FlatPoint parametric(const double t) const;

  bool intersect(const FlatLine &line, 
                 FlatPoint &i1, 
                 FlatPoint &i2) const;

  bool intersect_extended(const FlatPoint &p,
                          FlatPoint &i1,
                          FlatPoint &i2) const;
};


#endif
