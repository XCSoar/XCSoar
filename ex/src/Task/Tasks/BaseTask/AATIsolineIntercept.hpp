#ifndef AATISOLINEINTERCEPT_HPP
#define AATISOLINEINTERCEPT_HPP

#include "AATIsoline.hpp"

/**
 * Specialisation of AATIsoline to calculate intercepts between
 * a line extending from the aircraft to the isoline.
 *
 */
class AATIsolineIntercept: public AATIsoline
{
public:
    /** 
     * Constructor.
     * 
     * @param ap The AAT point for which the isoline is sought
     * 
     * @return Initialised object
     */
  AATIsolineIntercept(const AATPoint& ap);

/** 
 * Calculate intercept location.  Test line bearing is from previous
 * max/achieved point, through aircraft, adjusted by bearing_offset.
 * 
 * @param ap AAT point associated with the isoline
 * @param state Aircraft state from which intercept line originates
 * @param bearing_offset Offset of desired bearing between cruise track from previous and intercept line
 * @param ip Set location of intercept point (if returned true)
 * 
 * @return True if intercept is found and within OZ 
 */
  bool intercept(const AATPoint& ap,
                 const AIRCRAFT_STATE &state,
                 const double bearing_offset,
                 GEOPOINT& ip) const;
};

#endif
