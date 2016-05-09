/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef THERMAL_BAND_LOOK_HPP
#define THERMAL_BAND_LOOK_HPP

#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"

struct ThermalBandLook {
  bool inverse;

  // elements for drawing active thermal bands
  Pen pen_active;
  Brush brush_active;

  // elements for drawing inactive thermal band
  Pen pen_inactive;
  Brush brush_inactive;

  // pens used for drawing the MC setting
  Pen white_pen, black_pen;

  // pen used for drawing the working band
  Pen working_band_pen;

  void Initialise(bool inverse, Color sky_color);
};

#endif
