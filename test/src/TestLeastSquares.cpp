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

static void
ellipse(const LeastSquares& ls)
{
  /*
    A = a b = cov(x,x)   cov(x,y)
    c d   cov(x,y)   cov(y,y)

    T = trace = a + d     = cov(x,x) + cov(y,y)
    D = det = a*d - b*c   = cov(x,x) * cov(y,y) - cov(x,y)**2

    eigenvalues are L1,2 = T/2 +/-  sqrt(T^2/4 -D)
    if c is not zero, the eigenvectors are:
    L1-d    L2-d
    c       c
    else if b is not zero then the eigenvectors are:
    b       b
    L1-a    L2-a

    else the eigenvectors are
    1       0
    0       1

    http://www.visiondummy.com/wp-content/uploads/2014/04/error_ellipse.cpp

  */
  double a = ls.GetVarX();
  double b = ls.GetCovXY();
  double c = b;
  double d = ls.GetVarY();

  double T = a + d;
  double D = a*d - b*c;
  double La = T/2;
  double Lb = sqrt(T*T/4-D);

  double L1 = La+Lb;
  double L2 = La-Lb;
  printf("eigenvalues %g %g\n", L1, L2);

  double v1x, v1y, v2x, v2y;
  if (b == 0) {
    v1x = 1;
    v1y = 0;
    v2x = 0;
    v2y = 1;
  } else {
    v1x = L1- ls.GetVarY();
    v1y = c;
    v2x = L2- ls.GetVarY();
    v2y = c;
  }
  printf("eigenvectors %g %g %g %g\n", v1x, v1y, v2x, v2y);


  double angle = atan2(v1x, v2x);
  if (angle<0) {
    angle += M_2PI;
  }
  double chisquare_val = 2.4477;
  double halfmajor = chisquare_val*sqrt(L1);
  double halfminor = chisquare_val*sqrt(L2);
  printf("ellipse %g %g %g\n", angle, halfmajor, halfminor);
}

static bool
LSTest1(double w)
{
  LeastSquares ls;

  ls.Reset();

  for (double x=0; x<=1; x+= 0.1) {
    ls.Update(x,x, w);
  }
  printf("%g %g %g %g %g\n",
         ls.GetMeanX(), ls.GetMeanY(),
         ls.GetVarX(), ls.GetVarY(), ls.GetCovXY());
  ellipse(ls);

  ls.Reset();

  for (double x=-0.5; x<=0.5; x+= 0.1) {
    ls.Update(x,x, w);
  }
  printf("%g %g %g %g %g\n",
         ls.GetMeanX(), ls.GetMeanY(),
         ls.GetVarX(), ls.GetVarY(), ls.GetCovXY());
  ellipse(ls);

  ls.Reset();
  for (double x=-0.5; x<=0.5; x+= 0.1) {
    ls.Update(x,-x, w);
  }
  printf("%g %g %g %g %g\n",
         ls.GetMeanX(), ls.GetMeanY(),
         ls.GetVarX(), ls.GetVarY(), ls.GetCovXY());
  ellipse(ls);

  ls.Reset();
  for (double x=0; x<=1; x+= 0.1) {
    ls.Update(x,-x, w);
  }
  printf("%g %g %g %g %g\n",
         ls.GetMeanX(), ls.GetMeanY(),
         ls.GetVarX(), ls.GetVarY(), ls.GetCovXY());
  ellipse(ls);

  ls.Reset();
  for (double x=0; x<=1; x+= 0.1) {
    ls.Update(x,x*2, w);
  }
  printf("%g %g %g %g %g\n",
         ls.GetMeanX(), ls.GetMeanY(),
         ls.GetVarX(), ls.GetVarY(), ls.GetCovXY());
  ellipse(ls);

  return true;
}

int main(int argc, char **argv)
{
  plan_tests(2);

  ok1(LSTest1(1));
  ok1(LSTest1(2));

  return exit_status();
}
