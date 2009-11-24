#ifndef FILTER_HPP
#define FILTER_HPP

/**
 * Basic low-pass FIR filter from 2-pole design
 */
class Filter 
{
public:
/** 
 * Constructor, designs low-pass FIR filter
 * 
 * @param cutoff_wavelength 3dB cutoff wavelength (in cycles) of filter design
 * @param bessel If true, generates Bessel filter, otherwise critically damped filter
 */
  Filter(const double cutoff_wavelength,
    const bool bessel=true);

/** 
 * Resets filter to produce static value
 * 
 * @param x0 Steady state value of filter output
 * 
 * @return Filter output value
 */
  double reset(const double x0);

/** 
 * Updates low-pass filter to calculate filtered output given an input sample
 * 
 * @param x0 Input (pre-filtered) value at sample time
 * 
 * @return Filter output value
 */
  double update(const double x0);

private:
  double a[3];
  double b[2];
  double x[3];
  double y[2];
};


#endif
