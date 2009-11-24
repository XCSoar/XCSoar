#ifndef FLATELLIPSE_HPP
#define FLATELLIPSE_HPP

#include "FlatPoint.hpp"
#include "FlatLine.hpp"

/**
 * 2-d ellipse in real-valued projected coordinates, with methods for
 * intersection tests etc.  The ellipse itself need not be axis-aligned.
 */
struct FlatEllipse 
{
  /** 
   * Constructor.
   * 
   * @param _f1 Focus A
   * @param _f2 Focus B
   * @param _ap Any point on the ellipse
   * 
   * @return Initialised object
   */
  FlatEllipse(const FlatPoint &_f1,
              const FlatPoint &_f2,
              const FlatPoint &_ap);

/** 
 * Dummy constructor for zero-sized ellipse
 * 
 * @return Initialised object
 */
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
