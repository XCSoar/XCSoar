#ifndef AATISOLINEINTERCEPT_HPP
#define AATISOLINEINTERCEPT_HPP

#include "AATIsoline.hpp"

class AATIsolineIntercept: public AATIsoline
{
public:
  AATIsolineIntercept(const AATPoint& ap,
                      const AIRCRAFT_STATE &state);

  bool intercept(const double bearing_offset,
                 GEOPOINT& ip) const;

private:
  const GEOPOINT& p_aircraft;
  const GEOPOINT& p_previous;
  const GEOPOINT& p_target;

  /** @link dependency */
  /*#  AATPoint lnkAATPoint; */
};

#endif
