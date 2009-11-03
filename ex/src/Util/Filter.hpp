#ifndef FILTER_HPP
#define FILTER_HPP

class Filter 
{
public:
  Filter(const double cutoff_wavelength);
  double reset(const double x0);
  double update(const double x0);
private:
  double a[3];
  double b[2];
  double x[3];
  double y[2];
};


#endif
