#include "Filter.hpp"
#include <math.h>
#include <assert.h>
#include <stdio.h>

// ref http://unicorn.us.com/alex/2polefilters.html


Filter::Filter(const double cutoff_wavelength)
{
  double sample_freq = 1.0;
  double n= 1.0;
  // Bessel
  double c= pow((sqrt(pow(2.0,1.0/n)-0.75)-0.5),-0.5)/sqrt(3.0);
  double f_star = c/(sample_freq*cutoff_wavelength);

  assert(f_star<1.0/8.0);

  double omega0 = tan(3.1415926*f_star);
  double g= 3;
  double p= 3;
  double K1 = p*omega0;
  double K2 = g*omega0*omega0;

  a[0] = K2/(1.0+K1+K2);
  a[1] = 2*a[0];
  a[2] = a[0];
  b[0] = 2*a[0]*(1.0/K2-1.0);
  b[1] = 1.0-(a[0]+a[1]+a[2]+b[0]);

  reset(0.0);
}

double
Filter::reset(const double _x)
{
  x[0]= _x; y[0]=_x;
  x[1]= _x; y[1]=_x;
  x[2]= _x; 
  return _x;
}

double
Filter::update(const double _x)
{
  x[2]= x[1]; x[1]=x[0]; x[0]= _x;
  double _y = a[0]*x[0]+a[1]*x[1]+a[2]*x[2]+b[0]*y[0]+b[1]*y[1];
  y[1]= y[0]; y[0]= _y;
  return _y;
}
