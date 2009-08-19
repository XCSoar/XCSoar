#include "Math/Geometry.hpp"
#include "Math/FastMath.h"

#include <math.h>

void rotate(double &xin, double &yin, const double &angle)
{
  double x= xin;
  double y= yin;
  static double lastangle = 0;
  static double cost=1,sint=0;

  if(angle != lastangle)
    {
      lastangle = angle;
      cost = (double)fastcosine(angle);
      sint = (double)fastsine(angle);
    }
  xin = x*cost - y*sint;
  yin = y*cost + x*sint;
}

void frotate(float &xin, float &yin, const float &angle)
{
  float x= xin;
  float y= yin;
  static float lastangle = 0;
  static float cost=1,sint=0;

  if(angle != lastangle)
    {
      lastangle = angle;
      cost = (float)fastcosine(angle);
      sint = (float)fastsine(angle);
    }
  xin = x*cost - y*sint;
  yin = y*cost + x*sint;
}


void irotatescale(int &xin, int &yin, const double &angle,
                  const double &scale, double &x, double &y)
{
  static double lastangle = 0;
  static double lastscale = 0;
  static int cost=1024,sint=0;
  if((angle != lastangle)||(scale != lastscale))
    {
      lastscale = scale/1024;
      lastangle = angle;
      cost = ifastcosine(angle);
      sint = ifastsine(angle);
    }
  x = (xin*cost - yin*sint + 512)*lastscale;
  y = (yin*cost + xin*sint + 512)*lastscale;
}


void irotate(int &xin, int &yin, const double &angle)
{
  int x= xin;
  int y= yin;
  static double lastangle = 0;
  static int cost=1024,sint=0;

  if(angle != lastangle)
    {
      lastangle = angle;
      cost = ifastcosine(angle);
      sint = ifastsine(angle);
    }
  xin = (x*cost - y*sint + 512)/1024;
  yin = (y*cost + x*sint + 512)/1024;
}


void rotatescale(double &xin, double &yin,
                 const double &angle, const double &scale)
{
  double x= xin;
  double y= yin;
  static double lastangle = 0;
  static double lastscale = 0;
  static double cost=1,sint=0;

  if((angle != lastangle)||(scale != lastscale))
    {
      lastangle = angle;
      lastscale = scale;
      cost = (double)fastcosine(angle)*scale;
      sint = (double)fastsine(angle)*scale;
    }
  xin = x*cost - y*sint;
  yin = y*cost + x*sint;
}


void frotatescale(float &xin, float &yin, const float &angle, const float &scale)
{
  float x= xin;
  float y= yin;
  static float lastangle = 0;
  static float lastscale = 0;
  static float cost=1,sint=0;

  if((angle != lastangle)||(scale != lastscale))
    {
      lastangle = angle;
      lastscale = scale;
      cost = (float)fastcosine(angle)*scale;
      sint = (float)fastsine(angle)*scale;
    }
  xin = x*cost - y*sint;
  yin = y*cost + x*sint;
}

double AngleLimit360(double theta) {
  while (theta>=360.0) {
    theta-= 360.0;
  }
  while (theta<0.0) {
    theta+= 360.0;
  }
  return theta;
}

double AngleLimit180(double theta) {
  while (theta>180.0) {
    theta-= 360.0;
  }
  while (theta<-180.0) {
    theta+= 360.0;
  }
  return theta;
}

double Reciprocal(double InBound)
{
  return AngleLimit360(InBound+180);
}

bool AngleInRange(double Angle0, double Angle1, double x, bool is_signed) {
  Angle0 = AngleLimit360(Angle0);
  Angle1 = AngleLimit360(Angle1);
  x = AngleLimit360(x);

  if (Angle1>= Angle0) {
    if ((x>=Angle0) && (x<= Angle1)) {
      return true;
    }
  } else {
    if (is_signed) {
      if ((x>=Angle0) || (x<= Angle1)) {
        return true;
      }
    } else {
      if ((x<=Angle0) || (x>= Angle1)) {
        return true;
      }
    }
  }
  return false;
}

// Use only for AAT bisector calculations!
double HalfAngle(double Angle0, double Angle1) {
  Angle0 = AngleLimit360(Angle0);
  Angle1 = AngleLimit360(Angle1);

  // TODO code: check/test this? thankfully only occurs in one spot in AAT
  if (Angle1>= Angle0) {
    return (Angle0+Angle1)/2;
  } else {
    return (Angle0+Angle1+360)/2;
  }
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
