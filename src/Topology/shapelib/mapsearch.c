/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "mapprimitive.h"

#include <math.h>

#define LASTVERT(v,n)  ((v) == 0 ? n-2 : v-1)
#define NEXTVERT(v,n)  ((v) == n-2 ? 0 : v+1)

/*
** Returns MS_TRUE if rectangles a and b overlap
*/
int msRectOverlap(const rectObj *a, const rectObj *b)
{
  if(a->minx > b->maxx) return(MS_FALSE);
  if(a->maxx < b->minx) return(MS_FALSE);
  if(a->miny > b->maxy) return(MS_FALSE);
  if(a->maxy < b->miny) return(MS_FALSE);
  return(MS_TRUE);
}

/*
** Returns MS_TRUE if rectangle a is contained in rectangle b
*/
int msRectContained(const rectObj *a, const rectObj *b)
{
  if(a->minx >= b->minx && a->maxx <= b->maxx)
    if(a->miny >= b->miny && a->maxy <= b->maxy)
      return(MS_TRUE);
  return(MS_FALSE);
}

int msPointInRect(const pointObj *p, const rectObj *rect)
{
  if(p->x < rect->minx) return(MS_FALSE);
  if(p->x > rect->maxx) return(MS_FALSE);
  if(p->y < rect->miny) return(MS_FALSE);
  if(p->y > rect->maxy) return(MS_FALSE);
  return(MS_TRUE);
}
