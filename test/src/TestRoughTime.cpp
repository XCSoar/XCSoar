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
  ok1(TimeType::FromMinuteOfDayChecked((int)(12*60+1))       == TimeType(12,1) );
  ok1(TimeType::FromMinuteOfDayChecked((int)(12*60+1+24*60)) == TimeType(12,1) );
  ok1(TimeType::FromMinuteOfDayChecked((int)(12*60+1-24*60)) == TimeType(12,1) );

  ok1(TimeType( TimeStamp(12h) )          == TimeType(12,0) );
  ok1(TimeType( TimeStamp(12h+1min) )     == TimeType(12,1) );
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

static void test_conversions()
{
  ok1( ToFineTime( RoughTime::Invalid() ) == FineTime::Invalid() );
  ok1( ToFineTime( RoughTime(12,1) )      == FineTime(12,1) );

  ok1( ToRoughTime( FineTime::Invalid() ) == RoughTime::Invalid() );
  ok1( ToRoughTime( FineTime(12,1) )      == RoughTime(12,1) );
}

static void test_time_span()
{
  RoughTime r_a(12, 1);
  RoughTime r_b(11, 59);

  TimeSpan s = TimeSpan::FromRoughTimes(r_a,r_b);
  ok1( s.GetRoughStart() == r_a );
  ok1( s.GetRoughEnd() == r_b );

  s = TimeSpan(ToFineTime(r_a), ToFineTime(r_b));
  ok1( s.GetRoughStart() == r_a );
  ok1( s.GetRoughEnd() == r_b );

  /* test TimeSpan::IsInside() */
  FineTime a(12, 1);
  FineTime b(11, 59);

  s = TimeSpan::Invalid();
  ok1(!s.IsDefined());
  ok1(s.IsInside(a));
  ok1(s.IsInside(b));

  s = TimeSpan(FineTime(12, 0), FineTime::Invalid());
  ok1(s.IsDefined());
  ok1(s.IsInside(a));
  ok1(!s.IsInside(b));

  s = TimeSpan(FineTime::Invalid(), FineTime(12, 0));
  ok1(s.IsDefined());
  ok1(!s.IsInside(a));
  ok1(s.IsInside(b));

  s = TimeSpan(FineTime(12, 0), FineTime(12, 1));
  ok1(s.IsDefined());
  ok1(!s.IsInside(a));
  ok1(!s.IsInside(b));

  s = TimeSpan(FineTime(12, 0), FineTime(12, 30));
  ok1(s.IsDefined());
  ok1(s.IsInside(a));
  ok1(!s.IsInside(b));

  /* test midnight wraparound */
  a = FineTime(0, 0);
  b = FineTime(23, 59);
  FineTime c(22, 0);
  FineTime d(2, 0);
  s = TimeSpan(FineTime(23, 0), FineTime::Invalid());
  ok1(s.IsDefined());
  ok1(s.IsInside(a));
  ok1(s.IsInside(b));
  ok1(!s.IsInside(c));
  ok1(s.IsInside(d));

  s = TimeSpan(FineTime::Invalid(), FineTime(1, 0));
  ok1(s.IsDefined());
  ok1(s.IsInside(a));
  ok1(s.IsInside(b));
  ok1(s.IsInside(c));
  ok1(!s.IsInside(d));

  s = TimeSpan(FineTime(23, 1), FineTime(0, 30));
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
  plan_tests(136);

  test_time<RoughTime>();
  test_time<FineTime>();
  test_conversions();
  test_time_span();
  test_rough_time_delta();

  return exit_status();
}
