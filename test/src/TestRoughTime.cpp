// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "time/RoughTime.hpp"
#include "TestUtil.hpp"

using namespace std::chrono_literals;

static void test_rough_time()
{
  RoughTime a = RoughTime::Invalid();
  ok1(!a.IsValid());

  a = RoughTime(12, 1);
  ok1(a.IsValid());
  ok1(a.GetHour() == 12);
  ok1(a.GetMinute() == 1);
  ok1(a.GetMinuteOfDay() == 12 * 60 + 1);

  /* compare a RoughTime with itself */
  ok1(a == a);
  ok1(!(a != a));
  ok1(a <= a);
  ok1(a >= a);
  ok1(!(a < a));
  ok1(!(a > a));

  /* compare a RoughTime with another one */
  RoughTime b(11, 59);
  ok1(b.IsValid());
  ok1(a != b);
  ok1(!(a == b));
  ok1(!(a <= b));
  ok1(a >= b);
  ok1(!(a < b));
  ok1(a > b);
  ok1(b != a);
  ok1(!(b == a));
  ok1(b <= a);
  ok1(!(b >= a));
  ok1(b < a);
  ok1(!(b > a));

  /* test factory functions */
  ok1(RoughTime::FromMinuteOfDayChecked((unsigned)(12*60+1)) == RoughTime(12,1) );
  ok1(RoughTime::FromMinuteOfDayChecked((unsigned)(12*60+1+24*60)) == RoughTime(12,1) );
  ok1(RoughTime::FromMinuteOfDayChecked((int)(12*60+1)) == RoughTime(12,1) );
  ok1(RoughTime::FromMinuteOfDayChecked((int)(12*60+1+24*60)) == RoughTime(12,1) );
  ok1(RoughTime::FromMinuteOfDayChecked((int)(12*60+1-24*60)) == RoughTime(12,1) );

  ok1(RoughTime::FromSinceMidnightChecked(12h) == RoughTime(12,0) );
  ok1(RoughTime::FromSinceMidnightChecked(12h+1min) == RoughTime(12,1) );
  ok1(RoughTime::FromSinceMidnightChecked(12h+1s) == RoughTime(12,0) );
  ok1(RoughTime::FromSinceMidnightChecked(12h+1min+24h) == RoughTime(12,1) );
  ok1(RoughTime::FromSinceMidnightChecked(12h+1min-24h) == RoughTime(12,1) );
  
  ok1(RoughTime( TimeStamp(12h) ) == RoughTime(12,0) );
  ok1(RoughTime( TimeStamp(12h+1min) ) == RoughTime(12,1) );
  ok1(RoughTime( TimeStamp(12h+1s) ) == RoughTime(12,0) );
  ok1(RoughTime( TimeStamp(12h+1min+24h) ) == RoughTime(12,1) );
  ok1(RoughTime( TimeStamp(12h+1min-24h) ) == RoughTime(12,1) );

  /* test midnight wraparound */
  a = RoughTime(0, 0);
  b = RoughTime(23, 59);
  ok1(b.IsValid());
  ok1(a != b);
  ok1(!(a == b));
  ok1(!(a <= b));
  ok1(a >= b);
  ok1(!(a < b));
  ok1(a > b);
  ok1(b != a);
  ok1(!(b == a));
  ok1(b <= a);
  ok1(!(b >= a));
  ok1(b < a);
  ok1(!(b > a));
}

static void test_rough_time_span()
{
  RoughTime a(12, 1);
  RoughTime b(11, 59);

  /* test RoughTimeSpan::IsInside() */
  RoughTimeSpan s = RoughTimeSpan::Invalid();
  ok1(!s.IsDefined());
  ok1(s.IsInside(a));
  ok1(s.IsInside(b));

  s = RoughTimeSpan(RoughTime(12, 0), RoughTime::Invalid());
  ok1(s.IsDefined());
  ok1(s.IsInside(a));
  ok1(!s.IsInside(b));

  s = RoughTimeSpan(RoughTime::Invalid(), RoughTime(12, 0));
  ok1(s.IsDefined());
  ok1(!s.IsInside(a));
  ok1(s.IsInside(b));

  s = RoughTimeSpan(RoughTime(12, 0), RoughTime(12, 1));
  ok1(s.IsDefined());
  ok1(!s.IsInside(a));
  ok1(!s.IsInside(b));

  s = RoughTimeSpan(RoughTime(12, 0), RoughTime(12, 30));
  ok1(s.IsDefined());
  ok1(s.IsInside(a));
  ok1(!s.IsInside(b));

  /* test midnight wraparound */
  a = RoughTime(0, 0);
  b = RoughTime(23, 59);
  RoughTime c(22, 0);
  RoughTime d(2, 0);
  s = RoughTimeSpan(RoughTime(23, 0), RoughTime::Invalid());
  ok1(s.IsDefined());
  ok1(s.IsInside(a));
  ok1(s.IsInside(b));
  ok1(!s.IsInside(c));
  ok1(s.IsInside(d));

  s = RoughTimeSpan(RoughTime::Invalid(), RoughTime(1, 0));
  ok1(s.IsDefined());
  ok1(s.IsInside(a));
  ok1(s.IsInside(b));
  ok1(s.IsInside(c));
  ok1(!s.IsInside(d));

  s = RoughTimeSpan(RoughTime(23, 1), RoughTime(0, 30));
  ok1(s.IsDefined());
  ok1(s.IsInside(a));
  ok1(s.IsInside(b));
  ok1(!s.IsInside(c));
  ok1(!s.IsInside(d));
}

static void test_rough_time_delta()
{
  /* test operator+(RoughTime, RoughTimeDelta) */
  ok1(RoughTime(0, 0) + RoughTimeDelta::FromMinutes(0) == RoughTime(0, 0));
  ok1(RoughTime(0, 0) + RoughTimeDelta::FromMinutes(1) == RoughTime(0, 1));
  ok1(RoughTime(0, 0) + RoughTimeDelta::FromMinutes(60) == RoughTime(1, 0));
  ok1(RoughTime(0, 0) + RoughTimeDelta::FromHours(24) == RoughTime(0, 0));
  ok1(RoughTime(0, 0) + RoughTimeDelta::FromHours(25) == RoughTime(1, 0));

  ok1(RoughTime(0, 0) - RoughTimeDelta::FromMinutes(0) == RoughTime(0, 0));
  ok1(RoughTime(0, 0) - RoughTimeDelta::FromMinutes(1) == RoughTime(23, 59));
  ok1(RoughTime(0, 0) - RoughTimeDelta::FromMinutes(60) == RoughTime(23, 0));
  ok1(RoughTime(0, 0) - RoughTimeDelta::FromHours(24) == RoughTime(0, 0));
  ok1(RoughTime(0, 0) - RoughTimeDelta::FromHours(25) == RoughTime(23, 0));
}

int main()
{
  plan_tests(92);

  test_rough_time();
  test_rough_time_span();
  test_rough_time_delta();

  return exit_status();
}
