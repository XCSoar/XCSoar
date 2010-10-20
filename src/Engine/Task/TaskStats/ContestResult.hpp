#ifndef CONTEST_RESULT_HPP
#define CONTEST_RESULT_HPP

struct ContestResult
{
  /** Score (pts) according to OLC rule */
  fixed score;
  /** Optimum distance (m) travelled according to OLC rule */
  fixed distance;
  /** Time (s) of optimised OLC path */
  fixed time;
  /** Speed (m/s) of optimised OLC path */
  fixed speed;

  ContestResult() :
    score(fixed_zero),
    distance(fixed_zero),
    time(fixed_zero),
    speed(fixed_zero) {}
};

#endif
