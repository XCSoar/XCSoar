/*! \file
 * \brief Library for calculating Earth dimensions
 *
 * This library provides general functions for calculating dimensions
 * on the Earth with GPS coordinates.
 */

#ifndef XCSOAR_MATH_EARTH_HPP
#define XCSOAR_MATH_EARTH_HPP

/**
 * Finds cross track error in meters and closest point P4 between P3
 * and desired track P1-P2.  Very slow function!
 */
double CrossTrackError(double lon1, double lat1,
                       double lon2, double lat2,
                       double lon3, double lat3,
                       double *lon4, double *lat4);

/**
 * Calculates projected distance from P3 along line P1-P2.
 */
double ProjectedDistance(double lon1, double lat1,
                         double lon2, double lat2,
                         double lon3, double lat3);

void DistanceBearing(double lat1, double lon1, double lat2, double lon2,
                     double *Distance, double *Bearing);
double DoubleDistance(double lat1, double lon1, double lat2, double lon2,
		      double lat3, double lon3);

void FindLatitudeLongitude(double Lat, double Lon,
                           double Bearing, double Distance,
                           double *lat_out, double *lon_out);

#endif
