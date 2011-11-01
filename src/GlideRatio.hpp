/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Math/fixed.hpp"
#include "Compiler.h"

static const unsigned INVALID_GR = 999;

struct SETTINGS_COMPUTER;

class GlideRatioCalculator : private NonCopyable {
  struct Record {
    unsigned distance;
    int altitude;
  };

  /**
   * Rotary array with a predefined max capacity.
   */
  Record records[180];

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
  void Initialize(const SETTINGS_COMPUTER &settings);
  void Add(unsigned distance, int altitude);
  int Calculate() const;
};

// methods using low-pass filter

gcc_const
fixed
UpdateLD(fixed LD, fixed d, fixed h, fixed filter_factor);

#endif
