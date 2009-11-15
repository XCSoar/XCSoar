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

#include "Math/Geometry.hpp"
#include "Math/FastMath.h"

#include <math.h>

/**
 * Rotates the point (xin, yin) by angle degrees around (0, 0)
 *
 * double based
 * @param xin X value
 * @param yin Y value
 * @param angle Rotation angle
 */
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

/**
 * Rotates the point (xin, yin) by angle degrees around (0, 0)
 *
 * float based
 * @param xin X value
 * @param yin Y value
 * @param angle Rotation angle
 */
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

/**
 * Rotates the point (xin, yin) by angle degrees around (0, 0)
 *
 * int based (angle as double)
 * @param xin X value
 * @param yin Y value
 * @param angle Rotation angle
 */
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

/**
 * Limits the angle (theta) to 0 - 360 degrees
 * @param theta Input angle
 * @return Output angle (0-360 degrees)
 */
double AngleLimit360(double theta) {
  while (theta>=360.0) {
    theta-= 360.0;
  }
  while (theta<0.0) {
    theta+= 360.0;
  }
  return theta;
}

/**
 * Limits the angle (theta) to -180 - +180 degrees
 * @param theta Input angle
 * @return Output angle (-180 - +180 degrees)
 */
double AngleLimit180(double theta) {
  while (theta>180.0) {
    theta-= 360.0;
  }
  while (theta<-180.0) {
    theta+= 360.0;
  }
  return theta;
}

/**
 * Rotate angle by 180 degrees and limit to 0 - 360 degrees
 * @param InBound Input angle
 * @return Output angle (0 - 360 degrees)
 */
double Reciprocal(double InBound)
{
  return AngleLimit360(InBound+180);
}

/**
 * Detects if angle (x) is between two other angles (Angle0 and Angle1)
 * @param Angle0 Limit angle 1
 * @param Angle1 Limit angle 2
 * @param x Input anlge
 * @param is_signed Is the input angle signed?
 * @return True if between, False if not
 */
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

/**
 * Calculates the HalfAngle between the to given angles
 *
 * \todo
 * - Check calculations in all usage scenarios
 *
 * !! Use only for AAT bisector calculations !!
 * @param Angle0 Input angle 1
 * @param Angle1 Input angle 2
 * @return The HalfAngle
 */
double HalfAngle(double Angle0, double Angle1) {
  Angle0 = AngleLimit360(Angle0);
  Angle1 = AngleLimit360(Angle1);

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

