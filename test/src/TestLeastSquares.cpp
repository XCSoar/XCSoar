// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

int main()
{
  plan_tests(2);

  ok1(LSTest1(1));
  ok1(LSTest1(2));

  return exit_status();
}
