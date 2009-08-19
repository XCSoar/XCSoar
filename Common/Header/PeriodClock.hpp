#ifndef XCSOAR_PERIOD_CLOCK_HPP
#define XCSOAR_PERIOD_CLOCK_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/**
 * This is a stopwatch which saves the timestamp of an even, and can
 * check whether a specified time span has passed since then.
 */
class PeriodClock {
private:
  DWORD last;

public:
  /**
   * Initializes the object, setting the last time stamp to "0",
   * i.e. a check() will always succeed.  If you do not want this
   * default behaviour, call update() immediately after creating the
   * object.
   */
  PeriodClock():last(0) {}

  /**
   * Checks whether the specified duration has passed since the last
   * update.
   *
   * @param duration the duration in milliseconds
   */
  bool check(unsigned duration) {
    DWORD now = ::GetTickCount();
    return now >= last + duration;
  }

  /**
   * Updates the time stamp, setting it to the current clock.
   */
  void update() {
    last = ::GetTickCount();
  }

  /**
   * Checks whether the specified duration has passed since the last
   * update.  If yes, it updates the time stamp.
   *
   * @param duration the duration in milliseconds
   */
  bool check_update(unsigned duration) {
    DWORD now = ::GetTickCount();
    if (now >= last + duration) {
      last = now;
      return true;
    } else
      return false;
  }

  /**
   * Checks whether the specified duration has passed since the last
   * update.  After that, it updates the time stamp.
   *
   * @param duration the duration in milliseconds
   */
  bool check_always_update(unsigned duration) {
    DWORD now = ::GetTickCount();
    bool ret = now > last + duration;
    last = now;
    return ret;
  }
};

#endif
