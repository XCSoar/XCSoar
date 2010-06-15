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

#ifndef XCSOAR_GLIDE_RATIO_HPP
#define XCSOAR_GLIDE_RATIO_HPP

#include "Util/NonCopyable.hpp"

struct SETTINGS_COMPUTER;

typedef enum {
  ae15seconds,
  ae30seconds,
  ae60seconds,
  ae90seconds,
  ae2minutes,
  ae3minutes,
} AverEffTime_t;

class GlideRatioCalculator : private NonCopyable {
  struct record {
    unsigned distance;
    int altitude;
  };

  /**
   * Rotary array with a predefined max capacity.
   */
  record records[180];

  unsigned totaldistance;

  /**
   * Pointer to current first item in rotarybuf if used.
   */
  unsigned short start;

  /**
   * Real size of rotary buffer.
   */
  unsigned short size;

  bool valid;

public:
  void init(const SETTINGS_COMPUTER &settings);
  void add(unsigned distance, int altitude);
  int calculate() const;
};

// methods using low-pass filter

double UpdateLD(double LD, double d, double h, double filter_factor);


#endif
