// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "time/RoughTime.hpp"
#include "TestUtil.hpp"

using namespace std::chrono_literals;

template <typename TimeType>
static void test_time()
{
  TimeType a = TimeType::Invalid();
  ok1(!a.IsValid());

  a = TimeType(12, 1);
  ok1(a.IsValid());
  ok1(a.GetHour() == 12);
  ok1(a.GetMinute() == 1);
  ok1(a.GetMinuteOfDay() == 12 * 60 + 1);

  /* compare with itself */
  ok1(a == a);
  ok1(!(a != a));
  ok1(a <= a);
  ok1(a >= a);
  ok1(!(a < a));
  ok1(!(a > a));

  /* compare with another one */
  TimeType b(11, 59);
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
  ok1(TimeType::FromMinuteOfDayChecked((int)(12*60+1)) == TimeType(12,1) );
  ok1(TimeType::FromMinuteOfDayChecked((int)(12*60+1+24*60)) == TimeType(12,1) );
  ok1(TimeType::FromMinuteOfDayChecked((int)(12*60+1-24*60)) == TimeType(12,1) );

  ok1(TimeType( TimeStamp(12h) ) == TimeType(12,0) );
  ok1(TimeType( TimeStamp(12h+1min) ) == TimeType(12,1) );
  ok1(TimeType( TimeStamp(12h+1s) ) == TimeType(12,0) );
  ok1(TimeType( TimeStamp(12h+1min+24h) ) == TimeType(12,1) );
  ok1(TimeType( TimeStamp(12h+1min-24h) ) == TimeType(12,1) );

  /* test midnight wraparound */
  a = TimeType(0, 0);
  b = TimeType(23, 59);
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
  plan_tests(85);

  test_time<RoughTime>();
  test_rough_time_span();
  test_rough_time_delta();

  return exit_status();
}
