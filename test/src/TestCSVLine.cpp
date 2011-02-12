/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

int main(int argc, char **argv)
{
  plan_tests(19);

  CSVLine line1("1,2,x,4,5,6,7,8,9,10");

  // Test read(long)
  int number_one = line1.read(-1);
  ok1(number_one == 1);

  // Test rest()
  ok1(strcmp(line1.rest(), "2,x,4,5,6,7,8,9,10") == 0);

  // Test skip()
  ok1(line1.skip() == 1);

  // Test skip(int)
  line1.skip(3);
  ok1(line1.read(-1) == 6);

  // Test read_first_char()
  ok1(line1.read_first_char() == '7');

  // Test read(char)
  char number_eight[10];
  line1.read(number_eight, 10);
  ok1(strcmp(number_eight, "8") == 0);

  // Test read_compare(char)
  ok1(line1.read_compare("9"));

  // Test read(long)
  long number_ten = line1.read(-1);
  ok1(number_ten == 10);

  // Test default-value at line-end
  ok1(line1.read(11) == 11);

  // Test default-value at line-end
  ok1(!line1.read_checked(number_one));



  CSVLine line2("A0,4.5555,4.5555,33,0,1.337,42.42,42,xxx");

  // Test read_hex()
  ok1(line2.read_hex(-1) == 160);

  // Test read(double)
  ok1(equals(fixed(line2.read(0.0)), 4.5555));

  // Test read(fixed)
  ok1(equals(line2.read(fixed_zero), 4.5555));

  // Test read(bool)
  ok1(line2.read(false) == true);
  ok1(line2.read(false) == false);

  // Test read_checked()
  double test1;
  ok1(line2.read_checked(test1) && equals(fixed(test1), 1.337));
  fixed test2;
  ok1(line2.read_checked(test2) && equals(test2, 42.42));
  int test3;
  ok1(line2.read_checked(test3) && test3 == 42);
  ok1(!line2.read_checked(test3) && test3 == 42);

  return exit_status();
}
