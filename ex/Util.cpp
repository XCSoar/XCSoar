#include <math.h>
#include "Util.h"

double Bearing(const GEOPOINT& p1, const GEOPOINT& p2) {
  return atan2(p1.Longitude-p2.Longitude,p1.Latitude-p2.Latitude)
    *180.0/3.1415926;
}

#define sqr(x) ((x)*(x))

int count_distance=0;

double Distance(const GEOPOINT& p1, const GEOPOINT& p2) {
  count_distance++;
  return sqrt(sqr(p1.Latitude-p2.Latitude)+sqr(p1.Longitude-p2.Longitude));
}

double ProjectedDistance(const GEOPOINT& p1, const GEOPOINT& p2,
  const GEOPOINT& p3) {
  Distance(p1,p3);
};

double AngleLimit360(double theta) {
  while (theta>=360.0) {
    theta-= 360.0;
  }
  while (theta<0.0) {
    theta+= 360.0;
  }
  return theta;
}

double Reciprocal(double InBound)
{
  return AngleLimit360(InBound+180);
}


GEOPOINT FindLocation(const GEOPOINT& p1, 
                      double bearing, 
                      double distance) 
{
  // TODO
  return p1;
}

double HalfAngle(double a, double b) {
  return (a+b)/2;
}


double BiSector(double InBound, double OutBound)
{
  double result;

  InBound = Reciprocal(InBound);

  if(InBound == OutBound)
    {
      result = Reciprocal(InBound);
    }

  else if (InBound > OutBound)
    {
      if( (InBound - OutBound) < 180)
	{
	  result = Reciprocal((InBound+OutBound)/2);
	}
      else
	{
	  result = (InBound+OutBound)/2;
	}
    }
  else
    {
      if( (OutBound - InBound) < 180)
	{
	  result = Reciprocal((InBound+OutBound)/2);
	}
      else
	{
	  result = (InBound+OutBound)/2;
	}
    }
  return result;
}
