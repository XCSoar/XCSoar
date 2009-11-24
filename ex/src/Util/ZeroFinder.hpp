#ifndef ZERO_FINDER_HPP
#define ZERO_FINDER_HPP

/**
 * Zero finding and minimisation search algorithm
 */
class ZeroFinder {
public:
/** 
 * Constructor of zero finder search algorithm
 * 
 * @param _xmin Minimum allowable value of x
 * @param _xmax Maximum allowable value of x
 * @param _tolerance Absolute tolerance of solution (in x)
 */
  ZeroFinder(const double _xmin, const double _xmax, const double _tolerance): 
    xmin(_xmin),
    xmax(_xmax),
    tolerance(_tolerance)
    {
    };

/** 
 * Abstract method for function to be minimised or root-solved 
 * 
 * @param x Value of x
 * 
 * @return f(x)
 */
  virtual double f(const double x) = 0;

/** 
 * Check whether x is within search limits.  For use by sub-classes.
 * 
 * @param x Value of x
 * 
 * @return True if x is within search limits
 */
  virtual bool valid(const double x) {
    if ((x<=xmax)&&(x>=xmin)) {
      return true;
    } else {
      return false;
    }
  }

/** 
 * Find closest value of x that produces f(x)=0
 * Method used is a variant of a bisector search. 
 *
 * @param xstart Initial value of x (not used)
 * 
 * @return x value of best solution
 */
  double find_zero(const double xstart);

/** 
 * Find value of x that minimises f(x)
 * Method used is a variant of a bisector search. 
 * 
 * @param xstart Initial value of x (not used)
 * 
 * @return x value of best solution
 */
  double find_min(const double xstart);

protected:
  const double xmin; /**< min value of search range */
  const double xmax; /**< max value of search range */
  const double tolerance; /**< search tolerance */
  static const double epsilon; /**< machine tolerance */
  static const double sqrt_epsilon; /**< sqrt of machine tolerance */
};

#endif
