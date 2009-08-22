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

#include "Math/FastMath.h"

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.0174532925199432958
#define RAD_TO_DEG 57.2957795131
#endif

double COSTABLE[4096];
double SINETABLE[4096];
double INVCOSINETABLE[4096];
int ISINETABLE[4096];
int ICOSTABLE[4096];

void InitSineTable(void)
{
  int i;
  double angle;
  double cosa, sina;

  for(i=0;i<4096; i++)
    {
      angle = DEG_TO_RAD*((double)i*360)/4096;
      cosa = cos(angle);
      sina = sin(angle);
      SINETABLE[i] = sina;
      COSTABLE[i] = cosa;
      ISINETABLE[i] = iround(sina*1024);
      ICOSTABLE[i] = iround(cosa*1024);
      if ((cosa>0) && (cosa<1.0e-8)) {
        cosa = 1.0e-8;
      }
      if ((cosa<0) && (cosa>-1.0e-8)) {
        cosa = -1.0e-8;
      }
      INVCOSINETABLE[i] = 1.0/cosa;
    }
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

// http://www.azillionmonkeys.com/qed/sqroot.html


