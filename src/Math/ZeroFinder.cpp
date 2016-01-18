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
#include "ZeroFinder.hpp"

#include <limits>

#include <math.h>

/** machine tolerance */
static constexpr double epsilon = std::numeric_limits<double>::epsilon();
/** sqrt of machine tolerance */
static const double sqrt_epsilon = sqrt(epsilon);
static const double r((3. - sqrt(5.0)) / 2); /* Gold section ratio */

static inline void
limit_tolerance(double &f, const double tol_act)
{
  if (fabs(f) < tol_act)
    f = f > 0 ? tol_act : -tol_act;
}

inline double
ZeroFinder::tolerance_actual_min(const double x) const
{
  return sqrt_epsilon * fabs(x) + tolerance / 3;
}

inline double
ZeroFinder::tolerance_actual_zero(const double x) const
{
  return 2 * epsilon * fabs(x) + tolerance / 2;
}

//#define INSTRUMENT_ZERO
#ifdef INSTRUMENT_ZERO
unsigned long zero_skipped = 0;
unsigned long zero_total = 0;
#endif

inline bool
ZeroFinder::solution_within_tolerance(const double x,
                                      const double tol_act)
{

  // are we away from the edges? if so, check improved solution
  const auto x_minus = x-tol_act;
  if (xmin >= x_minus)
    return false;
  const auto x_plus = x+tol_act;
  if (x_plus >= xmax)
    return false;

  const auto fx = f(x);
  if (f(x_plus)<fx)
    return false;
  if (f(x_minus)<fx)
    return false;
  // existing solution is good 
  return true;
}

/*
  Note:
  - When you can, use find_zero search, since it narrows in at rate of 1.6 per step compared
    to 1.3 for the find_min.
*/

/*
 ************************************************************************
 *	    		    C math library
 * function ZEROIN - obtain a function zero within the given range
 *
 * Input
 *	double zeroin(ax,bx,f,tol)
 *	double ax; 			Root will be seeked for within
 *	double bx;  			a range [ax,bx]
 *	double (*f)(double x);		Name of the function whose zero
 *					will be seeked for
 *	double tol;			Acceptable tolerance for the root
 *					value.
 *					May be specified as 0.0 to cause
 *					the program to find the root as
 *					accurate as possible
 *
 * Output
 *	Zeroin returns an estimate for the root with accuracy
 *	4*epsilon*abs(x) + tol
 *
 * Algorithm
 *	G.Forsythe, M.Malcolm, C.Moler, Computer methods for mathematical
 *	computations. M., Mir, 1980, p.180 of the Russian edition
 *
 *	The function makes use of the bissection procedure combined with
 *	the linear or quadric inverse interpolation.
 *	At every step program operates on three abscissae - a, b, and c.
 *	b - the last and the best approximation to the root
 *	a - the last but one approximation
 *	c - the last but one or even earlier approximation than a that
 *		1) |f(b)| <= |f(c)|
 *		2) f(b) and f(c) have opposite signs, i.e. b and c confine
 *		   the root
 *	At every step Zeroin selects one of the two new approximations, the
 *	former being obtained by the bissection procedure and the latter
 *	resulting in the interpolation (if a,b, and c are all different
 *	the quadric interpolation is utilized, otherwise the linear one).
 *	If the latter (i.e. obtained by the interpolation) point is 
 *	reasonable (i.e. lies within the current interval [b,c] not being
 *	too close to the boundaries) it is accepted. The bissection result
 *	is used in the other case. Therefore, the range of uncertainty is
 *	ensured to be reduced at least by the factor 1.6
 *
 ************************************************************************
 */

double
ZeroFinder::find_zero(const double xstart)
{
#ifdef INSTRUMENT_ZERO
  zero_total++;
#endif
  if ((xmin<=xstart) || (xstart<=xmax) ||
      (f(xstart)> sqrt_epsilon))
    return find_zero_actual(xstart);
#ifdef INSTRUMENT_ZERO
  zero_skipped++;
#endif
  return xstart;
}

inline double
ZeroFinder::find_zero_actual(const double xstart)
{
  double a, b, c; // Abscissae, descr. see above
  double fa; // f(a)
  double fb; // f(b)
  double fc; // f(c)

  bool b_best = true; // b is best and last called

  c = a = xmin;  
  fc = fa = f(a);  

  b = xmax;  
  fb = f(b);

  // Main iteration loop
  for (;;) {
    // Distance from the last but one to the last approximation
    auto prev_step = b - a;
   
    if (fabs(fc) < fabs(fb)) {
      // Swap data for b to be the best approximation
      a = b;
      b = c;
      c = a;

      fa = fb;
      fb = fc;
      fc = fa;

      b_best = false;
    } else {
      b_best = true;
    }

    // Actual tolerance
    const auto tol_act = tolerance_actual_zero(b);

    // Step at this iteration
    auto new_step = (c - b) / 2;

    if (fabs(new_step) <= tol_act || fabs(fb) < sqrt_epsilon) {
      if (!b_best)
        // call once more
        f(b);

      // Acceptable approx. is found
      return b;
    }

    // Decide if the interpolation can be tried

    // If prev_step was large enough and was in true direction,
    // interpolation may be tried
    if (fabs(prev_step) >= tol_act && fabs(fa) > fabs(fb)) {
      // Interpolation step is calculated in the form p/q;
      // division operations is delayed until the last moment
      double p;
      double q;

      const auto cb = c - b;
      // If we have only two distinct points
      // -> linear interpolation can only be applied
      if (a == c) {
        const auto t1 = fb / fa;
        p = cb * t1;
        q = 1. - t1;
      } else {
        // Quadric inverse interpolation
        q = fa / fc;
        const auto t1 = fb / fc;
        const auto t2 = fb / fa;
        p = t2 * (cb * q * (q - t1) - (b - a) * (t1 - 1.));
        q = (q - 1.) * (t1 - 1.) * (t2 - 1.);
      }

      // p was calculated with the opposite sign;
      // make p positive and assign possible minus to q
      if (p > 0)
        q = -q;
      else
        p = -p;

      // If b+p/q falls in [b,c] and isn't too large it is accepted
      // If p/q is too large then the bissection procedure can
      // reduce [b,c] range to more extent
      if (p < (0.75 * cb * q - fabs(tol_act * q)/ 2)
          && p < fabs(prev_step * q / 2))
        new_step = p / q;
    }

    // Adjust the step to be not less than tolerance
    limit_tolerance(new_step, tol_act);

    // Save the previous approx.
    a = b;
    fa = fb;

    // Do step to a new approxim.
    b += new_step;
    fb = f(b);

    // Adjust c for it to have a sign opposite to that of b
    if ((fb > 0 && fc > 0) || (fb < 0 && fc < 0)) {
      c = a;
      fc = fa;
    }

    assert(b >= xmin);
    assert(b <= xmax);
  }
}


/*
 ************************************************************************
 *	    		    C math library
 * function FMINBR - one-dimensional search for a function minimum
 *			  over the given range
 *
 * Input
 *	double fminbr(a,b,f,tolerance)
 *	double a; 			Minimum will be seeked for over
 *	double b;  			a range [a,b], a being < b.
 *	double (*f)(double x);		Name of the function whose minimum
 *					will be seeked for
 *	double tolerance;			Acceptable tolerance for the minimum
 *					location. It have to be positive
 *					(e.g. may be specified as epsilon)
 *
 * Output
 *	Fminbr returns an estimate for the minimum location with accuracy
 *	3*sqrt_epsilon*abs(x) + tolerance.
 *	The function always obtains a local minimum which coincides with
 *	the global one only if a function under investigation being
 *	unimodular.
 *	If a function being examined possesses no local minimum within
 *	the given range, Fminbr returns 'a' (if f(a) < f(b)), otherwise
 *	it returns the right range boundary value b.
 *
 * Algorithm
 *	G.Forsythe, M.Malcolm, C.Moler, Computer methods for mathematical
 *	computations. M., Mir, 1980, p.202 of the Russian edition
 *
 *	The function makes use of the "gold section" procedure combined with
 *	the parabolic interpolation.
 *	At every step program operates three abscissae - x,v, and w.
 *	x - the last and the best approximation to the minimum location,
 *	    i.e. f(x) <= f(a) or/and f(x) <= f(b)
 *	    (if the function f has a local minimum in (a,b), then the both
 *	    conditions are fulfiled after one or two steps).
 *	v,w are previous approximations to the minimum location. They may
 *	coincide with a, b, or x (although the algorithm tries to make all
 *	u, v, and w distinct). Points x, v, and w are used to construct
 *	interpolating parabola whose minimum will be treated as a new
 *	approximation to the minimum location if the former falls within
 *	[a,b] and reduces the range enveloping minimum more efficient than
 *	the gold section procedure. 
 *	When f(x) has a second derivative positive at the minimum location
 *	(not coinciding with a or b) the procedure converges superlinearly
 *	at a rate order about 1.324
 *
 ************************************************************************
 */

double
ZeroFinder::find_min(const double xstart)
{
#ifdef INSTRUMENT_ZERO
  zero_total++;
#endif
  if (!solution_within_tolerance(xstart, tolerance_actual_min(xstart)))
    return find_min_actual(xstart);
#ifdef INSTRUMENT_ZERO
  zero_skipped++;
#endif
  return xstart;
}

inline double
ZeroFinder::find_min_actual(const double xstart)
{
  double x, v, w; // Abscissae, descr. see above
  double fx; // f(x)
  double fv; // f(v)
  double fw; // f(w)
  double a = xmin;
  double b = xmax;
  bool x_best = true;

  assert(tolerance > 0 && b > a);

  /* First step - always gold section*/
  x = w = v = a + r * (b - a);
  fx = fw = fv = f(v);

  // Main iteration loop
  for (;;) {
    // Range over which the minimum is seeked for
    const auto range = b - a;
    const auto middle_range = (a + b) / 2;

    // Actual tolerance
    const auto tol_act = tolerance_actual_min(x);
    const auto double_tol_act = 2 * tol_act;

    if (fabs(x-middle_range) + range / 2 <= double_tol_act) {
      if (!x_best)
        // call once more
        f(x);

      // Acceptable approx. is found
      return x;
    }

    // Step at this iteration
    // Obtain the gold section step
    auto new_step = r * (x < middle_range ? b - x : a - x);

    // Decide if the interpolation can be tried

    // If x and w are distinct
    // interpolation may be tried
    if (fabs(x - w) >= tol_act) {
      // Interpolation step is calculated as p/q;
      // division operation is delayed until last moment
      double p;
      double q;

      const auto t = (x - w) * (fx - fv);
      q = (x - v) * (fx - fw);
      p = (x - v) * q - (x - w) * t;
      q = 2 * (q - t);

      // q was calculated with the opposite sign;
      // make q positive and assign possible minus to p
      if (q > 0)
        p = -p;
      else
        q = -q;

      // If x+p/q falls in [a,b] not too close to a and b,
      // and isn't too large it is accepted
      // If p/q is too large then the gold section procedure can
      // reduce [a,b] range to more extent
      if (fabs(p) < fabs(new_step * q) && p > q * (a - x + double_tol_act)
          && p < q * (b - x - double_tol_act))
        new_step = p / q;
    }

    // Adjust the step to be not less than tolerance
    limit_tolerance(new_step, tol_act);

    // Obtain the next approximation to min and reduce the enveloping range
    {
      // Tentative point for the min
      const auto t = x + new_step;
      const auto ft = f(t);
      // t is a better approximation
      if (ft <= fx) {
        // Reduce the range so that t would fall within it
        (t < x ? b : a) = x;

        // Assign the best approx to x
        v = w;
        w = x;
        x = t;

        fv = fw;
        fw = fx;
        fx = ft;
        x_best = false;
      } else {
        // x remains the better approx
        x_best = true;
        // Reduce the range enclosing x
        (t < x ? a : b) = t;

        if ((ft <= fw) || (w == x)) {
          v = w;
          w = t;
          fv = fw;
          fw = ft;
        } else if ((ft <= fv) || (v == x) || (v == w)) {
          v = t;
          fv = ft;
        }
      }
    }
  }
}
