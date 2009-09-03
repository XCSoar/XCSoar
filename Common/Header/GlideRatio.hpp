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

#ifndef XCSOAR_GLIDE_RATIO_HPP
#define XCSOAR_GLIDE_RATIO_HPP

#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"
#include "Defines.h"

typedef enum{
	ae15seconds=0,
	ae30seconds,
	ae60seconds,
	ae90seconds,
	ae2minutes,
	ae3minutes,
} AverEffTime_t;


typedef struct {
        int     distance[MAXLDROTARYSIZE]; // rotary array with a predefined max capacity
        int     altitude[MAXLDROTARYSIZE];
	int	totaldistance;
        short   start;          // pointer to current first item in rotarybuf if used
        short   size;           // real size of rotary buffer (0-size)
	bool	valid;
} ldrotary_s;

void InitLDRotary(ldrotary_s *buf);
void	InsertLDRotary(DERIVED_INFO *Calculated, ldrotary_s *buf, int distance, int altitude);
int	CalculateLDRotary(DERIVED_INFO *Calculated, ldrotary_s *buf);

// limit to reasonable values
double LimitLD(double LD);
// methods using low-pass filter

double UpdateLD(double LD, double d, double h, double filter_factor);


#endif
