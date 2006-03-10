/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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
#include "stdafx.h"
#include "McReady.h"
#include "resource.h"
#include "externs.h"

#include "XCSoar.h"
#include "Port.h"

#include <math.h>
#include <windows.h>
/*
  double a = -0.00190772449;
  double b = 0.06724332;
  double c = -1.141438761599;
*/

double GlidePolar::BallastFactor;
double GlidePolar::polar_a;
double GlidePolar::polar_b;
double GlidePolar::polar_c;
int GlidePolar::Vminsink = 2;
int GlidePolar::Vbestld = 2;
double GlidePolar::sinkratecache[200];
double GlidePolar::bestld = 0.0;
double GlidePolar::minsink = 10000.0;


void GlidePolar::SetBallast() {
  LockFlightData();
  double BallastWeight;
  BallastWeight = WEIGHTS[2] * BALLAST;
  BallastWeight += WEIGHTS[0] + WEIGHTS[1];
  BallastWeight = (double)sqrt(BallastWeight);
  double bugfactor = 1.0/BUGS;
  polar_a = POLAR[0] / BallastWeight*bugfactor;
  polar_b = POLAR[1] * bugfactor;
  polar_c = POLAR[2] * BallastWeight*bugfactor;

  // do preliminary scan to find min sink and best LD
  // this speeds up mcready calculations because we have a reduced range
  // to search across.
  // this also limits speed to fly to logical values (will never try
  // to fly slower than min sink speed)

  minsink = 10000.0;
  bestld = 0.0;
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
  UnlockFlightData();

  int polar_ai = iround((polar_a*10)*4096);
  int polar_bi = iround((polar_b)*4096);
  int polar_ci = iround((polar_c/10)*4096);
  int minsinki = -iround(minsink*10);
  int vbestldi = iround(Vbestld*10);
  int bestldi = iround(bestld*10);

  if (GPS_INFO.VarioAvailable) {

    TCHAR nmeabuf[100];
    wsprintf(nmeabuf,TEXT("PDVGP,%d,%d,%d,%d,%d,%d,0"),
	     polar_ai,
	     polar_bi,
	     polar_ci,
	     minsinki,
	     vbestldi,
	     bestldi);

    VarioWriteNMEA(nmeabuf);
  }

}


double GlidePolar::SinkRateFast(double MC, int v) {
  int i = max(4,min(v,(int)SAFTEYSPEED));
  return sinkratecache[i]-MC;
}


double GlidePolar::SinkRate(double V) {

  return SinkRate(polar_a,polar_b,polar_c,0.0,0.0,V);

}


double GlidePolar::SinkRate(double V, double n) {
  if (n<0.01) {
    n=0.01;
  }
  double sqrtn = (double)isqrt4((unsigned long)(n*10000))/100.0;
  return SinkRate(polar_a/sqrtn,polar_b,polar_c*sqrtn,0.0,0.0,V);

}


double GlidePolar::MacCreadyAltitude(double emcready,
                                    double Distance, double Bearing,
                                    double WindSpeed, double WindBearing,
		       double *BestCruiseTrack,
		       double *VMacCready,
		       bool isFinalGlide,
                       double *TimeToGo)
{

  int i;
  double BestSpeed, BestGlide, Glide;
  double BestSinkRate, TimeToDest;
  double AltitudeNeeded;
  double HeadWind, CrossWind;
  double CrossBearing;
  double VMG;
  double BestTime;

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
  BestGlide = 10000;
  BestTime = 0;

  bool effectivefinalglide = isFinalGlide;

  if (emcready<=0.0) {
    effectivefinalglide = true;
  }

  double vtot;
  if (Distance<1.0) {
    Distance = 1;
  }

  double TimeToDestTotal = -1; // initialise to error value
  double tcruise, tclimb, tdest;

  for(i=Vminsink;i<SAFTEYSPEED;i+=1)
    {
      double vtrack = (double)i; // TAS along bearing in cruise

      // glide angle = velocity projected along path / sink rate
      // need to work out best velocity along path given wind vector
      // need to work out percent time spent cruising
      // SinkRate function returns negative value for sink

      if (effectivefinalglide) {

        sinkrate = -SinkRateFast(emcready, i);
	tc = 1.0; // assume no circling, e.g. final glide at best LD
                  // with no climbs

      } else {

        sinkrate = -SinkRateFast(0, i);
        tc = max(0.0,min(1.0,emcready/(sinkrate+emcready)));
      }

      // calculate average speed along track relative to wind
      vtot = (vtrack*vtrack*tc*tc-CrossWind*CrossWind);

      // if able to advance against crosswind
      if (vtot>0) {

        // calculate average speed along track relative to ground
	vtot = sqrt(vtot)-HeadWind;

        // if able to advance against headwind
	if (vtot>0) {

          // inverse glide ratio relative to ground
          Glide = (sinkrate)/vtot;

          // time spent in cruise
	  tcruise = (Distance/(vtot))*tc;

          // time spent in climb
	  tclimb;

	  if (effectivefinalglide) {
	    tclimb = 0.0;
	  } else {
            tclimb = sinkrate*(tcruise/emcready);
	  }

          // total time to destination
	  tdest = max(tcruise+tclimb,0.00000001);

	  if(
             // best glide angle when in final glide
	     ((Glide <= BestGlide)&&(effectivefinalglide))
	     ||
             // best average speed when in maintaining height mode
	     ((1/tdest >= BestTime)&&(!effectivefinalglide))
	     )
	    {

	      if (effectivefinalglide) {
		BestGlide = Glide;
	      } else {
                BestTime = 1/tdest;
              }

	      BestSpeed = vtrack;
              TimeToDestTotal = tdest;

	      if (BestCruiseTrack) {
		// best track bearing is the track along cruise that
		// compensates for the drift during climb
		*BestCruiseTrack =
		  atan2(-CrossWind*(1-tc),vtot
			+HeadWind*(1-tc))*RAD_TO_DEG+Bearing;
	      }

	      if (VMacCready) {
		*VMacCready = vtrack;
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


double GlidePolar::SinkRate(double a,double b, double c,
                            double MC, double HW, double V)
{
  double temp;

  // Quadratic form: w = c+b*(V)+a*V*V

  temp =  a*(V+HW)*(V+HW);
  temp += b*(V+HW);
  temp += c-MC;

  return temp;
}

