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

#include "Math/LeastSquares.hpp"
#include "TestUtil.hpp"

#include <stdio.h>
#include <math.h>

static bool
LSTest1(double w)
{
  LeastSquares ls;
  ErrorEllipse ellipse;

  ls.Reset();

  for (double x=0; x<=1; x+= 0.1) {
    ls.Update(x,x, w);
  }
  printf("%g %g %g %g %g\n",
         ls.GetMeanX(), ls.GetMeanY(),
         ls.GetVarX(), ls.GetVarY(), ls.GetCovXY());
  ellipse = ls.GetErrorEllipse();

  ls.Reset();

  for (double x=-0.5; x<=0.5; x+= 0.1) {
    ls.Update(x,x, w);
  }
  printf("%g %g %g %g %g\n",
         ls.GetMeanX(), ls.GetMeanY(),
         ls.GetVarX(), ls.GetVarY(), ls.GetCovXY());
  ellipse = ls.GetErrorEllipse();

  ls.Reset();
  for (double x=-0.5; x<=0.5; x+= 0.1) {
    ls.Update(x,-x, w);
  }
  printf("%g %g %g %g %g\n",
         ls.GetMeanX(), ls.GetMeanY(),
         ls.GetVarX(), ls.GetVarY(), ls.GetCovXY());
  ellipse = ls.GetErrorEllipse();

  ls.Reset();
  for (double x=0; x<=1; x+= 0.1) {
    ls.Update(x,-x, w);
  }
  printf("%g %g %g %g %g\n",
         ls.GetMeanX(), ls.GetMeanY(),
         ls.GetVarX(), ls.GetVarY(), ls.GetCovXY());
  ellipse = ls.GetErrorEllipse();

  ls.Reset();
  for (double x=0; x<=1; x+= 0.1) {
    ls.Update(x,x*2, w);
  }
  printf("%g %g %g %g %g\n",
         ls.GetMeanX(), ls.GetMeanY(),
         ls.GetVarX(), ls.GetVarY(), ls.GetCovXY());
  ellipse = ls.GetErrorEllipse();

  return true;
}

int main(int argc, char **argv)
{
  plan_tests(2);

  ok1(LSTest1(1));
  ok1(LSTest1(2));

  return exit_status();
}
