// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "io/CSVLine.hpp"
#include "TestUtil.hpp"

#include <cstring>
#include <string>

using std::string_view_literals::operator""sv;

static void
Test1()
{
  CSVLine line("1,2,x,4,5,6,7,8,9,10");

  // Test read(int)
  ok1(line.Read(-1) == 1);

  // Test rest()
  ok1(line.Rest() == "2,x,4,5,6,7,8,9,10"sv);

  // Test ReadView()
  ok1(line.ReadView() == "2"sv);

  // Test skip(int)
  line.Skip(3);
  ok1(line.Read(-1) == 6);

  // Test read_first_char()
  ok1(line.ReadFirstChar() == '7');

  // Test read(char)
  ok1(line.ReadView() == "8"sv);

  // Test read_compare(char)
  ok1(line.ReadCompare("9"));

  // Test read(long)
  ok1(line.Read(-1l) == 10l);

  // Test default-value at line-end
  ok1(line.Read(11) == 11);

  // Test default-value at line-end
  int temp_int;
  ok1(!line.ReadChecked(temp_int));
}

static void
Test2()
{
  CSVLine line("A0,4.5555,4.5555,42,0,1.337,42.42,42,xxx");

  // Test read_hex()
  ok1(line.ReadHex(-1) == 160);

  // Test read(double)
  ok1(equals(line.Read(0.0), 4.5555));

  // Test read(fixed)
  ok1(equals(line.Read(0.0), 4.5555));

  // Test read(bool)
  ok1(line.Read(false) == true);
  ok1(line.Read(false) == false);

  // Test read_checked()
  double temp_double;
  ok1(line.ReadChecked(temp_double) && equals(temp_double, 1.337));

  double temp_fixed;
  ok1(line.ReadChecked(temp_fixed) && equals(temp_fixed, 42.42));

  int temp_int;
  ok1(line.ReadChecked(temp_int) && temp_int == 42);
  ok1(!line.ReadChecked(temp_int) && temp_int == 42);
}

int
main()
{
  plan_tests(19);

  Test1();
  Test2();

  return exit_status();
}
