/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_DITHER_HPP
#define XCSOAR_SCREEN_DITHER_HPP

#include "Compiler.h"
#include <stddef.h>
#include <stdint.h>

class Dither {
public:
  Dither():error_dist_buffer(NULL),buffer_width(0) {}
  ~Dither() {
    DestroyBuffer();
  }
  void dither_luminosity8_to_uint16(uint8_t* __restrict src, 
				    uint16_t* __restrict dest, 
				    int width, int height);

private:
  typedef int ErrorDistType; // must be wider than 8bits

  ErrorDistType* error_dist_buffer;
  int buffer_width;

  void DestroyBuffer() {
    if (error_dist_buffer) {
      delete[] error_dist_buffer;      
    }
  }

  void ResizeBuffer(const int &width) {
    if (width != buffer_width) {
      DestroyBuffer();
      buffer_width = width;
      error_dist_buffer = new ErrorDistType[(width+2)*2];
    }
  }
};

#endif

