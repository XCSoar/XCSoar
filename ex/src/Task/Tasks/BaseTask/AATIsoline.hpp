#ifndef AATISOLINE_HPP
#define AATISOLINE_HPP

#include "Navigation/GeoEllipse.hpp"
#include "AATPoint.hpp"

/**
 *  Object representing an isoline, being the locus
 *  of potential target points within an AATPoint's observation
 *  zone such that all points have constant double-leg distance
 * (distance from previous max to this point to next planned).
 * 
 *  Internally, this is represented by an ellipse in flat-earth projection.
 */
class AATIsoline
{
public:
    /** 
     * Constructor.
     * 
     * @param ap The AAT point for which to calculate the Isoline
     */
  AATIsoline(const AATPoint& ap);
protected:
  const GeoEllipse ell;
  /** @link dependency */
  /*#  AATPoint lnkAATPoint; */
};


#endif
