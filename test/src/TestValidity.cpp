// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/Validity.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

static Validity
invalid()
{
  Validity v;
  v.Clear();
  return v;
}

int main()
{
  plan_tests(11);

  Validity v;
  v.Clear();
  ok1(!v.IsValid());
  v.Update(TimeStamp{std::chrono::seconds{100}});
  ok1(v.IsValid());
  v.Expire(TimeStamp{std::chrono::seconds{101}}, std::chrono::seconds(5));
  ok1(v.IsValid());
  v.Expire(TimeStamp{std::chrono::seconds{105}}, std::chrono::seconds(5));
  ok1(v.IsValid());
  v.Expire(TimeStamp{std::chrono::seconds{106}}, std::chrono::seconds(5));
  ok1(!v.IsValid());

  v.Update(TimeStamp{std::chrono::seconds{100}});
  ok1(v.Modified(Validity(TimeStamp{std::chrono::seconds{99}})));
  ok1(!v.Modified(Validity(TimeStamp{std::chrono::seconds{100}})));
  ok1(!v.Modified(Validity(TimeStamp{std::chrono::seconds{101}})));
  ok1(!v.Complement(Validity(TimeStamp{std::chrono::seconds{1}})));

  v.Clear();
  ok1(!v.Complement(invalid()));
  ok1(v.Complement(Validity(TimeStamp{std::chrono::seconds{1}})));

  return exit_status();
}
