#include "stdafx.h"
#include "XCSoar.h"
#include "Utils.h"

#include <math.h>
#define NUMV 40
#define VSCALE 10.0
#define NUMP 20
#define NUMTHETA (36)
#define DEGTORAD (M_PI/180.0)

static double anglelimit(double ang) {
  while (ang<-M_PI) {
    ang+= 2.0*M_PI;
  }
  while (ang>M_PI) {
    ang-= 2.0*M_PI;
  }
  return ang;
}


class ZigZagPoint {
public:
  ZigZagPoint() {
    time = -1;
    for (int i=0; i<NUMV; i++) {
      theta_west_ok[i] = false;
    }    
  }

  double V_tas;
  double V_gps;
  double theta_gps;
  double time;

  void Set(double t, double aV_tas, double aV_gps, double atheta_gps) {
    V_tas = aV_tas;
    V_gps = aV_gps;
    time = t;
    theta_gps = atheta_gps;
    CalculateThetaWEst();
  }

  double theta_west_1[NUMV];
  double theta_west_2[NUMV];
  bool theta_west_ok[NUMV];

  double cos_theta_gps;
  double sin_theta_gps;

private:
  int V_gps_x;
  int V_gps_y;
  int V_tas_l;


  void CalculateThetaWEst() {
    int i;
    for (i=0; i<NUMV; i++) {
      double Vwest = i*VSCALE/NUMV;
      theta_west_ok[i] = EstimateW0(Vwest,i);
    }
    cos_theta_gps = cos(theta_gps);
    sin_theta_gps = sin(theta_gps);
    V_gps_x = iround(100*V_gps*cos_theta_gps);
    V_gps_y = iround(100*V_gps*sin_theta_gps);
    V_tas_l = iround(100*V_tas);
    
  }

  bool EstimateW0(double Vwest, int i) {
    double west_rat = Vwest/V_tas;
    double gps_rat = V_gps/V_tas;

    if (gps_rat<0.001) {
      // speed too small
      return false;
    }
    if (gps_rat+west_rat<1.0) {
      // wind too weak
      return false;
    }
    if ((Vwest>V_gps)&&(Vwest>V_tas)) {
      // wind too strong
      return false;
    }
    if (west_rat<0.001) {
      // wind speed too small
      return false;
    }

    double cosgamma = (west_rat*west_rat+gps_rat*gps_rat-1.0)/
      (2.0*west_rat*gps_rat);
    if (fabs(cosgamma)<=1.0) {
      double gamma = acos(cosgamma);
      theta_west_1[i]= -anglelimit(M_PI-theta_gps-gamma);
      theta_west_2[i]= -anglelimit(M_PI-theta_gps+gamma);

      return true;
    } else {
      return false;
    }
  }

public:
  int EstimateW1(int V_west_x, int V_west_y) {
    int v_tas_x = V_gps_x+V_west_x;
    int v_tas_y = V_gps_y+V_west_y;
    long vv = isqrt4(v_tas_x*v_tas_x+v_tas_y*v_tas_y);
    int err = (1000*abs((long)V_tas_l-vv))/V_tas_l;
    // returns error in tenths of percent
    return err;
  }

};


class ZigZag {
public:

  ZigZagPoint points[NUMP];

  ZigZag() {
    for (int i=0; i<NUMP; i++) {
      points[i].time = -1;
    }
    for (int k=0; k<NUMTHETA; k++) {
      double theta = anglelimit(k*2.0*M_PI/NUMTHETA);
      thetalist[k]= theta;
    }

  }

  void AddPoint(double t, double V_tas, double V_gps, double theta_gps) {
    // find oldest point
    double toldest = 0;
    int ioldest=0;
    int i;
    for (i=0; i<NUMP; i++) {
      if (t<points[i].time) {
	points[i].time = -1;
      }
    }    
    for (i=0; i<NUMP; i++) {
      if ((points[i].time<toldest)||(i==0)||(t<points[i].time)) {
	toldest = points[i].time;
	ioldest=i;
      }
    }
    
    i = ioldest;
    points[i].Set(t,V_tas,V_gps,theta_gps);
  }

  bool CheckValidity(double t) {
    // requires:
    // -- all points to be initialised
    // -- all points to be within last 5 minutes
    // -- spread in bearing of >40 degrees
    int nf=0;
    double ctg=0.0;
    double stg=0.0;
    for (int i=0; i<NUMP; i++) {
      if (points[i].time>0) {
	nf++;
	if (t-points[i].time>10*60) {
	  // clear point so it gets filled next time
	  points[i].time = -1;
	  return false;
	}
	ctg += points[i].cos_theta_gps;
	stg += points[i].sin_theta_gps;
      }
    }
    if (nf<NUMP) {
      return false;
    } else {
      double theta_av = atan2(stg,ctg);
      double dtheta_max = 0;
      double dtheta_min = 0;
      for (i=0; i<NUMP; i++) {
	double da = anglelimit(points[i].theta_gps-theta_av);
	if (da>dtheta_max) {
	  dtheta_max = da;
	}
	if (da<dtheta_min) {
	  dtheta_min = da;
	}
      }
      double spread = dtheta_max-dtheta_min;
      if ((spread<40*DEGTORAD)||(spread>270*DEGTORAD)) {
	return false;
      } else {
	return true;
      }
    }
  }

private:
  double thetalist[NUMTHETA];

  // search all points for error against theta at wind speed index j
  double AngleError(double theta, int j) {
    double de=0;
    int nf=0;
    for (int i=0; i<NUMP; i++) {
      if (points[i].theta_west_ok[j]) {
	double e1 = fabs(anglelimit(theta-points[i].theta_west_1[j]));
	double e2 = fabs(anglelimit(theta-points[i].theta_west_2[j]));
	if (e1<=e2) {
	  de += e1*e1;
	} else {
	  de += e2*e2;
	}
	nf++;
      } 
    }
    if (nf>0) {
      return de/nf;
    } else {
      return -1;
    }
  }

  // search for theta to give best match at wind speed index i
  double FindBestAngle(int i, double *theta_best) {
    double debest = 1000000;
    bool ok=false;
    for (int k=0; k<NUMTHETA; k++) {
      double ae=AngleError(thetalist[k], i);
      if (ae<0) {
	continue;
      }
      ok = true;
      if (ae<debest) {
	debest = ae;
	*theta_best = thetalist[k];
      }
    }
    return ok;
  }

  // find average error in true airspeed given wind estimate
  int VError(int V_west_x, int V_west_y) {
    int verr = 0;
    int nf=0;
    for (int j=0; j<NUMP; j++) {
      if (points[j].time>=0) {
	verr+= points[j].EstimateW1(V_west_x,
				    V_west_y);
	nf++;
      }
    }
    if (nf>0) {
      return verr/nf;
    } else {
      return 0;
    }
  }

  bool UpdateSearch(double V_west, double theta_west, 
		    double *V_westb, double *theta_westb,
		    bool start) {

    static int vdebest = 10000;
    int i = iround(V_west*NUMV/VSCALE);
    if (start) {
      *V_westb = V_west;
      *theta_westb = theta_west;
      vdebest = 10000;      
    }

    // find best angle estimate for wind speed i
    double theta_best=0.0;
    if (start) {
      theta_best = *theta_westb;
    } else {
      if (!FindBestAngle(i, &theta_best)) {
	theta_best = *theta_westb;
      }
    }
    
    // given wind speed and direction, find TAS error
    int V_west_x = iround(100*V_west*sin(theta_best));
    int V_west_y = iround(100*V_west*cos(theta_best));
    int verr = VError(V_west_x, V_west_y);
    
    // search for minimum error
    // (this is not monotonous)
    if ((verr<vdebest)&&(verr<100)&&(!start)) {
      vdebest = verr;
      *V_westb = V_west;
      *theta_westb = theta_best;
      return true;
    } else {
      if (start) {
	vdebest = verr;
      }
      return false;
    }
  }

public:
  bool Estimate(double *V_westb, double *theta_westb, double *error) {
    int i;
    bool improved=false;
    bool scanned[NUMV];
    for (i=0; i<NUMV; i++) {
      scanned[i]=false;
    }

    // find error at current wind estimate
    UpdateSearch(*V_westb, *theta_westb, 
		 V_westb, theta_westb,
		 true);

    // scan for 6 points around current best estimate.
    // if a better estimate is found, keep scanning around
    // that point, and don't repeat scans
    
    bool improved_this = true;
    while (improved_this) {
      improved_this = false;
      int ib = iround(*V_westb*NUMV/VSCALE);
      int il, ih;
      il = min(NUMV-1,max(0,ib-3));
      ih = min(NUMV-1,max(0,ib+3));
      for (i=il; i<=ih; i++) {
	if (scanned[i]) continue;
	// see if we can find a better estimate
	double V_west = i*VSCALE/NUMV;
	improved_this |= UpdateSearch(V_west, *theta_westb, 
				      V_westb, theta_westb,
				      false);
	scanned[i]= true;
      }
      improved |= improved_this;
    }
    // return true if estimate was improved
    return improved;
  };

};




//////////////////////////////////////////////////////////////////////

ZigZag myzigzag;



void TestZigZag() {


  double t, V_tas, V_gps, theta_gps, V_wind, theta_wind, theta_glider;

  V_wind = 5.0;
  theta_wind = 45*DEGTORAD;
  int i;
  for (i=0; i<=NUMP; i++) {
    t = i;
    V_tas = 20.0;
    theta_glider = sin(t*M_PI*2.0/30.0)*30*DEGTORAD;
    double V_gps_x = V_tas * sin(theta_glider) - V_wind*sin(theta_wind);
    double V_gps_y = V_tas * cos(theta_glider) - V_wind*cos(theta_wind);

    V_gps = sqrt(V_gps_x*V_gps_x+V_gps_y*V_gps_y);
    theta_gps = atan2(V_gps_x,V_gps_y);

    myzigzag.AddPoint(t, V_tas, V_gps, theta_gps);
  }

  // ok, ready to calculate
  if (myzigzag.CheckValidity(t+1)) {
    // data is ok to make an estimate
    double V_wind_estimate=0;
    double theta_wind_estimate=0;
    double percent_error=0;
    myzigzag.Estimate(&V_wind_estimate, &theta_wind_estimate, &percent_error);
  }

}


static void ZigZagStore(char *Str)
{
  FILE *stream;
  static TCHAR szFileName[] = TEXT("\\NOR Flash\\xcsoar-zigzag.log");

  stream = _wfopen(szFileName,TEXT("a+"));

  fwrite(Str,strlen(Str),1,stream);

  fclose(stream);
}


bool WindZigZagUpdate(NMEA_INFO* Basic, DERIVED_INFO* Calculated,
		      double *zzwindspeed, double *zzwindbearing) {
  bool doupdate = false;
  static double tlastupdate = -1;

#ifndef _SIM_
  if (!Basic->AirspeedAvailable) {
    return false;
  }
#endif

  // TestZigZag();  

  // TODO: also reject if accelerating (|n-1|>0.1 ?)
  // TODO: correct TAS for vertical speed
  if (fabs(Calculated->TurnRate)>10.0) {
    return false;
  }
  if (fabs(Basic->TrueAirspeed)<10.0) {
    return false;
  }

  if (Basic->AccelerationAvailable) {
    if (fabs(Basic->Gload-1.0)>0.2) {
      return false;
    }
  }

  static double tLast = -1;
  static double bearingLast=0;

  if (Basic->Time>tLast+5.0) {
    doupdate = true;
  }
  if (fabs(bearingLast-Basic->TrackBearing)>10.0) {
    doupdate = true;
  }

  if (!doupdate) {
    return false;
  }

  tLast = Basic->Time;
  bearingLast = Basic->TrackBearing;

  myzigzag.AddPoint(Basic->Time, Basic->TrueAirspeed, 
		    Basic->Speed, Basic->TrackBearing*DEGTORAD);

  if (myzigzag.CheckValidity(Basic->Time)) {
    // data is ok to make an estimate

    double V_wind_estimate = Calculated->WindSpeed;
    double theta_wind_estimate = Calculated->WindBearing*DEGTORAD;
    double percent_error=0;
    DWORD tzero = ::GetTickCount();

    if (myzigzag.Estimate(&V_wind_estimate, 
			  &theta_wind_estimate, 
			  &percent_error)) {
      if (theta_wind_estimate<0) {
	theta_wind_estimate+= 2.0*M_PI;
      }
      theta_wind_estimate /= DEGTORAD;
      DWORD tone = ::GetTickCount();
      int dt = tone-tzero;
      
      *zzwindspeed = V_wind_estimate;
      *zzwindbearing = theta_wind_estimate;

      /*
      char text[100];
      sprintf(text,"%f %3.1f %03.0f %3.1f %03.0f %f # zigzag\n", 
	      Basic->Time,
	      V_wind_estimate,
	      theta_wind_estimate,
	      Calculated->WindSpeed, 
	      Calculated->WindBearing,
	      percent_error);
      ZigZagStore(text);
      */

      if (Basic->Time<= tlastupdate) {
	tlastupdate = Basic->Time;
      }

      // don't update wind from zigzag more often than 
      // every 20 seconds, so it is balanced with respect
      // to circling
      if (Basic->Time-tlastupdate>20) {
	tlastupdate = Basic->Time;
	return true;
      } else {
	return false;
      }
    }
  }
  return false;
}
