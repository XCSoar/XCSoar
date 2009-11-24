#ifndef DIFF_FILTER_HPP
#define DIFF_FILTER_HPP

/**
 * Differentiating low-pass IIR filter
 * adapted from http://www.dsprelated.com/showarticle/35.php
 */
class DiffFilter 
{
public:
/** 
 * Constructor.  Initialises as if fed x_default continuously.
 * 
 * @param x_default Default value of input
 */
  DiffFilter(double x_default=0.0) {
    reset(x_default,0);
  }

/** 
 * Updates low-pass differentiating filter to calculate filtered output given an input sample
 * 
 * @param x0 Input (pre-filtered) value at sample time
 * 
 * @return Filter output value
 */
  double update(const double x0);

/** 
 * Resets filter as if fed value to produce y0
 * 
 * @param x0 Steady state value of filter input
 * @param y0 Desired value of differentiated output
 * 
 */
  void reset(const double x0, const double y0);

private:
  double x[7];
};


#endif
