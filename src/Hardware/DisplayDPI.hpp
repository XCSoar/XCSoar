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

#ifndef XCSOAR_HARDWARE_DISPLAY_DPI_HPP
#define XCSOAR_HARDWARE_DISPLAY_DPI_HPP

#include "Compiler.h"

namespace Display {
  /**
   * Sets the displays x/y DPI
   * @param x Number of pixels per logical inch along the screen width
   * @param y Number of pixels per logical inch along the screen height
   */
  void SetForcedDPI(unsigned x_dpi, unsigned y_dpi);

  /**
   * Returns the number of pixels per logical inch along the screen width
   * @param custom_dpi overide system dpi settings, but not command line dpi
   * @return Number of pixels per logical inch along the screen width
   */
#ifndef __APPLE__
  gcc_const
#endif
  unsigned GetXDPI(unsigned custom_dpi=0);
  /**
   * Returns the number of pixels per logical inch along the screen height
   * @param custom_dpi overide system dpi settings, but not command line dpi
   * @return Number of pixels per logical inch along the screen height
   */
#ifndef __APPLE__
  gcc_const
#endif
  unsigned GetYDPI(unsigned custom_dpi=0);
}

#endif
