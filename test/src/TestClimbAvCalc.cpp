// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/ClimbAverageCalculator.hpp"
#include "FLARM/Calculations.hpp"
#include "FLARM/List.hpp"
#include "TestUtil.hpp"

#include <cstdio>

static void
TestBasic()
{
  ClimbAverageCalculator c;
  c.Reset();

  double av = 0;

  constexpr FloatDuration AVERAGE_TIME = std::chrono::seconds{30};

  // Test normal behavior
  c.GetAverage(TimeStamp{}, 0, AVERAGE_TIME);
  for (unsigned i = 1; i <= 15; i++)
    av = c.GetAverage(TimeStamp{std::chrono::seconds{i}}, i, AVERAGE_TIME);

  ok1(equals(av, 1.0));

  for (unsigned i = 1; i <= 15; i++)
    av = c.GetAverage(TimeStamp{std::chrono::seconds{15 + i}}, 15 + i * 2, AVERAGE_TIME);

  ok1(equals(av, 1.5));

  for (unsigned i = 1; i <= 15; i++)
    av = c.GetAverage(TimeStamp{std::chrono::seconds{30 + i}}, 45 + i * 2, AVERAGE_TIME);

  ok1(equals(av, 2.0));
}

static void
TestDuplicateTimestamps()
{
  ClimbAverageCalculator c;
  double av = 0;

  constexpr FloatDuration AVERAGE_TIME = std::chrono::seconds{30};

  // Test time difference = zero behavior
  c.Reset();
  c.GetAverage(TimeStamp{}, 0, AVERAGE_TIME);
  for (unsigned i = 1; i <= 15; i++)
    c.GetAverage(TimeStamp{std::chrono::seconds{i}}, i, AVERAGE_TIME);

  for (unsigned i = 1; i <= 15; i++) {
    c.GetAverage(TimeStamp{std::chrono::seconds{15 + i}}, 15 + i * 2, AVERAGE_TIME);
    c.GetAverage(TimeStamp{std::chrono::seconds{15 + i}}, 15 + i * 2, AVERAGE_TIME);
    av = c.GetAverage(TimeStamp{std::chrono::seconds{15 + i}}, 15 + i * 2, AVERAGE_TIME);
  }

  ok1(equals(av, 1.5));
}

static void
TestExpiration()
{
  ClimbAverageCalculator c;
  c.Reset();

  constexpr FloatDuration AVERAGE_TIME = std::chrono::seconds{30};

  // Test expiration for empty data
  ok1(c.Expired(TimeStamp{}, std::chrono::minutes{1}));
  ok1(c.Expired(TimeStamp{std::chrono::seconds{15}}, std::chrono::minutes{1}));

  // Add values and test non-expiration
  bool expired = false;
  for (unsigned i = 1; i <= 60; i++) {
    c.GetAverage(TimeStamp{std::chrono::seconds{i}}, i, AVERAGE_TIME);
    expired = expired || c.Expired(TimeStamp{std::chrono::seconds{i}}, std::chrono::minutes{1});
  }

  ok1(!expired);

  // Test expiration with 30sec
  ok1(!c.Expired(TimeStamp{std::chrono::seconds{89}}, std::chrono::seconds{30}));
  ok1(!c.Expired(TimeStamp{std::chrono::seconds{90}}, std::chrono::seconds{30}));
  ok1(c.Expired(TimeStamp{std::chrono::seconds{91}}, std::chrono::seconds{30}));

  // Test expiration with 60sec
  ok1(!c.Expired(TimeStamp{std::chrono::seconds{119}}, std::chrono::minutes{1}));
  ok1(!c.Expired(TimeStamp{std::chrono::seconds{120}}, std::chrono::minutes{1}));
  ok1(c.Expired(TimeStamp{std::chrono::seconds{121}}, std::chrono::minutes{1}));

  // Time warp
  ok1(c.Expired(TimeStamp{std::chrono::seconds{59}}, std::chrono::minutes{1}));
  ok1(!c.Expired(TimeStamp{std::chrono::seconds{60}}, std::chrono::minutes{1}));
  ok1(!c.Expired(TimeStamp{std::chrono::seconds{61}}, std::chrono::minutes{1}));
}

static void
TestWindowReadiness()
{
  ClimbAverageCalculator c;
  c.Reset();
  constexpr FloatDuration AVERAGE_TIME = std::chrono::seconds{30};

  auto result = c.GetAverageWithSpan(TimeStamp{std::chrono::seconds{1}},
                                     100, AVERAGE_TIME);
  ok1(result.time_span == FloatDuration::zero());
  ok1(!result.IsComplete(AVERAGE_TIME));

  result = c.GetAverageWithSpan(TimeStamp{std::chrono::seconds{30}},
                                129, AVERAGE_TIME);
  ok1(result.time_span == std::chrono::seconds{29});
  ok1(!result.IsComplete(AVERAGE_TIME));

  result = c.GetAverageWithSpan(TimeStamp{std::chrono::seconds{31}},
                                130, AVERAGE_TIME);
  ok1(result.time_span == AVERAGE_TIME);
  ok1(result.IsComplete(AVERAGE_TIME));
  ok1(equals(result.average, 1));

  result = c.GetAverageWithSpan(TimeStamp{std::chrono::seconds{31}},
                                160, AVERAGE_TIME);
  ok1(result.IsComplete(AVERAGE_TIME));
  ok1(equals(result.average, 2));

  result = c.GetAverageWithSpan(TimeStamp{std::chrono::seconds{10}},
                                10, AVERAGE_TIME);
  ok1(result.time_span == FloatDuration::zero());
  ok1(!result.IsComplete(AVERAGE_TIME));
}

static void
TestHighRateSamples()
{
  ClimbAverageCalculator c;
  c.Reset();
  constexpr FloatDuration AVERAGE_TIME = std::chrono::seconds{30};

  ClimbAverageResult result{};
  for (unsigned i = 0; i <= 600; ++i) {
    const FloatDuration elapsed{i / 20.};
    result = c.GetAverageWithSpan(TimeStamp{std::chrono::seconds{1} + elapsed},
                                  elapsed.count() * 2, AVERAGE_TIME,
                                  FlarmCalculations::SAMPLE_POLICY);
  }

  ok1(result.IsComplete(AVERAGE_TIME));
  ok1(equals(result.time_span.count(), 30));
  ok1(equals(result.average, 2));
}

static void
TestSamplingPolicy()
{
  ClimbAverageCalculator c;
  c.Reset();

  const auto policy = FlarmCalculations::SAMPLE_POLICY;
  const auto window = FlarmCalculations::AVERAGE_TIME;
  const TimeStamp start{std::chrono::seconds{1}};

  auto result = c.GetAverageWithSpan(start, 100, window, policy);
  ok1(result.sample_action == ClimbSampleAction::APPENDED);
  ok1(!result.reset);

  result = c.GetAverageWithSpan(start, 101, window, policy);
  ok1(result.sample_action == ClimbSampleAction::REPLACED);

  result = c.GetAverageWithSpan(
    start + FloatDuration{0.249}, 102, window, policy);
  ok1(result.sample_action == ClimbSampleAction::IGNORED);

  result = c.GetAverageWithSpan(
    start + policy.minimum_interval, 103, window, policy);
  ok1(result.sample_action == ClimbSampleAction::APPENDED);

  result = c.GetAverageWithSpan(
    start + policy.minimum_interval + policy.maximum_gap,
    104, window, policy);
  ok1(result.sample_action == ClimbSampleAction::APPENDED);
  ok1(!result.reset);

  result = c.GetAverageWithSpan(
    start + policy.minimum_interval + policy.maximum_gap +
      FloatDuration{0.001},
    105, window, policy);
  ok1(result.sample_action == ClimbSampleAction::IGNORED);
  ok1(!result.reset);

  result = c.GetAverageWithSpan(
    start + policy.minimum_interval + policy.maximum_gap * 2 +
      FloatDuration{0.002},
    106, window, policy);
  ok1(result.sample_action == ClimbSampleAction::APPENDED);
  ok1(result.reset);
  ok1(result.time_span == FloatDuration::zero());

  result = c.GetAverageWithSpan(start, 107, window, policy);
  ok1(result.sample_action == ClimbSampleAction::APPENDED);
  ok1(result.reset);
  ok1(result.time_span == FloatDuration::zero());

  result = c.GetAverageWithSpan(TimeStamp::Undefined(), 108,
                                window, policy);
  ok1(result.sample_action == ClimbSampleAction::IGNORED);
  ok1(result.reset);
}

static void
TestFlarmDiscontinuities()
{
  FlarmCalculations calculations;
  const auto id = FlarmId::FromValue(123);

  FlarmCalculations::AverageResult result{};
  for (unsigned i = 1; i <= 31; ++i)
    result = calculations.Average30sWithSpan(
      id, TimeStamp{std::chrono::seconds{i}}, 100 + i);

  ok1(result.IsComplete(std::chrono::seconds{30}));
  ok1(equals(result.average, 1));

  result = calculations.Average30sWithSpan(
    id, TimeStamp{std::chrono::seconds{37}}, 137);
  ok1(!result.IsComplete(std::chrono::seconds{30}));

  result = calculations.Average30sWithSpan(
    id, TimeStamp{std::chrono::seconds{20}}, 120);
  ok1(result.time_span == FloatDuration::zero());

  TrafficList empty{};
  empty.Clear();
  calculations.ResetMissing(empty);
  result = calculations.Average30sWithSpan(
    id, TimeStamp{std::chrono::seconds{21}}, 121);
  ok1(result.time_span == FloatDuration::zero());
  ok1(!result.IsComplete(std::chrono::seconds{30}));
}

int main()
{
  plan_tests(53);

  TestBasic();
  TestDuplicateTimestamps();
  TestExpiration();
  TestWindowReadiness();
  TestHighRateSamples();
  TestSamplingPolicy();
  TestFlarmDiscontinuities();

  return exit_status();
}
