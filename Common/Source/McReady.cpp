/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "McReady.h"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Device/device.h"
#include "Thread/Mutex.hpp"
#include "Units.hpp"
#include <tchar.h>
#include <math.h>
#include <windows.h>
/*
  double a = -0.00190772449;
  double b = 0.06724332;
  double c = -1.141438761599;
*/


//double GlidePolar::BallastFactor;
double GlidePolar::RiskGamma = 0.0;
double GlidePolar::polar_a;
double GlidePolar::polar_b;
double GlidePolar::polar_c;
int GlidePolar::Vminsink = 2;
int GlidePolar::Vbestld = 2;
double GlidePolar::sinkratecache[MAXSAFETYSPEED];
double GlidePolar::bestld = 0.0;
double GlidePolar::minsink = 10000.0;
double GlidePolar::BallastLitres = 0.0;
double GlidePolar::WingArea = 0.0;
double GlidePolar::WingLoading = 0.0;

double GlidePolar::SafetyMacCready= 0.0;
bool GlidePolar::AbortSafetyUseCurrent = false;

static int iSAFETYSPEED=0;

static Mutex mutexGlidePolar;

//Flight Data Globals
double        GlidePolar::MACCREADY = 0; // JMW now in SI units (m/s) for consistency
double        GlidePolar::BUGS = 1;
double        GlidePolar::BALLAST = 0;
double        GlidePolar::CRUISE_EFFICIENCY = 1.0;

int           POLARID = 0;
double        POLAR[POLARSIZE] = {0,0,0};
double        WEIGHTS[POLARSIZE] = {250,70,100};
bool          AutoMacCready = false;

void GlidePolar::Lock() {
  mutexGlidePolar.Lock();
}

void GlidePolar::Unlock() {
  mutexGlidePolar.Unlock();
}

double GlidePolar::AbortSafetyMacCready() {
  double retval;
  Lock();
  if (AbortSafetyUseCurrent) {
    retval = MACCREADY;
  } else {
    retval= SafetyMacCready;
  }
  Unlock();
  return retval;
}

double GlidePolar::GetAUW() {
  double retval;
  Lock();
  retval = BallastLitres + WEIGHTS[0] + WEIGHTS[1];
  Unlock();
  return retval;
}

double GlidePolar::GetMacCready() {
  double retval;
  Lock();
  retval = MACCREADY;
  Unlock();
  return retval;
}

double GlidePolar::GetBugs() {
  double retval;
  Lock();
  retval = BUGS;
  Unlock();
  return retval;
}

double GlidePolar::GetBallast() {
  double retval;
  Lock();
  retval = BALLAST;
  Unlock();
  return retval;
}

double GlidePolar::GetCruiseEfficiency() {
  double retval;
  Lock();
  retval = CRUISE_EFFICIENCY;
  Unlock();
  return retval;
}

double GlidePolar::GetBallastLitres() {
  double retval;
  Lock();
  retval = BallastLitres;
  Unlock();
  return retval;
}

void GlidePolar::SetMacCready(double val) {
  Lock();
  MACCREADY = val;
  Unlock();
}

void GlidePolar::SetBugs(double val) {
  Lock();
  BUGS = val;
  Unlock();
}

void GlidePolar::SetCruiseEfficiency(double val) {
  Lock();
  CRUISE_EFFICIENCY = val;
  Unlock();
}

void GlidePolar::SetBallast(double val) {
  Lock();
  BALLAST = val;
  Unlock();
}

void GlidePolar::UpdatePolar(bool send) {

  Lock();
  double BallastWeight;
  BallastLitres = WEIGHTS[2] * BALLAST;
  BallastWeight = GetAUW();
  if (WingArea>0.1) {
    WingLoading = BallastWeight/WingArea;
  } else {
    WingLoading = 0;
  }
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

  if ((SAFTEYSPEED==0)||(SAFTEYSPEED>=MAXSAFETYSPEED)) {
    SAFTEYSPEED=MAXSAFETYSPEED-1;
  }
  iSAFETYSPEED=(int)SAFTEYSPEED;

  for(i=4;i<=iSAFETYSPEED;i++)
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

  int polar_ai = iround((polar_a*10)*4096);
  int polar_bi = iround((polar_b)*4096);
  int polar_ci = iround((polar_c/10)*4096);
  int minsinki = -iround(minsink*10);
  int vbestldi = iround(Vbestld*10);
  int bestldi = iround(bestld*10);
  Unlock();
  /* use this instead? etc
    devPutBugs(devA(), BUGS);
    devPutBugs(devB(), BUGS);
/// JMW TODO

  if (GPS_INFO.VarioAvailable) {

    TCHAR nmeabuf[100];
    _stprintf(nmeabuf,TEXT("PDVGP,%d,%d,%d,%d,%d,%d,0"),
              polar_ai,
              polar_bi,
              polar_ci,
              minsinki,
              vbestldi,
              bestldi);

    VarioWriteNMEA(nmeabuf);
  }
  */

  if (send) {
    devPutBallast(devA(), BALLAST);
    devPutBallast(devB(), BALLAST);
    devPutBugs(devA(), BUGS);
    devPutBugs(devB(), BUGS);
  }
}


inline double GlidePolar::_SinkRateFast(const double &MC, const int &v) {
  return sinkratecache[v]-MC;
}

double GlidePolar::SinkRateFast(const double &MC, const int &v) {
  return _SinkRateFast(MC, max(4,min(iSAFETYSPEED, v)));
}


double GlidePolar::SinkRate(double V) {

  return SinkRate(polar_a,polar_b,polar_c,0.0,0.0,V);

}


#define MIN_MACCREADY 0.000000000001


double GlidePolar::SinkRate(double V, double n) {
  double w0 = SinkRate(polar_a,polar_b,polar_c,0.0,0.0,V);
  n = max(0.1,fabs(n));
  //  double v1 = V/max(1,Vbestld);
  double v2 = Vbestld/max(Vbestld/2,V);
  return w0-(V/(2*bestld))* (n*n-1)*(v2*v2);
}


double GlidePolar::MacCreadyAltitude_internal(double emcready,
                                              double Distance,
                                              double Bearing,
                                              const double WindSpeed,
                                              const double WindBearing,
                                              double *BestCruiseTrack,
                                              double *VMacCready,
                                              const bool isFinalGlide,
                                              double *TimeToGo,
					      const double cruise_efficiency)
{

  int i;
  double BestSpeed, BestGlide, Glide;
  double BestSinkRate, TimeToDestCruise;
  static double HeadWind, CrossWind=0.0;
  static double CrossBearingLast= -1.0;
  static double WindSpeedLast= -1.0;
  double CrossBearing;
  double BestTime;
  static double HeadWindSqd, CrossWindSqd=0.0;

  CrossBearing = AngleLimit360(Bearing - WindBearing);
  if ((CrossBearing != CrossBearingLast)||(WindSpeed != WindSpeedLast)) {
    // saves a few floating point operations
    HeadWind = WindSpeed * fastcosine(CrossBearing);
    CrossWind = WindSpeed * fastsine(CrossBearing);
    HeadWindSqd = HeadWind*HeadWind;
    CrossWindSqd = CrossWind*CrossWind;

    // save old values
    CrossBearingLast = CrossBearing;
    WindSpeedLast = WindSpeed;
  }

  double sinkrate;
  double tc; // time spent in cruise

  // TODO accuracy: extensions to Mc to incorporate real-life issues
  // - [done] best cruise track and bearing (final glide and for waypoint)
  // - climb before or after turning waypoints.
  // - mcready ring changes with height allowing for risk and decreased rate
  // - cloud streets
  // - sink rate between thermals
  // - modify Vtrack for IAS

  //Calculate Best Glide Speed
  BestSpeed = 2;
  BestGlide = 10000;
  BestTime = 1e6;

  if (BestCruiseTrack) {
    *BestCruiseTrack = Bearing;
  }

  double vtot;
  if (Distance<1.0) {
    Distance = 1;
  }

  double TimeToDestTotal = ERROR_TIME; // initialise to error value
  TimeToDestCruise = -1; // initialise to error value

  for(i=Vminsink;i<iSAFETYSPEED;i++) {
    double vtrack_real = ((double)i); // actual airspeed
    double vtrack = vtrack_real*cruise_efficiency;
    // TAS along bearing in cruise

    // glide angle = velocity projected along path / sink rate
    // need to work out best velocity along path given wind vector
    // need to work out percent time spent cruising
    // SinkRate function returns negative value for sink

    if (isFinalGlide) {
      sinkrate = -_SinkRateFast(max(0.0,emcready), i);
      tc = 1.0; // assume no circling, e.g. final glide at best LD
      // with no climbs
    } else {
      emcready = max(MIN_MACCREADY,emcready);
      sinkrate = -_SinkRateFast(0.0, i);
      tc = max(0.0,min(1.0,emcready/(sinkrate+emcready)));
    }

    // calculate average speed along track relative to wind
    vtot = (vtrack*vtrack*tc*tc-CrossWindSqd);

    // if able to advance against crosswind
    if (vtot>0) {
      // if able to advance against headwind
      if (vtot>HeadWindSqd) {
	// calculate average speed along track relative to ground
	vtot = sqrt(vtot)-HeadWind;
      } else {
	// can't advance at this speed
	continue;
      }
    }

    // can't advance at this speed
    if (vtot<=0) continue;

    bool bestfound = false;

    if (isFinalGlide) {
      // inverse glide ratio relative to ground
      Glide = sinkrate/vtot;

      // best glide angle when in final glide
      if (Glide <= BestGlide) {
	bestfound = true;
	BestGlide = Glide;
	TimeToDestTotal = Distance/vtot;
      }
    } else {
      // time spent in cruise
      double Time_cruise = (tc/vtot)*Distance;
      double Time_climb = sinkrate*(Time_cruise/emcready);

      // total time to destination
      TimeToDestTotal = max(Time_cruise+Time_climb,0.0001);
      // best average speed when in maintaining height mode
      if (TimeToDestTotal <= BestTime) {
	bestfound = true;
	BestTime = TimeToDestTotal;
      }
    }

    if (bestfound) {
      BestSpeed = min(SAFTEYSPEED, vtrack_real);
      if (BestCruiseTrack) {
	// best track bearing is the track along cruise that
	// compensates for the drift during climb
	*BestCruiseTrack =
	  atan2(CrossWind*(tc-1),vtot
		+HeadWind*(1-tc))*RAD_TO_DEG+Bearing;
      }
      if (VMacCready) {
	*VMacCready = BestSpeed;
      }

      // speed along track during cruise component
      TimeToDestCruise = Distance*tc/vtot;
    } else {
	// no need to continue search, max already found..
      break;
    }
  }

  BestSinkRate = SinkRateFast(0,(int)BestSpeed);

  if (TimeToGo) {
    *TimeToGo = TimeToDestTotal;
  }

  // this is the altitude needed to final glide to destination
  return -BestSinkRate * TimeToDestCruise;
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



double GlidePolar::FindSpeedForSinkRate(double w) {
  // find the highest speed that provides a sink rate less than
  // the specified sink rate
  double vbest= Vminsink;
  for (int v=4; v<iSAFETYSPEED; v++) {
    double wthis = _SinkRateFast(0, v);
    if (wthis<w) {
      vbest = v;
    }
  }
  return vbest;
}


double GlidePolar::MacCreadyAltitude_heightadjust(double emcready,
						  double Distance,
						  double Bearing,
						  const double WindSpeed,
						  const double WindBearing,
						  double *BestCruiseTrack,
						  double *VMacCready,
						  const bool isFinalGlide,
                                                  double *TimeToGo,
                                                  const double AltitudeAboveTarget,
						  const double cruise_efficiency)
{
  double Altitude;
  double TTG = 0;

  if (!isFinalGlide || (AltitudeAboveTarget<=0)) {

    // if not in final glide or below target altitude, need to
    // climb-cruise the whole way

    Altitude = MacCreadyAltitude_internal(emcready,
                                          Distance, Bearing,
                                          WindSpeed, WindBearing,
                                          BestCruiseTrack,
                                          VMacCready,
                                          false,
                                          &TTG,
					  cruise_efficiency);
  } else {

    // if final glide mode and can final glide part way

    double t_t = ERROR_TIME;
    double h_t = MacCreadyAltitude_internal(emcready,
                                            Distance, Bearing,
                                            WindSpeed, WindBearing,
                                            BestCruiseTrack,
                                            VMacCready,
                                            true,
                                            &t_t,
					    cruise_efficiency);

    if (h_t<=0) {
      // error condition, no distance to travel
      TTG = t_t;
      Altitude = 0;

    } else {
      double h_f = AltitudeAboveTarget;
      // fraction of leg that can be final glided
      double f = min(1.0,max(0.0,h_f/h_t));

      if (f<1.0) {
        // if need to climb-cruise part of the way

	double d_c = Distance*(1.0 - f);

	double t_c;
	double h_c = MacCreadyAltitude_internal(emcready,
						d_c, Bearing,
						WindSpeed, WindBearing,
						BestCruiseTrack,
						VMacCready,
						false,
						&t_c,
						cruise_efficiency);

        if (h_c<0) {
          // impossible at this Mc, so must be final glided
          Altitude = -1;
          TTG = ERROR_TIME;
        } else {
          Altitude = f*h_t + h_c;
	  TTG = f*t_t + t_c;
        }

      } else {

        // can final glide the whole way

        Altitude = h_t;
        TTG = t_t;

      }
    }
  }

  if (TimeToGo) {
    *TimeToGo = TTG;
  }
  return Altitude;


}


double GlidePolar::MacCreadyAltitude(double emcready,
                                     double Distance,
				     const double Bearing,
                                     const double WindSpeed,
				     const double WindBearing,
                                     double *BestCruiseTrack,
                                     double *VMacCready,
                                     const bool isFinalGlide,
                                     double *TimeToGo,
                                     const double AltitudeAboveTarget,
				     const double cruise_efficiency) {

  double TTG = ERROR_TIME;
  double Altitude = -1;
  bool invalidMc = (emcready<MIN_MACCREADY);
  bool invalidAltitude = false;

  if (!invalidMc || isFinalGlide) {
    Altitude = MacCreadyAltitude_heightadjust(emcready,
                                              Distance, Bearing,
                                              WindSpeed, WindBearing,
                                              BestCruiseTrack,
                                              VMacCready,
                                              isFinalGlide,
                                              &TTG,
                                              AltitudeAboveTarget,
					      cruise_efficiency);
    if (Altitude<0) {
      invalidAltitude = true;
    } else {
      // All ok
      if (TTG<0.9*ERROR_TIME) {
        goto onExit;
      }
    }
  }

  // Never going to make it at this rate, so assume final glide
  // with no climb
  // This can occur if can't make progress against headwind,
  // or if Mc is too small

  Altitude = MacCreadyAltitude_heightadjust(emcready,
                                            Distance, Bearing,
                                            WindSpeed, WindBearing,
                                            BestCruiseTrack,
                                            VMacCready,
                                            true,
                                            &TTG, 1.0e6,
					    cruise_efficiency);

  if (invalidAltitude) {
    TTG += ERROR_TIME;
    // if it failed due to invalid Mc, need to increase estimated
    // time and the glider better find that lift magically
  }

 onExit:
  if (TimeToGo) {
    *TimeToGo = TTG;
  }

  return Altitude;

}

static double FRiskFunction(double x, double k) {
  return 2.0/(1.0+exp(-x*k))-1.0;
}

double GlidePolar::MacCreadyRisk(double HeightAboveTerrain,
                                 double MaxThermalHeight,
                                 double MC) {
  double riskmc = MC;

  double hthis = max(1.0, HeightAboveTerrain);
  double hmax = max(hthis, MaxThermalHeight);
  double x = hthis/hmax;
  double f;

  if (RiskGamma<0.1) {
    return MC;
  } else if (RiskGamma>0.9) {
    f = x;
  } else {
    double k;
    k = 1.0/(RiskGamma*RiskGamma)-1.0;
    f = FRiskFunction(x, k)/FRiskFunction(1.0, k);
  }
  double mmin = 0; // min(MC,AbortSafetyMacCready());
  riskmc = f*riskmc+(1-f)*mmin;
  return riskmc;
}
