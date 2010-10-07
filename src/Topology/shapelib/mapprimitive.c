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
#include "maperror.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

void msFree(void *p) {
  free(p);
}

/*
** Free memory allocated for a character array
*/
void msFreeCharArray(char **array, int num_items)
{
  int i;

  if((num_items < 0) || !array) return;

  for(i=0;i<num_items;i++)
    free(array[i]);
  msFree(array);

  return;
}

void msInitShape(shapeObj *shape)
{
  // spatial component
  shape->line = NULL;
  shape->numlines = 0;
  shape->type = MS_SHAPE_NULL;
  shape->bounds.minx = shape->bounds.miny = -1;
  shape->bounds.maxx = shape->bounds.maxy = -1;

  // attribute component
  shape->values = NULL;
  shape->numvalues = 0;

  // annotation component
  shape->text = NULL;

  // bookkeeping component
  shape->classindex = 0; // default class
  shape->tileindex = shape->index = -1;
}

void msFreeShape(shapeObj *shape)
{
  int c;

  for (c= 0; c < shape->numlines; c++)
    free(shape->line[c].point);
  free(shape->line);

  if(shape->values) msFreeCharArray(shape->values, shape->numvalues);
  if(shape->text) free(shape->text);

  msInitShape(shape); // now reset
}
