#ifndef FLATELLIPSE_HPP
#define FLATELLIPSE_HPP

#include "FlatPoint.hpp"
#include "FlatLine.hpp"

struct FlatEllipse 
{
  FlatEllipse(const FlatPoint &_f1,
              const FlatPoint &_f2,
              const FlatPoint &_ap);

  FlatEllipse():f1(0.0,0.0),
                f2(0.0,0.0),
                ap(0.0,0.0), 
                p(0.0,0.0),
                a(1.0),b(1.0),theta(0.0),
                theta_initial(0.0)
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
