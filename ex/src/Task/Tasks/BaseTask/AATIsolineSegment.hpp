#ifndef AATISOLINESEGMENT_HPP
#define AATISOLINESEGMENT_HPP

#include "AATIsoline.hpp"

class AATIsolineSegment: public AATIsoline
{
public:
  AATIsolineSegment(const AATPoint& ap);

  bool valid() const;

  GEOPOINT parametric(const double t) const;

private:
  double t_up;
  double t_down;
};

#endif
