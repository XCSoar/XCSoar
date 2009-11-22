#include "AvFilter.hpp"

bool
AvFilter::update(const double x0) 
{
  if (n<n_max) {
    x[n]= x0;
    n++;
  }
  return (n>=n_max);
}

double
AvFilter::average()
{
  double y=0;
  for (unsigned i=0; i<n; i++) {
    y+= x[i];
  }
  return y/n;
}

void
AvFilter::reset()
{
  n=0;
}
