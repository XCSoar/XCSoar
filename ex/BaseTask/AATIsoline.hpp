#ifndef AATISOLINE_HPP
#define AATISOLINE_HPP

#include "Navigation/GeoEllipse.hpp"
#include "AATPoint.hpp"

class AATIsoline
{
public:
  AATIsoline(const AATPoint& ap);
protected:
  const GeoEllipse ell;
  /** @link dependency */
  /*#  AATPoint lnkAATPoint; */
};


#endif
