#ifndef XCSOAR_COMPATIBILITY_MATH_H
#define XCSOAR_COMPATIBILITY_MATH_H

#include <math.h>

#ifndef HAVE_MSVCRT

static inline double
_hypot(double x, double y)
{
  return hypot(x, y);
}

#endif /* !HAVE_MSVCRT */

#endif
