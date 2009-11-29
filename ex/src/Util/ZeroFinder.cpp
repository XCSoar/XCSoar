/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include <math.h>
#include <assert.h>
#include <algorithm>
#include <limits>

const double ZeroFinder::epsilon = std::numeric_limits<double>::epsilon();
const double ZeroFinder::sqrt_epsilon = sqrt(std::numeric_limits<double>::epsilon());
const double ZeroFinder::r = (3.-sqrt(5.0))/2;	/* Gold section ratio		*/


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

#include "math.h"

double ZeroFinder::find_zero(const double xstart) {
  double a,b,c;				/* Abscissae, descr. see above	*/
  double fa;				/* f(a)				*/
  double fb;				/* f(b)				*/
  double fc;				/* f(c)				*/

  bool b_best = true; // b is best and last called

  c = a = xmin;  
  fc = fa = f(a);  

  b = xmax;  
  fb = f(b);

  for(;;)		/* Main iteration loop	*/
  {
    double prev_step = b-a;		/* Distance from the last but one*/
					/* to the last approximation	*/    
   
    if( fabs(fc) < fabs(fb) )
    {                         		/* Swap data for b to be the 	*/
      a = b;  b = c;  c = a;          /* best approximation		*/
      fa=fb;  fb=fc;  fc=fa;
      b_best = false;
    } else {
      b_best = true;
    }
    const double tol_act = 		/* Actual tolerance	        */
      2*epsilon*fabs(b) + tolerance/2;

    double new_step = (c-b)*0.5;    /* Step at this iteration       */

    if( fabs(new_step) <= tol_act || fb == (double)0 ) {
      if (!b_best) {
        fb = f(b); // call once more
      }
      return b;				/* Acceptable approx. is found	*/
    }

    			/* Decide if the interpolation can be tried	*/
    if( fabs(prev_step) >= tol_act	/* If prev_step was large enough*/
	&& fabs(fa) > fabs(fb) )	/* and was in true direction,	*/
    {					/* Interpolatiom may be tried	*/

      double p;      			/* Interpolation step is calcu- */
      double q;      			/* lated in the form p/q; divi- */
  					/* sion operations is delayed   */
 					/* until the last moment	*/

      const double cb = c-b;
      if( a==c )			/* If we have only two distinct	*/
	{				/* points linear interpolation 	*/
	  const double t1 = fb/fa;	/* can only be applied		*/
	  p = cb*t1;
	  q = 1.0 - t1;
 	}
	else				/* Quadric inverse interpolation*/
	{
	  q = fa/fc;  
          const double t1 = fb/fc;  
          const double t2 = fb/fa;
	  p = t2 * ( cb*q*(q-t1) - (b-a)*(t1-1.0) );
	  q = (q-1.0) * (t1-1.0) * (t2-1.0);
	}
	if( p>(double)0 )		/* p was calculated with the op-*/
	  q = -q;			/* posite sign; make p positive	*/
	else				/* and assign possible minus to	*/
	  p = -p;			/* q				*/

	if( p < (0.75*cb*q-fabs(tol_act*q)/2)	/* If b+p/q falls in [b,c]*/
	    && p < fabs(prev_step*q/2) )	/* and isn't too large	*/
	  new_step = p/q;			/* it is accepted	*/
					/* If p/q is too large then the	*/
					/* bissection procedure can 	*/
					/* reduce [b,c] range to more	*/
					/* extent			*/
    }

    if( fabs(new_step) < tol_act ) {	/* Adjust the step to be not less*/
      if( new_step > (double)0 )	/* than toleranceerance		*/
	new_step = tol_act;
      else
	new_step = -tol_act;
    }

    a = b;  fa = fb;			/* Save the previous approx.	*/
    b += new_step;                      /* Do step to a new approxim.	*/
    fb = f(b);

    if( (fb > 0 && fc > 0) || (fb < 0 && fc < 0) )
    {                 			/* Adjust c for it to have a sign*/
      c = a;  fc = fa;                  /* opposite to that of b	*/
    }

    assert((b<=xmax) && (b>=xmin));
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
 *	double tolerance;			Acceptable toleranceerance for the minimum
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
double ZeroFinder::find_min(const double xstart)
{
  double x,v,w;				/* Abscissae, descr. see above	*/
  double fx;				/* f(x)				*/
  double fv;				/* f(v)				*/
  double fw;				/* f(w)				*/
  double a = xmin;
  double b = xmax;
  bool x_best = true;

  assert( tolerance > 0 && b > a );

  /* First step - always gold section*/
  x = w = v = a + r*(b-a);
  fx= fw = fv = f(v);

  for(;;)		/* Main iteration loop	*/
  {
    const double range = b-a;      /* Range over which the minimum */
                                   /* is seeked for		*/
    const double middle_range = (a+b)*0.5;

    const double tol_act =			/* Actual toleranceerance		*/
      sqrt_epsilon*fabs(x) + tolerance/3;

    if( fabs(x-middle_range) + range*0.5 <= 2*tol_act ) {
      if (!x_best) {
        fx = f(x); // call once more
      }
      return x;				/* Acceptable approx. is found	*/
    }

    double new_step      		/* Step at this iteration       */
					/* Obtain the gold section step	*/
      = r * ( x<middle_range ? b-x : a-x );

    			/* Decide if the interpolation can be tried	*/
    if( fabs(x-w) >= tol_act  )		/* If x and w are distinct      */
    {					/* interpolatiom may be tried	*/
	double p; 		/* Interpolation step is calcula-*/
	double q;              /* ted as p/q; division operation*/
                                        /* is delayed until last moment	*/

	const double t = (x-w) * (fx-fv);
	q = (x-v) * (fx-fw);
	p = (x-v)*q - (x-w)*t;
	q = 2*(q-t);

	if( q>(double)0 )		/* q was calculated with the op-*/
	  p = -p;			/* posite sign; make q positive	*/
	else				/* and assign possible minus to	*/
	  q = -q;			/* p				*/

	if( fabs(p) < fabs(new_step*q) &&	/* If x+p/q falls in [a,b]*/
	    p > q*(a-x+2*tol_act) &&		/* not too close to a and */
	    p < q*(b-x-2*tol_act)  )            /* b, and isn't too large */
	  new_step = p/q;			/* it is accepted         */
					/* If p/q is too large then the	*/
					/* gold section procedure can 	*/
					/* reduce [a,b] range to more	*/
					/* extent			*/
    }

    if( fabs(new_step) < tol_act ) {	/* Adjust the step to be not less*/
      if( new_step > (double)0 )	/* than toleranceerance		*/
	new_step = tol_act;
      else
	new_step = -tol_act;
    }

				/* Obtain the next approximation to min	*/
    {				/* and reduce the enveloping range	*/
      const double t = x + new_step;	/* Tentative point for the min	*/
      const double ft = f(t);
      if( ft <= fx )
      {                                 /* t is a better approximation	*/
	if( t < x )			/* Reduce the range so that	*/
	  b = x;                        /* t would fall within it	*/
	else
	  a = x;
      
	v = w;  w = x;  x = t;		/* Assign the best approx to x	*/
	fv=fw;  fw=fx;  fx=ft;
        x_best = false;
      }
      else                              /* x remains the better approx  */
      {        		             
        x_best = true;
	if( t < x )			/* Reduce the range enclosing x	*/
	  a = t;                   
	else
	  b = t;
      
        if( ft <= fw || w==x )
        {
           v = w;  w = t;
	   fv=fw;  fw=ft;
        }
        else if( ft<=fv || v==x || v==w )
        {
           v = t;
	   fv=ft;
        }
      }
      
    }			/* ----- end-of-block ----- */
  }		/* ===== End of loop ===== */

}
