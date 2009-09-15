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

#include "WayPointList.hpp"

#include <assert.h>
#include <stdlib.h>

WayPointList::WayPointList()
  :list(NULL), calc_list(NULL), count(0) {}

WayPointList::~WayPointList()
{
  clear();
}

void
WayPointList::clear()
{
  for (unsigned i = 0; i < count; ++i)
    free(list[i].Details);

  count = 0;

  if (list != NULL) {
    ::LocalFree((HLOCAL)list);
    list = NULL;
  }

  if (calc_list != NULL) {
    ::LocalFree((HLOCAL)calc_list);
    calc_list = NULL;
  }
}

static void
initialize(WPCALC &wpcalc)
{
  wpcalc.Preferred = false;
  wpcalc.Distance=-1;
  wpcalc.AltArrival=-1;
  wpcalc.AltReqd=-1;
  wpcalc.Bearing=-1;
  wpcalc.GR=-1;
  wpcalc.VGR=-1;
}

WAYPOINT *
WayPointList::append()
{
  if (count % 50 == 0 && !grow())
    return NULL;

  memset(&list[count], 0, sizeof(list[count]));
  initialize(calc_list[count]);
  return &list[count++];
}

int
WayPointList::append(const WAYPOINT &way_point)
{
  WAYPOINT *dest = append();
  if (dest == NULL)
    return -1;

  *dest = way_point;
  return dest->Number = dest - list;
}

bool
WayPointList::grow()
{
  WAYPOINT *new_list;
  WPCALC *new_calc_list;

  new_list = (WAYPOINT *)realloc(list, (count + 50) * sizeof(list[0]));
  if (new_list == NULL)
    /* out of memory */
    return false;

  list = new_list;

  new_calc_list = (WPCALC *)realloc(calc_list, (count + 50) * sizeof(calc_list[0]));
  if (new_calc_list == NULL)
    /* out of memory */
    return false;

  calc_list = new_calc_list;

  return true;
}

void
WayPointList::pop()
{
  assert(count > 0);

  free(list[--count].Details);
}
