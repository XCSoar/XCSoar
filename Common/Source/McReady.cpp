/*
  XCSoar Glide Computer
  Copyright (C) 2000 - 2004  M Roberts

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
*/
#include "stdafx.h"
#include "McReady.h"
#include "resource.h"
#include "externs.h"

#include "XCSoar.h"

#include <math.h>
#include <windows.h>
/*
  double a = -0.00190772449;
  double b = 0.06724332;
  double c = -1.141438761599;
*/

static double SinkRate(double a,double b, double c, double MC, double HW, double V);

static double BallastFactor;
static double polar_a;
static double polar_b;
static double polar_c;
static int Vminsink = 2;
static int Vbestld = 2;

double sinkratecache[200];


void SetBallast() {
  double BallastWeight;
  BallastWeight = WEIGHTS[2] * BALLAST;
  BallastWeight += WEIGHTS[0] + WEIGHTS[1];
  BallastWeight = (double)sqrt(BallastWeight);
  polar_a = POLAR[0] / BallastWeight;
  polar_b = POLAR[1];
  polar_c = POLAR[2] * BallastWeight;

  // do preliminary scan to find min sink and best LD
  // this speeds up mcready calculations because we have a reduced range
  // to search across.
  // this also limits speed to fly to logical values (will never try
  // to fly slower than min sink speed)

  double minsink = 10000.0;
  double bestld = 0.0;
  int i;

  if ((SAFTEYSPEED==0)||(SAFTEYSPEED>200)) {
    SAFTEYSPEED=150;
  }

  for(i=4;i<SAFTEYSPEED;i+=1)
    {
      double vtrack = (double)i; // TAS along bearing in cruise
      double thesinkrate
        =  -SinkRate(polar_a,polar_b,polar_c,0,0,vtrack);

      double ld = vtrack/thesinkrate;
      if (ld>=bestld) {
        bestld = ld;
        Vbestld = i;
      }
      if (thesinkrate<= minsink) {
        minsink = thesinkrate;
        Vminsink = i;
      }
      sinkratecache[i] = -thesinkrate;

    }

}


double SinkRateFast(double MC, int v) {
  int i = max(4,min(v,(int)SAFTEYSPEED));
  return sinkratecache[i]-MC;
}


double SinkRate(double V) {

  return SinkRate(polar_a,polar_b,polar_c,0.0,0.0,V);

}


double SinkRate(double V, double n) {
  if (n<0.01) {
    n=0.01;
  }
  double sqrtn = (double)isqrt4((unsigned long)(n*10000))/100.0;
  return SinkRate(polar_a/sqrtn,polar_b,polar_c*sqrtn,0.0,0.0,V);

}


double McCreadyAltitude(double MCREADY, double Distance, double Bearing, double WindSpeed, double WindBearing,
		       double *BestCruiseTrack,
		       double *VMcCready,
		       int isFinalGlide,
                       double *TimeToGo)
{
  int i;
  double BestSpeed, BestGlide, Glide;
  double BestSinkRate, TimeToDest;
  double AltitudeNeeded;
  double HeadWind, CrossWind;
  double CrossBearing;
  double VMG;

  CrossBearing = Bearing - WindBearing;

  HeadWind = WindSpeed * fastcosine(CrossBearing);
  CrossWind = WindSpeed * fastsine(CrossBearing);

  // JMW TODO: Calculate best cruise bearing
  double sinkrate;
  double tc; // time spent in cruise
  VMG = 0.00001;

  // JMW TODO this should be modified to incorporate:
  // - [done] best cruise track and bearing (final glide and for waypoint)
  // - climb before or after turning waypoints.
  // - mcready ring changes with height allowing for risk and decreased rate
  // - cloud streets
  // - sink rate between thermals
  // - check these equations allow for bugs
  // - modify Vtrack for IAS

  //Calculate Best Glide Speed
  BestSpeed = 2;
  BestGlide = 0.0000001;// -BestSpeed / SinkRate(a,b,c,MCREADY,HeadWind,2);

  bool effectivefinalglide = isFinalGlide || (MCREADY<=0.2);

  double vtot;
  if (Distance<1.0) {
    Distance = 1;
  }

  double TimeToDestTotal = -1; // initialise to error value

  for(i=Vminsink;i<SAFTEYSPEED;i+=1)
    {
      double vtrack = (double)i; // TAS along bearing in cruise

      // glide angle = velocity projected along path / sink rate
      // need to work out best velocity along path given wind vector
      // need to work out percent time spent cruising
      // SinkRate function returns negative value for sink

      if (effectivefinalglide) {
        //	sinkrate = -SinkRate(polar_a,polar_b,polar_c,MCREADY,0.0,vtrack);
        sinkrate = -SinkRateFast(MCREADY, i);
      } else {

        //	sinkrate = -SinkRate(polar_a,polar_b,polar_c,0.0,0.0,vtrack);
        sinkrate = -SinkRateFast(0, i);
      }
      tc = MCREADY/(sinkrate+MCREADY);

      if (effectivefinalglide) {
	tc = 1.0; // assume no circling, e.g. final glide at best LD with no climbs
      }

      vtot = (vtrack*vtrack*tc*tc-CrossWind*CrossWind);
      if (vtot>0) {
	vtot = sqrt(vtot)-HeadWind;

	if (vtot>0) {

	  Glide = vtot/ (sinkrate);

	  double tcruise = (Distance/(vtot))*tc;
	  double tclimb;
	  if (effectivefinalglide) {
	    tclimb = 0.0;
	  } else {
	    tclimb = sinkrate*(tcruise/MCREADY);
	  }
	  double tdest = tcruise+tclimb;
          if (tdest<1.0) {
            tdest = 1.0;
          }

	  // JMW TODO: fix this...
	  if(
	     ((Glide >= BestGlide)&&(effectivefinalglide))
	     ||
	     ((1/tdest >= BestGlide)&&(!effectivefinalglide))
	     )
	    {
	      if (effectivefinalglide) {
		BestGlide = Glide;
	      } else {
		BestGlide = 1/tdest;
	      }
	      BestSpeed = vtrack;
              if (tdest<=1.0) {
                TimeToDestTotal = -1.0;
              } else {
                TimeToDestTotal = tdest;
              }
	      if (BestCruiseTrack) {
		// best track bearing is the track along cruise that
		// compensates for the drift during climb
		*BestCruiseTrack =
		  atan2(-CrossWind*(1-tc),vtot
			+HeadWind*(1-tc))*RAD_TO_DEG+Bearing;
	      }
	      if (VMcCready) {
		*VMcCready = vtrack;
	      }
	      VMG = vtot/tc; // speed along track during cruise component
	    }
	  else
	    {
	      // no need to continue search, max already found..
	      // break;
	    }
	}
      }

    }

  BestSinkRate =
    SinkRateFast(0,(int)BestSpeed);
  TimeToDest = Distance / (VMG); // this time does not include thermalling part!

  if (TimeToGo) {
    *TimeToGo = TimeToDestTotal;
  }

  AltitudeNeeded = -BestSinkRate * TimeToDest;
  // this is the altitude needed to final glide to destination

  return AltitudeNeeded;
}


double SinkRate(double a,double b, double c, double MC, double HW, double V)
{
  double temp;

  // Quadratic form: w = c+b*(V)+a*V*V

  temp =  a*(V+HW)*(V+HW);
  temp += b*(V+HW);
  temp += c-MC;

  return temp;
}
