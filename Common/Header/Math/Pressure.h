#ifndef XCSOAR_MATH_PRESSURE_H
#define XCSOAR_MATH_PRESSURE_H

extern double QNH;

#ifdef __cplusplus
extern "C" {
#endif

double FindQNH(double alt_raw, double alt_known);

/**
 * Converts altitude with QNH=1013.25 reference to QNH adjusted
 * altitude
 */
double AltitudeToQNHAltitude(double alt);

double StaticPressureToAltitude(double ps);

double AirDensity(double altitude);

/**
 * divide TAS by this number to get IAS
 */
double AirDensityRatio(double altitude);

#ifdef __cplusplus
}
#endif

#endif
