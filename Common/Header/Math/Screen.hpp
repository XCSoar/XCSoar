/*! \file
 * \brief Library for calculating on-screen coordinates
 */

#ifndef XCSOAR_MATH_SCREEN_HPP
#define XCSOAR_MATH_SCREEN_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void ScreenClosestPoint(const POINT &p1, const POINT &p2,
                        const POINT &p3, POINT *p4, int offset);

void PolygonRotateShift(POINT* poly, int n, int x, int y,
                        double angle);

#endif
