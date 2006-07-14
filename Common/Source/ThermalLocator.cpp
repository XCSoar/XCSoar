#include "stdafx.h"
#include "XCSoar.h"
#include "ThermalLocator.h"
#include "Utils.h"
#include <math.h>

int EnableThermalLocator = 1;


void ThermalLocator_Point::Drift(double t_0,
				 double longitude_0, double latitude_0,
				 double drift_lon, double drift_lat) {

  // convert to flat earth coordinates, then drift by wind and delta t  
  double dt = t_0-t;
  weight = 1.0/(exp(-3.0*dt/TLOCATOR_NMAX));
  x = (longitude+drift_lon*dt-longitude_0)*fastcosine(latitude_0);
  y = (latitude+drift_lat*dt-latitude_0);
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
  points[nindex].logw = log(max(w,0.1)*10.0);
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

  double slogw = 0;
  double sx=0;
  double sy=0;
  int i;
  for (i=0; i<TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      double wthis = (points[i].w)*points[i].weight;
      double dx = points[i].x*wthis;
      double dy = points[i].y*wthis;
      sx += dx;
      sy += dy;
      slogw += wthis;
    }
  }
  if (slogw>0) {
    sx /= slogw;
    sy /= slogw;
    
    est_x = sx;
    est_y = sy;
    
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


double ThermalLocator::Estimate(double t_x, double t_y) {
  double error = -1;

  ols.Reset();

  int i;
  for (i=0; i<TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      double dx = t_x-points[i].x;
      double dy = t_y-points[i].y;
      double ex = dx*dx+dy*dy;
      double ey = points[i].logw;
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


void ThermalLocator::Drift(double t_0, 
			   double longitude_0, double latitude_0,
			   double wind_lon, double wind_lat) {


  for (int i=0; i<TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      points[i].Drift(t_0, longitude_0, latitude_0, wind_lon, wind_lat);
    }
  }
}

