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
  virtual double f(const double x) = 0;
  virtual bool valid(const double x) {
    if ((x<=xmax)&&(x>=xmin)) {
      return true;
    } else {
      return false;
    }
  }
  double find_zero(const double xstart) {
    double x = _find_zero(xstart);
    return x_limited(x);
  }
  double find_min(const double xstart) {
    double x = _find_min(xstart);
    return x_limited(x);
  }
protected:
  const double xmin;
  const double xmax;
  const double tolerance;
private:
  double f_limited(double x);
  double x_limited(double x);
  double _find_zero(const double xstart);
  double _find_min(const double xstart);
};

#endif
