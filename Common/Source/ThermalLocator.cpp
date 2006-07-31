#include "stdafx.h"
#include "XCSoar.h"
#include "ThermalLocator.h"
#include "Utils.h"
#include <math.h>

int EnableThermalLocator = 1;

#define SFACT 111195

void ThermalLocator_Point::Drift(double t_0,
				 double longitude_0, double latitude_0,
				 double drift_lon, double drift_lat) {

  // convert to flat earth coordinates, then drift by wind and delta t  
  double dt = t_0-t;
  weight = 1.0/(exp(-3.0*dt/TLOCATOR_NMAX));
  x = (longitude+drift_lon*dt-longitude_0)*fastcosine(latitude_0);
  y = (latitude+drift_lat*dt-latitude_0);

  iweight = iround(weight*100);
  xiw = iround(x*SFACT*iweight);
  yiw = iround(y*SFACT*iweight);

};


ThermalLocator::ThermalLocator() {
  initialised = true;
  Reset();
}


void ThermalLocator::Reset() {
  if (initialised) {
    initialised = false;
    
    // clear array
    for (int i=0; i<TLOCATOR_NMAX; i++) {
      points[i].valid = false;
    }
    nindex = 0;
    npoints = 0;
  }
}


void ThermalLocator::AddPoint(double t, double longitude, double latitude, double w) {
  points[nindex].longitude = longitude;
  points[nindex].latitude = latitude;
  points[nindex].t = t;
  points[nindex].w = w;
  points[nindex].iw = iround(w*10);
  //  points[nindex].logw = log(max(w,0.1)*10.0);
  points[nindex].valid = true;
  nindex++;
  nindex = (nindex % TLOCATOR_NMAX);

  if (npoints<TLOCATOR_NMAX-1) {
    npoints++;
  }

  if (!initialised) {
    initialised = true;

    // set initial estimate
    est_longitude = longitude;
    est_latitude = latitude;
    est_r = 0;
    est_w = 0;
    est_t = t;
  }

}

void ThermalLocator::Update(double t_0, 
			    double longitude_0, double latitude_0,
			    double wind_speed, double wind_bearing,
			    double trackbearing,
			    double *Thermal_Longitude,
			    double *Thermal_Latitude,
			    double *Thermal_W,
			    double *Thermal_R) {

  if (npoints<TLOCATOR_NMIN) {
    *Thermal_R = 0;
    *Thermal_W = 0;
    return; // nothing to do.
  }

  double traildrift_lat = (latitude_0
			   -FindLatitude(latitude_0, 
					 longitude_0, 
					 wind_bearing, 
					 wind_speed));
  double traildrift_lon = (longitude_0
			   -FindLongitude(latitude_0, 
					  longitude_0, 
					  wind_bearing, 
					  wind_speed));

  // drift points (only do this once)
  Drift(t_0, longitude_0, latitude_0, traildrift_lon, traildrift_lat);

  // drift estimate from previous time step
  double dt = t_0-est_t;
  est_longitude += traildrift_lon*dt;
  est_latitude += traildrift_lat*dt;

  est_x = (est_longitude-longitude_0)*fastcosine(latitude_0);
  est_y = (est_latitude-latitude_0);

  int slogw = 0;
  int sx=0;
  int sy=0;
  int i;

  int xav=0;
  int yav=0;

  for (i=0; i<TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      xav+= points[i].xiw;
      yav+= points[i].yiw;
      slogw += points[i].iweight;
    }
  }
  xav/= slogw;
  yav/= slogw;

  // xav, yav is average glider's position

  slogw = 0;
  for (i=0; i<TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      int dx = (points[i].xiw-xav*points[i].iweight)*points[i].iw;
      int dy = (points[i].yiw-yav*points[i].iweight)*points[i].iw;
      sx += dx;
      sy += dy;
      slogw += points[i].iw*points[i].iweight;
    }
  }
  if (slogw>0) {
    sx /= slogw;
    sy /= slogw;


    int vx = iround(100*fastsine(trackbearing));
    int vy = iround(100*fastcosine(trackbearing));
    long dx = sx;
    long dy = sy;
    int mag = isqrt4((dx*dx+dy*dy)*256*256)/256;

    // find magnitude of angle error
    double g = max(-0.99,min(0.99,(dx*vx + dy*vy)/(100.0*mag)));
    double angle = acos(g)*RAD_TO_DEG-90;

#ifdef DEBUG
    char buffer[100];
    sprintf(buffer,"%d %d %d %d %d %f # centering\n",
	    sx, sy, xav, yav, mag, angle);
    DebugStore(buffer);
#endif

    est_x = (sx+xav)/(1.0*SFACT);
    est_y = (sy+yav)/(1.0*SFACT);

    est_t =  t_0;
    est_latitude = est_y+latitude_0;
    est_longitude = est_x/fastcosine(latitude_0)+longitude_0;
    
    *Thermal_Longitude = est_longitude;
    *Thermal_Latitude = est_latitude;
    *Thermal_R = 1;
    *Thermal_W = 1;    
  } else {
    *Thermal_R = 0;
    *Thermal_W = 0;    
  }

  /*
  double error = Estimate(est_x, est_y);
  error = 1.0;
  if (error>=0) {
    // solution is valid, update position of thermal center
    est_t =  t_0;
    est_latitude = est_y+latitude_0;
    est_longitude = est_x/fastcosine(latitude_0)+longitude_0;
      
    *Thermal_R = est_r;
    *Thermal_W = est_w;
    *Thermal_Longitude = est_longitude;
    *Thermal_Latitude = est_latitude;
  } else {
    
    // reset?
    est_longitude = longitude_0;
    est_latitude = latitude_0;
    
    *Thermal_R = 0;
    *Thermal_W = 0;    
  }
  */

  /*
  ////////

  int i;
  for (i=0; i<4; i++) {

    double dx = 1.0e-4; // approx 10 meters
    double dy = 1.0e-4; // approx 10 meters
    
    double e0 = Estimate(est_x, est_y);
    double ex = Estimate(est_x+dx, est_y);
    double ey = Estimate(est_x, est_y+dy);
    
    if ((e0>=0)&&(ex>=0)&&(ey>=0)) {
      double dedx = (ex-e0)/dx;
      double dedy = (ey-e0)/dy;
      double ddx = dx*2;
      double ddy = dy*2;
      est_x -= max(min(ddx,dedx),-ddx);
      est_y -= max(min(ddy,dedy),-ddy);
    }
    
    double error = Estimate(est_x, est_y);
    if (error>=0) {
      // solution is valid, update position of thermal center
      est_t =  t_0;
      est_latitude = est_y+latitude_0;
      est_longitude = est_x/fastcosine(latitude_0)+longitude_0;
      
      *Thermal_R = est_r;
      *Thermal_W = est_w;
      *Thermal_Longitude = est_longitude;
      *Thermal_Latitude = est_latitude;
    } else {
      
      // reset?
      est_longitude = longitude_0;
      est_latitude = latitude_0;
      
      *Thermal_R = 0;
      *Thermal_W = 0;

      break;
    }
  }
  */
}

/*
double ThermalLocator::Estimate(double t_x, double t_y) {
  double error = -1;

  ols.Reset();

  int i;
  for (i=0; i<TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      double dx = t_x-points[i].x;
      double dy = t_y-points[i].y;
      double ex = dx*dx+dy*dy;
      double ey = 0; // points[i].logw;
      points[i].d = ex;
      ols.least_squares_add(ex, ey, 1.0); // points[i].weight);
    }
  }
  ols.least_squares_update();

  if (ols.m<0) {
    est_r = sqrt(-1.0/ols.m);
    est_w = exp(ols.b)/10.0;
  } else {
    return -1;
  }

  error = 0;
  double sx = 0;
  for (i=0; i<TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      double w = est_w*exp(points[i].d*ols.m);
      double werr = (w-points[i].w);
      error += werr*werr;
      sx += fabs(points[i].w);
    }
  }
  sx /= npoints;
  error = sqrt(error/npoints)/sx;

  // Model: w = W exp (-D^2/R^2)
  // take log
  //  ln(w)= ln(W) - D^2. (R^-2)
  // this is of form
  // y = m.x + c
  // with m = -R^-2     sqrt(-1/m) = R
  //      c = ln(W)
  //      x = D^2
  //      y = ln(w)
  
  return error;
}
*/

void ThermalLocator::Drift(double t_0, 
			   double longitude_0, double latitude_0,
			   double wind_lon, double wind_lat) {


  for (int i=0; i<TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      points[i].Drift(t_0, longitude_0, latitude_0, wind_lon, wind_lat);
    }
  }
}

#include "Calculations.h"




void ThermalLocator::EstimateThermalBase(double Thermal_Longitude,
					 double Thermal_Latitude,
					 double altitude,
					 double wthermal,
					 double wind_speed, 
					 double wind_bearing,
					 double *ground_longitude,
					 double *ground_latitude,
					 double *ground_alt) {

  if ((Thermal_Longitude == 0.0)||(Thermal_Latitude==0.0)||(wthermal<1.0)) {
    *ground_longitude = 0.0;
    *ground_latitude = 0.0;
    *ground_alt = -1.0;
    return;
  }

  double Tmax;
  Tmax = (altitude/wthermal);
  double dt = Tmax/10;

  LockTerrainDataCalculations();

  double lat = FindLatitude(Thermal_Latitude, Thermal_Longitude, 
			    wind_bearing, 
			    wind_speed*dt);
  double lon = FindLongitude(Thermal_Latitude, Thermal_Longitude, 
			     wind_bearing, 
			     wind_speed*dt);
  double Xrounding = fabs(lon-Thermal_Longitude)/2;
  double Yrounding = fabs(lat-Thermal_Latitude)/2;
  terrain_dem_calculations.SetTerrainRounding(Xrounding, Yrounding);

  double latlast = lat;
  double lonlast = lon;
  double hground;

  for (double t = 0; t<=Tmax; t+= dt) {

    lat = FindLatitude(Thermal_Latitude, Thermal_Longitude, 
		       wind_bearing, 
		       wind_speed*t);
    lon = FindLongitude(Thermal_Latitude, Thermal_Longitude, 
			wind_bearing, 
			wind_speed*t);

    double hthermal = altitude-wthermal*t;
    hground = terrain_dem_calculations.GetTerrainHeight(lat, lon);
    double dh = hthermal-hground;
    if (dh<0) {
      t = t+dh/wthermal;
      lat = FindLatitude(Thermal_Latitude, Thermal_Longitude, 
			 wind_bearing, 
			 wind_speed*t);
      lon = FindLongitude(Thermal_Latitude, Thermal_Longitude, 
			  wind_bearing, 
			  wind_speed*t);
      break;
    }
  }
  UnlockTerrainDataCalculations();

  hground = terrain_dem_calculations.GetTerrainHeight(lat, lon);

  *ground_longitude = lon;
  *ground_latitude = lat;
  *ground_alt = hground;

}
