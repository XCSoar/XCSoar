/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Math/FastMath.h"
#include "TestUtil.hpp"

#include <stdio.h>
#include <stdlib.h>

static 
void test_normalise_err(const int x, const int y) 
{
  int x0=x;
  int y0=y;
  i_normalise(x0, y0);
  int mag0 = iround((fixed)sqrt(x0*x0+y0*y0));
  int error0 = abs((1<<NORMALISE_BITS)-mag0);

  int x1=x;
  int y1=y;
  i_normalise_fast(x1, y1);
  int mag1 = ihypot(x1, y1);
  int error1 = abs((1<<NORMALISE_BITS)-mag1);

  bool err_ok = error1<= error0+1;
  if (!err_ok) {
    printf("# %d %d %d %d %d %d %d %d %d %d\n", 
           x, y, x0, y0, mag0, error0, x1, y1, mag1, error1);
  }

  char label[80];
  sprintf(label,"fast integer normalise error x=%d y=%d", x, y);
  ok(err_ok, label, 0);
}

int slow_norm3(const int k, const int x, const int y, const int z);
 
int slow_norm3(const int k, const int x, const int y, const int z) {
  const long mag = (long)x*x+(long)y*y+(long)x*x;
  return (1<<NORMALISE_BITS)*k/isqrt4(mag);
}

int main(int argc, char** argv) {

  plan_tests(4);

  test_normalise_err(100, 50);
  test_normalise_err(-100, 50);
  test_normalise_err(100, -50);
  test_normalise_err(-100, -50);

  return exit_status();
}

