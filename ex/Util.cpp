#include <math.h>
#include "Util.h"

double Bearing(const GEOPOINT& p1, const GEOPOINT& p2) {
  return AngleLimit360(atan2(p2.Longitude-p1.Longitude,p2.Latitude-p1.Latitude)
                       *180.0/3.1415926);
}

#define sqr(x) ((x)*(x))

int count_distance=0;

double Distance(const GEOPOINT& p1, const GEOPOINT& p2) {
  count_distance++;
  return sqrt(sqr(p1.Latitude-p2.Latitude)+sqr(p1.Longitude-p2.Longitude));
}

double ProjectedDistance(const GEOPOINT& p1, const GEOPOINT& p2,
  const GEOPOINT& p3) {
  return Distance(p1,p3);
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


unsigned int isqrt4(unsigned long val) {
  unsigned int temp, g=0;

  if (val >= 0x40000000) {
    g = 0x8000;
    val -= 0x40000000;
  }

#define INNER_MBGSQRT(s)                      \
  temp = (g << (s)) + (1 << ((s) * 2 - 2));   \
  if (val >= temp) {                          \
    g += 1 << ((s)-1);                        \
    val -= temp;                              \
  }

  INNER_MBGSQRT (15)
  INNER_MBGSQRT (14)
  INNER_MBGSQRT (13)
  INNER_MBGSQRT (12)
  INNER_MBGSQRT (11)
  INNER_MBGSQRT (10)
  INNER_MBGSQRT ( 9)
  INNER_MBGSQRT ( 8)
  INNER_MBGSQRT ( 7)
  INNER_MBGSQRT ( 6)
  INNER_MBGSQRT ( 5)
  INNER_MBGSQRT ( 4)
  INNER_MBGSQRT ( 3)
  INNER_MBGSQRT ( 2)

#undef INNER_MBGSQRT
  temp = g+g+1;
  if (val >= temp) g++;
  return g;
}


GEOPOINT InterpolateLocation(const GEOPOINT& p1,
                             const GEOPOINT& p2, 
                             const double t) 
{
  GEOPOINT p = p1;
  p.Longitude += t*(p2.Longitude-p1.Longitude);
  p.Latitude += t*(p2.Latitude-p1.Latitude);
  return p;
}
