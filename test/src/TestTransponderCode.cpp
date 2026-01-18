// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TransponderCode.hpp"
#include "TestUtil.hpp"
#include "util/StringAPI.hxx"

#include <stdio.h>

using namespace std::chrono;

int main()
{
  plan_tests(13);

  ok1(TransponderCode::Parse(std::string_view{"7000"}).IsDefined());
  ok1(TransponderCode{07000}.IsDefined());
  ok1(equals(TransponderCode().GetCode(), 0));
  ok1(!TransponderCode::Parse(std::string_view{"7797"}).IsDefined());
  ok1(!TransponderCode{7797}.IsDefined());
  ok1(!TransponderCode::Null().IsDefined());

  ok1(!TransponderCode::Parse(std::string_view{"20000"}).IsDefined());
  ok1(!TransponderCode{20000}.IsDefined());

  TransponderCode code{04567};
  ok1(code.IsDefined());

  ok1(code.Format() == "4567");

  code.Clear();
  ok1(!code.IsDefined());
  ok1(code.Format().empty());

  code = TransponderCode::Parse(std::string_view{"4567"});
  ok1(code.IsDefined());

  return exit_status();
}
