/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

int
main(int argc, char **argv)
{
  plan_tests(19);

  {
    CSVLine line("1,2,x,4,5,6,7,8,9,10");

    {
      // Test read(long)
      int temp = line.read(-1);
      ok1(temp == 1);
    }

    // Test rest()
    ok1(strcmp(line.rest(), "2,x,4,5,6,7,8,9,10") == 0);

    // Test skip()
    ok1(line.skip() == 1);

    // Test skip(int)
    line.skip(3);
    ok1(line.read(-1) == 6);

    // Test read_first_char()
    ok1(line.read_first_char() == '7');

    {
      // Test read(char)
      char temp[10];
      line.read(temp, 10);
      ok1(strcmp(temp, "8") == 0);
    }

    // Test read_compare(char)
    ok1(line.read_compare("9"));

    {
      // Test read(long)
      long temp = line.read(-1);
      ok1(temp == 10);
    }

    // Test default-value at line-end
    ok1(line.read(11) == 11);

    {
      // Test default-value at line-end
      int temp;
      ok1(!line.read_checked(temp));
    }
  }

  {
    CSVLine line("A0,4.5555,4.5555,42,0,1.337,42.42,42,xxx");

    // Test read_hex()
    ok1(line.read_hex(-1) == 160);

    // Test read(double)
    ok1(equals(fixed(line.read(0.0)), 4.5555));

    // Test read(fixed)
    ok1(equals(line.read(fixed_zero), 4.5555));

    // Test read(bool)
    ok1(line.read(false) == true);
    ok1(line.read(false) == false);

    // Test read_checked()
    {
      double temp;
      ok1(line.read_checked(temp) && equals(fixed(temp), 1.337));
    }
    {
      fixed temp;
      ok1(line.read_checked(temp) && equals(temp, 42.42));
    }
    {
      int temp;
      ok1(line.read_checked(temp) && temp == 42);
      ok1(!line.read_checked(temp) && temp == 42);
    }
  }

  return exit_status();
}
