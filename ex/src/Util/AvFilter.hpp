#ifndef AV_FILTER_HPP
#define AV_FILTER_HPP

#include <vector>

/**
 * Average/bucket filter.  When filter is full, can return samples
 */
class AvFilter 
{
public:
  /**
   * Constructor, reserves fized size of bucket
   *
   * @param _n_max Number of elements in bucket
   */
  AvFilter(const unsigned _n_max):n_max(_n_max)  {
    x.reserve(n_max);
    reset();
  }

/** 
 * Updates filter to add sample to buffer
 * 
 * @param x0 Input (pre-filtered) value at sample time
 * 
 * @return True if buffer is full
 */
  bool update(const double x0);

/** 
 * Calculate average from samples
 * 
 * @return Average value in buffer
 */
  double average();

/** 
 * Resets filter (zero samples)
 * 
 */
  void reset();

private:
  std::vector<double> x;
  unsigned n;
  const unsigned n_max;
};

#endif
