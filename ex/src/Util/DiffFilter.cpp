#include "DiffFilter.hpp"

void
DiffFilter::reset(double x0) 
{
  for (unsigned i=0; i<7; i++) {
    x[i]= x0;
  }
}

double
DiffFilter::update(double x0)
{
  x[6]= x[5];
  x[5]= x[4];
  x[4]= x[3];
  x[3]= x[2];
  x[2]= x[1];
  x[1]= x[0];
  x[0] = x0;
  /// \note not sure why need to divide by pi/2 here
  return ((x[6]-x[0])/16+x[2]-x[4])/1.5708;
}

