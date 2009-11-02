#include "Gradient.hpp"
#include <algorithm>
#include <math.h>

double AngleToGradient(const double d)
{
  if (fabs(d)) {
    return std::min(999.0,std::max(-999.0,1.0/d));
  } else {
    return 999.0;
  }
}
