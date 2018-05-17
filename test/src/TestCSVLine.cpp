/* Copyright_License {

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

#include "IO/CSVLine.hpp"
#include "TestUtil.hpp"

#include <cstring>
#include <string>

static void
Test1()
{
  CSVLine line("1,2,x,4,5,6,7,8,9,10");

  // Test read(int)
  ok1(line.Read(-1) == 1);

  // Test rest()
  const auto rest = line.Rest();
  ok1(std::string(rest.begin(), rest.end()) == "2,x,4,5,6,7,8,9,10");

  // Test skip()
  ok1(line.Skip() == 1);

  // Test skip(int)
  line.Skip(3);
  ok1(line.Read(-1) == 6);

  // Test read_first_char()
  ok1(line.ReadFirstChar() == '7');

  // Test read(char)
  char temp[10];
  line.Read(temp, 10);
  ok1(strcmp(temp, "8") == 0);

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
main(int argc, char **argv)
{
  plan_tests(19);

  Test1();
  Test2();

  return exit_status();
}
