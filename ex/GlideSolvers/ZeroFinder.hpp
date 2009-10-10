#ifndef ZERO_FINDER_HPP
#define ZERO_FINDER_HPP

class ZeroFinder {
public:
  ZeroFinder(const double _xmin, const double _xmax, const double _tolerance): 
    xmin(_xmin),
    xmax(_xmax),
    tolerance(_tolerance)
    {
    };
  virtual double f(double x) = 0;
  virtual bool valid(double x) {
    if ((x<=xmax)&&(x>=xmin)) {
      return true;
    } else {
      return false;
    }
  }
  double find_zero(const double xstart);
  double find_min(const double xstart);
protected:
  const double xmin;
  const double xmax;
  const double tolerance;
};

#endif
