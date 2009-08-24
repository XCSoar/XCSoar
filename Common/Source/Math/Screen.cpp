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

#include "Math/Screen.hpp"
#include "Math/Geometry.hpp"
#include "Math/FastMath.h"
#include "Utils.h"
#include "InfoBoxLayout.h"
#include "MapWindow.h"

#include <math.h>

#ifndef RAD_TO_DEG
#define RAD_TO_DEG 57.2957795131
#endif

void protate(POINT &pin, const double &angle)
{
  int x= pin.x;
  int y= pin.y;
  static double lastangle = 0;
  static int cost=1024,sint=0;

  if(angle != lastangle)
    {
      lastangle = angle;
      cost = ifastcosine(angle);
      sint = ifastsine(angle);
    }
  pin.x = (x*cost - y*sint + 512 )/1024;
  pin.y = (y*cost + x*sint + 512 )/1024;

  // round (x/b) = (x+b/2)/b;
  // b = 2; x = 10 -> (10+1)/2=5
  // b = 2; x = 11 -> (11+1)/2=6
  // b = 2; x = -10 -> (-10+1)/2=4
}

void protateshift(POINT &pin, const double &angle,
                  const int &xs, const int &ys)
{
  int x= pin.x;
  int y= pin.y;
  static double lastangle = 0;
  static int cost=1024,sint=0;

  if(angle != lastangle)
    {
      lastangle = angle;
      cost = ifastcosine(angle);
      sint = ifastsine(angle);
    }
  pin.x = (x*cost - y*sint + 512 + (xs*1024))/1024;
  pin.y = (y*cost + x*sint + 512 + (ys*1024))/1024;

}

double ScreenAngle(int x1, int y1, int x2, int y2)
{
  return atan2((double)y2-y1, (double)x2-x1)*RAD_TO_DEG;
}

void ScreenClosestPoint(const POINT &p1, const POINT &p2,
                        const POINT &p3, POINT *p4, int offset)
{
  int v12x, v12y, v13x, v13y;

  v12x = p2.x-p1.x; v12y = p2.y-p1.y;
  v13x = p3.x-p1.x; v13y = p3.y-p1.y;

  int mag12 = isqrt4(v12x*v12x+v12y*v12y);
  if (mag12>1) {
    // projection of v13 along v12 = v12.v13/|v12|
    int proj = (v12x*v13x+v12y*v13y)/mag12;
    // fractional distance
    double f;
    if (offset>0) {
      if (offset*2<mag12) {
        proj = max(0, min(proj, mag12));
        proj = max(offset, min(mag12-offset, proj+offset));
      } else {
        proj = mag12/2;
      }
    }
    f = min(1.0,max(0.0,(double)proj/mag12));

    // location of 'closest' point
    p4->x = lround(v12x*f)+p1.x;
    p4->y = lround(v12y*f)+p1.y;
  } else {
    p4->x = p1.x;
    p4->y = p1.y;
  }
}

void PolygonRotateShift(POINT* poly, const int n, const int xs, const int ys,
			const double angle) {
  static double lastangle = -1;
  static int cost=1024, sint=0;

  if(angle != lastangle) {
    lastangle = angle;
    int deg = DEG_TO_INT(AngleLimit360(angle));
    cost = ICOSTABLE[deg]*InfoBoxLayout::scale;
    sint = ISINETABLE[deg]*InfoBoxLayout::scale;
  }
  const int xxs = xs*1024+512;
  const int yys = ys*1024+512;
  POINT *p = poly;
  const POINT *pe = poly+n;

  while (p<pe) {
    int x= p->x;
    int y= p->y;
    p->x = (x*cost - y*sint + xxs)/1024;
    p->y = (y*cost + x*sint + yys)/1024;
    p++;
  }
}

BOOL PolygonVisible(const POINT *lpPoints, int nCount, RECT rc)
{
  const RECT MapRect = MapWindow::GetMapRect();
  BOOL Sector[9] = {FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE};
  int i;
  int Count = 0;
  (void)rc;
  //return TRUE;

  for(i=0;i<nCount;i++)
    {
      if(lpPoints[i].y < MapRect.top)
	{
	  if(lpPoints[i].x < MapRect.left)
	    {
	      Sector[0] = TRUE;
	    }
	  else if((lpPoints[i].x >=MapRect.left)
		  && (lpPoints[i].x <MapRect.right))
	    {
	      Sector[1] = TRUE;
	    }
	  else if(lpPoints[i].x >=MapRect.right)
	    {
	      Sector[2] = TRUE;
	    }
	}
      else if((lpPoints[i].y >=MapRect.top)
	      && (lpPoints[i].y <MapRect.bottom))
	{
	  if(lpPoints[i].x <MapRect.left)
	    {
	      Sector[3] = TRUE;
	    }
	  else if((lpPoints[i].x >=MapRect.left)
		  && (lpPoints[i].x <MapRect.right))
	    {
	      Sector[4] = TRUE;
	      return TRUE;
	    }
	  else if(lpPoints[i].x >=MapRect.right)
	    {
	      Sector[5] = TRUE;
	    }
	}
      else if(lpPoints[i].y >=MapRect.bottom)
	{
	  if(lpPoints[i].x <MapRect.left)
	    {
	      Sector[6] = TRUE;
	    }
	  else if((lpPoints[i].x >=MapRect.left)
		  && (lpPoints[i].x <MapRect.right))
	    {
	      Sector[7] = TRUE;
	    }
	  else if(lpPoints[i].x >=MapRect.right)
	    {
	      Sector[8] = TRUE;
	    }
	}
    }

  for(i=0;i<9;i++)
    {
      if(Sector[i])
	{
	  Count ++;
	}
    }

  if(Count>= 2)
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}


bool CheckRectOverlap(RECT rc1, RECT rc2) {
  if(rc1.left >= rc2.right) return(false);
  if(rc1.right <= rc2.left) return(false);
  if(rc1.top >= rc2.bottom) return(false);
  if(rc1.bottom <= rc2.top) return(false);
  return(true);
}


