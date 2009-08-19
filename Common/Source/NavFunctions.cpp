/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

#include "StdAfx.h"
#include "NavFunctions.h"
#include "Compatibility/math.h"

void xXY_to_LL(double Lat_TP, double Long_TP, double X_int, double Y_int, double *Lat, double *Long)
{
   double X, Y, Ynp, Ysp;
   double Sin_Lat_TP, Cos_Lat_TP;
   double Sin_DLng, Cos_DLng;
   double Lat_Num, Lat_Denum;
   double Long_Num, Long_Denum;
   double Temp;
   double   Delta_Long;
 //  long   X_Own = 0;
 //  long   Y_Own = 0;

   /* Adjust the X,Y coordinate's according to the "global" X,Y    */
   /* coordinate system.                                           */

   //xrTP_Position(&X_Own, &Y_Own);
   X = (double) (X_int);
   Y = (double) (Y_int);
   if ( Lat_TP != 90 ) {

      /* Non polar projection */

      if ( X != 0.0 )
	  {

         Cos_Lat_TP = cos((double) DEG_TO_RAD * (double) Lat_TP);
         Sin_Lat_TP = sin((double) DEG_TO_RAD * (double) Lat_TP);

         Temp = EARTH_DIAMETER * Cos_Lat_TP;
         Ynp =  Temp/(1.0 + Sin_Lat_TP);
         Ysp = -Temp/(1.0 - Sin_Lat_TP);

         Long_Num = X * (Ynp - Ysp);
         Long_Denum = (Ynp - Y)*(Y - Ysp) - X*X;
         Delta_Long = (double)(RAD_TO_DEG * atan2(Long_Num, Long_Denum));
         *Long = Long_TP + Delta_Long;

         Cos_DLng = cos((double) DEG_TO_RAD * (double) Delta_Long);
         Sin_DLng = sin((double) DEG_TO_RAD * (double) Delta_Long);
         Lat_Num   = Y*Sin_DLng + X*Sin_Lat_TP*Cos_DLng;
         Lat_Denum = X*Cos_Lat_TP;

         *Lat  = (double)(RAD_TO_DEG * atan2(Lat_Num, Lat_Denum));
         if ( *Lat > 90 ) {
               *Lat -= 180;
         }
         else {
            if ( *Lat < -90 ) {
               *Lat += 180;
            }
         }
      }
      else {
         *Lat  = Lat_TP + (double)(RAD_TO_DEG * atan2(Y, EARTH_DIAMETER) * 2);
         *Long = Long_TP;
      }
   }
   else {

      /* Polar projection     */

      Delta_Long = (double)(RAD_TO_DEG * atan2(-X, Y));

      Lat_Num = X*X + Y*Y;
      Lat_Denum = SQUARED_EARTH_DIAMETER;

      *Lat = 90 -(double)(RAD_TO_DEG * atan2(Lat_Num, Lat_Denum) * 2);
      *Long = Long_TP + Delta_Long;
   }
}


void xLL_to_XY(double Lat_TP, double Long_TP, double Lat_Pnt, double Long_Pnt, double *X, double *Y)
{
   double       Delta_Long;
   double     sin_TP_Lat;
   double     cos_TP_Lat;
   double    sin_Lat_Pnt;
   double    cos_Lat_Pnt;
   double sin_Delta_Long;
   double cos_Delta_Long;
   double          Denom;



   Delta_Long = Long_Pnt - Long_TP;

   sin_TP_Lat = sin((double)(Lat_TP * DEG_TO_RAD));
   cos_TP_Lat = cos((double)(Lat_TP * DEG_TO_RAD));
   sin_Lat_Pnt = sin((double)(Lat_Pnt * DEG_TO_RAD));
   cos_Lat_Pnt = cos((double)(Lat_Pnt * DEG_TO_RAD));
   sin_Delta_Long = sin((double)(Delta_Long * DEG_TO_RAD));
   cos_Delta_Long = cos((double)(Delta_Long * DEG_TO_RAD));

   Denom = 1.0 + sin_TP_Lat*sin_Lat_Pnt + cos_TP_Lat*cos_Lat_Pnt*cos_Delta_Long;

  // xrTP_Position(&X_Own, &Y_Own);
   *X = (double) (((EARTH_DIAMETER*cos_Lat_Pnt*sin_Delta_Long)/Denom));

   *Y = (double) ((((EARTH_DIAMETER*(cos_TP_Lat*sin_Lat_Pnt -
                   sin_TP_Lat*cos_Lat_Pnt*cos_Delta_Long))/Denom)));
}


void xXY_Brg_Rng(double X_1, double Y_1, double X_2, double Y_2, double *Bearing, double *Range)
{
  double  Rad_Bearing;
  double Rad_360 = (2 * PI);

  double y = (X_2 - X_1);
  double x = (Y_2 - Y_1);

  if (fabs(x)>0.00000001 && fabs(y)>0.00000001){
    Rad_Bearing = atan2(y, x);
  } else {
    Rad_Bearing = 0;
  }

  if (Rad_Bearing < 0) {
    Rad_Bearing += Rad_360;
  }
  *Bearing = (double)(RAD_TO_DEG * Rad_Bearing);
  *Range = (double) (_hypot((double) (X_2 - X_1), (double) (Y_2 - Y_1)));
}

void xBrg_Rng_XY(double X_RefPos, double Y_RefPos, double Bearing, double Range, double *X, double *Y)
{
  (void)X_RefPos;
  (void)Y_RefPos;


double V = Bearing / RAD_TO_DEG;

	*X = (double) ( (sin(V) * (double) Range) + 0.5 );
	*Y = (double) ( (cos(V) * (double) Range) + 0.5 );

}




void xCrs_Spd_to_VxVy(double Crs, double Spd, double *Vx, double *Vy)
{
  double Crs_rad;
  double   Tmp_Vx, Tmp_Vy;

  Crs_rad = DEG_TO_RAD * (double) Crs;

  Tmp_Vx = (double) ( (sin(Crs_rad) * (double) Spd) + 0.5 );
  Tmp_Vy = (double) ( (cos(Crs_rad) * (double) Spd) + 0.5 );
  *Vx = Tmp_Vx;
  *Vy = Tmp_Vy;
}


void xVxVy_to_Crs_Spd(double Vx, double Vy, double *Crs, double *Spd)
{
  double Tmp_Spd;

  *Crs = ((double) ((RAD_TO_DEG * 0.5 * atan2((double) Vx, (double) Vy))+ 0.5)) * 0.5;

  Tmp_Spd = (double) (sqrt(((double) Vx * (double) Vx) + ((double) Vy * (double) Vy)) + 0.5);
  *Spd = Tmp_Spd;
}


void LL_to_BearRange(double Lat_TP, double Long_TP, double Lat_Pnt, double Long_Pnt, double *Bearing, double *Range)
{
	double pos_X, pos_Y;

	xLL_to_XY(Lat_TP,
		Long_TP,
		Lat_Pnt,
		Long_Pnt,
		&pos_X, &pos_Y);

	double bea;
	double ran;
	xXY_Brg_Rng(0,0,pos_X, pos_Y, &bea, &ran);
	*Bearing = bea;
	*Range = ran;
}
