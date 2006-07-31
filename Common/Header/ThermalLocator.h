#ifndef THERMALLOCATOR_H
#define THERMALLOCATOR_H

#define TLOCATOR_NMIN 5
#define TLOCATOR_NMAX 100

#include "leastsqs.h"

class ThermalLocator_Point {
 public:
  ThermalLocator_Point() {
    valid = false;
  }
  double latitude;
  double longitude;
  double t;
  double w;
  //  double logw;
  double d;
  bool valid;
  double weight;

  void Drift(double t_0,
	     double longitude_0, double latitude_0,
	     double wind_lon, double wind_lat);
  double x;
  double y;
  int xiw;
  int yiw;
  int iweight;
  int iw;
};

class ThermalLocator {
 public:
  ThermalLocator();

  void Reset();
  void AddPoint(double t, double longitude, double latitude, double w);
  void Update(double t_0, 
	      double longitude_0, double latitude_0,
	      double wind_speed, double wind_bearing,
	      double trackbearing,
	      double *Thermal_Longitude,
	      double *Thermal_Latitude,
	      double *Thermal_W,
	      double *Thermal_R);
  //  double Estimate(double t_x, double t_y);

  void EstimateThermalBase(double Thermal_Longitude,
			   double Thermal_Latitude,
			   double altitude,
			   double wthermal,
			   double wind_speed, 
			   double wind_bearing,
			   double *ground_longitude,
			   double *ground_latitude,
			   double *ground_alt);
  double est_x;
  double est_y;
  double est_w;
  double est_r;
  double est_t;
  double est_latitude;
  double est_longitude;
 private:
  void Drift(double t_0, 
	     double longitude_0, double latitude_0,
	     double wind_lon, double wind_lat);
  ThermalLocator_Point points[TLOCATOR_NMAX]; 
  LeastSquares ols;
  bool initialised;
  int nindex;
  int npoints;

};

#endif
