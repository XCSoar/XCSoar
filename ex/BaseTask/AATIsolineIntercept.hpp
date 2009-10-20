#ifndef AATISOLINEINTERCEPT_HPP
#define AATISOLINEINTERCEPT_HPP

#include "AATIsoline.hpp"

class AATIsolineIntercept: public AATIsoline
{
public:
  AATIsolineIntercept(const AATPoint& ap);

  bool intercept(const AATPoint& ap,
                 const AIRCRAFT_STATE &state,
                 const double bearing_offset,
                 GEOPOINT& ip) const;
};

#endif
