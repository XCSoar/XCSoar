/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_PERIOD_CLOCK_HPP
#define XCSOAR_PERIOD_CLOCK_HPP

#include <chrono>

/**
 * This is a stopwatch which saves the timestamp of an event, and can
 * check whether a specified time span has passed since then.
 */
class PeriodClock {
protected:
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;
  using Stamp = Clock::time_point;

private:
  Stamp last;

public:
  /**
   * Initializes the object, setting the last time stamp to "0",
   * i.e. a Check() will always succeed.  If you do not want this
   * default behaviour, call Update() immediately after creating the
   * object.
   */
  constexpr
  PeriodClock() = default;

protected:
  static auto GetNow() {
    return std::chrono::steady_clock::now();
  }

  constexpr auto Elapsed(Stamp now) const {
    return last > Stamp()
      ? now - last
      : Duration(-1);
  }

  template<class Rep, class Period>
  constexpr bool Check(Stamp now,
                       const std::chrono::duration<Rep,Period> &duration) const noexcept {
    return now >= last + Import(duration);
  }

  void Update(Stamp now) {
    last = now;
  }

public:
  constexpr bool IsDefined() const {
    return last > Stamp{};
  }

  /**
   * Resets the clock.
   */
  void Reset() {
    last = Stamp{};
  }

  /**
   * Returns the number of milliseconds elapsed since the last
   * update().  Returns -1 if update() was never called.
   */
  auto Elapsed() const {
    return Elapsed(GetNow());
  }

  /**
   * Combines a call to Elapsed() and Update().
   */
  auto ElapsedUpdate() {
    const auto now = GetNow();
    auto result = Elapsed(now);
    Update(now);
    return result;
  }

  /**
   * Checks whether the specified duration has passed since the last
   * update.
   *
   * @param duration the duration in milliseconds
   */
  template<class Rep, class Period>
  bool Check(const std::chrono::duration<Rep,Period> &duration) const noexcept {
    return Check(GetNow(), duration);
  }

  /**
   * Updates the time stamp, setting it to the current clock.
   */
  void Update() {
    Update(GetNow());
  }

  /**
   * Updates the time stamp, setting it to the current clock plus the
   * specified offset.
   */
  template<class Rep, class Period>
  constexpr void UpdateWithOffset(const std::chrono::duration<Rep,Period> &offset) noexcept {
    Update(GetNow() + Import(offset));
  }

  /**
   * Checks whether the specified duration has passed since the last
   * update.  If yes, it updates the time stamp.
   *
   * @param duration the duration in milliseconds
   */
  template<class Rep, class Period>
  bool CheckUpdate(const std::chrono::duration<Rep,Period> &duration) noexcept {
    Stamp now = GetNow();
    if (Check(now, duration)) {
      Update(now);
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
  template<class Rep, class Period>
  bool CheckAlwaysUpdate(const std::chrono::duration<Rep,Period> &duration) noexcept {
    Stamp now = GetNow();
    bool ret = Check(now, duration);
    Update(now);
    return ret;
  }

protected:
  template<class Rep, class Period>
  static auto Import(const std::chrono::duration<Rep,Period> &duration) noexcept {
    return std::chrono::duration_cast<Duration>(duration);
  }
};

#endif
