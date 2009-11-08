#ifndef GRADIENT_HPP
#define GRADIENT_HPP

/** 
 * Convert angle or (approximate) inverse-gradient to gradient.
 * Where absolute value of gradient is greater than 999 or undefined,
 * the value is limited to 999
 *
 * @param d Angle (radians) or inverse gradient
 * 
 * @return Gradient equivalent to angle
 */
double AngleToGradient(const double d);

#endif
