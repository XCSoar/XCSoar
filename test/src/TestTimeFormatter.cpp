// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Formatter/TimeFormatter.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"
#include "TestUtil.hpp"

static void
TestFormat()
{
  char buffer[256];

  FormatTime(buffer, FloatDuration{});
  ok1(StringIsEqual(buffer, "00:00:00"));

  FormatTime(buffer, std::chrono::seconds{1});
  ok1(StringIsEqual(buffer, "00:00:01"));

  FormatTime(buffer, std::chrono::seconds{59});
  ok1(StringIsEqual(buffer, "00:00:59"));

  FormatTime(buffer, std::chrono::seconds{60});
  ok1(StringIsEqual(buffer, "00:01:00"));

  FormatTime(buffer, std::chrono::seconds{60 * 5});
  ok1(StringIsEqual(buffer, "00:05:00"));

  FormatTime(buffer, std::chrono::seconds{60 * 59});
  ok1(StringIsEqual(buffer, "00:59:00"));

  FormatTime(buffer, std::chrono::seconds{60 * 60});
  ok1(StringIsEqual(buffer, "01:00:00"));

  FormatTime(buffer, std::chrono::seconds{60 * 60 * 3 + 60 * 25});
  ok1(StringIsEqual(buffer, "03:25:00"));

  FormatTime(buffer, std::chrono::seconds{60 * 60 * 19 + 60 * 47 + 43});
  ok1(StringIsEqual(buffer, "19:47:43"));

  FormatTime(buffer, std::chrono::seconds{-(60 * 59)});
  ok1(StringIsEqual(buffer, "-00:59:00"));

  FormatTime(buffer, std::chrono::seconds{-(60 * 60 * 19 + 60 * 47 + 43)});
  ok1(StringIsEqual(buffer, "-19:47:43"));
}

static void
TestFormatLong()
{
  char buffer[256];

  FormatTimeLong(buffer, {});
  ok1(StringIsEqual(buffer, "00:00:00.000"));

  FormatTimeLong(buffer, FloatDuration{1.123});
  ok1(StringIsEqual(buffer, "00:00:01.123"));

  FormatTimeLong(buffer, std::chrono::seconds{59});
  ok1(StringIsEqual(buffer, "00:00:59.000"));

  FormatTimeLong(buffer, FloatDuration{60.001});
  ok1(StringIsEqual(buffer, "00:01:00.001"));

  FormatTimeLong(buffer, std::chrono::seconds{60 * 5});
  ok1(StringIsEqual(buffer, "00:05:00.000"));

  FormatTimeLong(buffer, std::chrono::seconds{60 * 59});
  ok1(StringIsEqual(buffer, "00:59:00.000"));

  FormatTimeLong(buffer, std::chrono::seconds{60 * 60});
  ok1(StringIsEqual(buffer, "01:00:00.000"));

  FormatTimeLong(buffer, std::chrono::seconds{60 * 60 * 3 + 60 * 25});
  ok1(StringIsEqual(buffer, "03:25:00.000"));

  FormatTimeLong(buffer, FloatDuration{60 * 60 * 19 + 60 * 47 + 43.765});
  ok1(StringIsEqual(buffer, "19:47:43.765"));

  FormatTimeLong(buffer, std::chrono::seconds{-(60 * 59)});
  ok1(StringIsEqual(buffer, "-00:59:00.000"));

  FormatTimeLong(buffer, FloatDuration{-(60 * 60 * 19 + 60 * 47 + 43.765)});
  ok1(StringIsEqual(buffer, "-19:47:43.765"));
}

static void
TestHHMM()
{
  char buffer[256];

  FormatSignedTimeHHMM(buffer, {});
  ok1(StringIsEqual(buffer, "00:00"));

  FormatSignedTimeHHMM(buffer, std::chrono::seconds{1});
  ok1(StringIsEqual(buffer, "00:00"));

  FormatSignedTimeHHMM(buffer, std::chrono::seconds{59});
  ok1(StringIsEqual(buffer, "00:00"));

  FormatSignedTimeHHMM(buffer, std::chrono::seconds{60});
  ok1(StringIsEqual(buffer, "00:01"));

  FormatSignedTimeHHMM(buffer, std::chrono::seconds{60 * 5});
  ok1(StringIsEqual(buffer, "00:05"));

  FormatSignedTimeHHMM(buffer, std::chrono::seconds{60 * 59});
  ok1(StringIsEqual(buffer, "00:59"));

  FormatSignedTimeHHMM(buffer, std::chrono::seconds{60 * 60});
  ok1(StringIsEqual(buffer, "01:00"));

  FormatSignedTimeHHMM(buffer, std::chrono::seconds{60 * 60 * 3 + 60 * 25});
  ok1(StringIsEqual(buffer, "03:25"));

  FormatSignedTimeHHMM(buffer, std::chrono::seconds{60 * 60 * 19 + 60 * 47});
  ok1(StringIsEqual(buffer, "19:47"));

  FormatSignedTimeHHMM(buffer, std::chrono::seconds{-(60 * 59)});
  ok1(StringIsEqual(buffer, "-00:59"));

  FormatSignedTimeHHMM(buffer, std::chrono::seconds{-(60 * 60 * 19 + 60 * 47)});
  ok1(StringIsEqual(buffer, "-19:47"));
}

#include <stdio.h>

static void
TestTwoLines()
{
  char buffer[256], buffer2[256];

  FormatTimeTwoLines(buffer, buffer2, {});
  ok1(StringIsEqual(buffer, "00'00"));
  ok1(StringIsEqual(buffer2, ""));

  FormatTimeTwoLines(buffer, buffer2, std::chrono::seconds{1});
  ok1(StringIsEqual(buffer, "00'01"));
  ok1(StringIsEqual(buffer2, ""));

  FormatTimeTwoLines(buffer, buffer2, std::chrono::seconds{59});
  ok1(StringIsEqual(buffer, "00'59"));
  ok1(StringIsEqual(buffer2, ""));

  FormatTimeTwoLines(buffer, buffer2, std::chrono::seconds{60});
  ok1(StringIsEqual(buffer, "01'00"));
  ok1(StringIsEqual(buffer2, ""));

  FormatTimeTwoLines(buffer, buffer2, std::chrono::seconds{60 * 5});
  ok1(StringIsEqual(buffer, "05'00"));
  ok1(StringIsEqual(buffer2, ""));

  FormatTimeTwoLines(buffer, buffer2, std::chrono::seconds{60 * 59});
  ok1(StringIsEqual(buffer, "59'00"));
  ok1(StringIsEqual(buffer2, ""));

  FormatTimeTwoLines(buffer, buffer2, std::chrono::seconds{60 * 60});
  ok1(StringIsEqual(buffer, "01:00"));
  ok1(StringIsEqual(buffer2, "00"));

  FormatTimeTwoLines(buffer, buffer2, std::chrono::seconds{60 * 60 * 3 + 60 * 25 + 13});
  ok1(StringIsEqual(buffer, "03:25"));
  ok1(StringIsEqual(buffer2, "13"));

  FormatTimeTwoLines(buffer, buffer2, std::chrono::seconds{60 * 60 * 19 + 60 * 47 + 28});
  ok1(StringIsEqual(buffer, "19:47"));
  ok1(StringIsEqual(buffer2, "28"));

  FormatTimeTwoLines(buffer, buffer2, std::chrono::seconds{-(60 * 59)});
  ok1(StringIsEqual(buffer, "-59'00"));
  ok1(StringIsEqual(buffer2, ""));

  FormatTimeTwoLines(buffer, buffer2, std::chrono::seconds{-(60 * 60 * 19 + 60 * 47 + 28)});
  ok1(StringIsEqual(buffer, "-19:47"));
  ok1(StringIsEqual(buffer2, "28"));
}

static void
TestSmart(int _time, const char *expected_output1,
          const char *expected_output2, const char *expected_output3,
          const char *expected_output4, const char *separator = " ")
{
  char buffer[256];

  const auto time = std::chrono::seconds{_time};

  FormatTimespanSmart(buffer, time, 1, separator);
  ok1(StringIsEqual(buffer, expected_output1));

  FormatTimespanSmart(buffer, time, 2, separator);
  ok1(StringIsEqual(buffer, expected_output2));

  FormatTimespanSmart(buffer, time, 3, separator);
  ok1(StringIsEqual(buffer, expected_output3));

  FormatTimespanSmart(buffer, time, 4, separator);
  ok1(StringIsEqual(buffer, expected_output4));
}

static void
TestSmart()
{
  TestSmart(0, "0 sec", "0 sec", "0 sec", "0 sec");
  TestSmart(1, "1 sec", "1 sec", "1 sec", "1 sec");
  TestSmart(59, "59 sec", "59 sec", "59 sec", "59 sec");
  TestSmart(60, "1 min", "1 min", "1 min", "1 min");

  TestSmart(60 + 59, "1 min", "1 min 59 sec", "1 min 59 sec",
            "1 min 59 sec");

  TestSmart(60 * 5 + 34, "5 min", "5 min 34 sec", "5 min 34 sec",
            "5 min 34 sec");

  TestSmart(60 * 59, "59 min", "59 min", "59 min", "59 min");
  TestSmart(60 * 60, "1 h", "1 h", "1 h", "1 h");

  TestSmart(60 * 60 * 3 + 60 * 25, "3 h", "3 h 25 min",
            "3 h 25 min", "3 h 25 min");

  TestSmart(60 * 60 * 19 + 60 * 47, "19 h", "19 h 47 min",
            "19 h 47 min", "19 h 47 min");

  TestSmart(60 * 60 * 19 + 47, "19 h", "19 h",
            "19 h 0 min 47 sec", "19 h 0 min 47 sec");

  TestSmart(60 * 60 * 19 + 60 * 47 + 5, "19 h", "19 h 47 min",
            "19 h 47 min 5 sec", "19 h 47 min 5 sec");

  TestSmart(60 * 60 * 24 * 3 + 60 * 60 * 19 + 60 * 47 + 5, "3 days",
            "3 days 19 h", "3 days 19 h 47 min",
            "3 days 19 h 47 min 5 sec");

  TestSmart(-(60 * 60 * 24 * 3 + 60 * 60 * 19 + 60 * 47 + 5), "-3 days",
            "-3 days 19 h", "-3 days 19 h 47 min",
            "-3 days 19 h 47 min 5 sec");
}

int main()
{
  plan_tests(111);

  TestFormat();
  TestFormatLong();
  TestHHMM();
  TestTwoLines();
  TestSmart();

  return exit_status();
}
