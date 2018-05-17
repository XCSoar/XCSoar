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

#include "Dither.hpp"

#include <algorithm>

// Code adapted from imx.60 linux kernel EPD driver by Daiyu Ko <dko@freescale.com>
//

void
Dither::DitherGreyscale(const uint8_t *gcc_restrict src,
                        unsigned src_pitch,
                        uint8_t *gcc_restrict dest,
                        unsigned dest_pitch,
                        unsigned width, unsigned height)
{
  const unsigned width_2 = width + 2;
  allocated_error_dist_buffer.GrowDiscard(width_2 * 2u);
  ErrorDistType *const error_dist_buffer = allocated_error_dist_buffer.begin();
  std::fill(error_dist_buffer, error_dist_buffer + width_2 * 2u, 0);

  for (; height; --height) {
    ErrorDistType *gcc_restrict err_dist_l0 =
      error_dist_buffer + ((height & 1) ? width_2 : 0) + 1;
    ErrorDistType *gcc_restrict err_dist_l1 =
      error_dist_buffer + ((height & 1) ? 0 : width_2);

    int e0 = *err_dist_l0++;
    int e1 = *err_dist_l1;

    /* scan the line and convert the Y8 to BW */
    for (unsigned column = 0; column < width; ++column) {
      ErrorDistType bwPix = e0 + src[column];

      uint8_t color = 0;
      if (bwPix >= 128) {
        --color;
	bwPix -= 255;
      }

      dest[column] = color;

      /* modify the error distribution buffer */

      // SIERRA LITE
      bwPix >>= 1;

      e0 = *err_dist_l0 + bwPix;
      *err_dist_l0++ = e0;

      bwPix >>= 1;

      *err_dist_l1++ = e1 + bwPix;
      e1 = bwPix;
    }

    *err_dist_l1 = e1;

    src += src_pitch;
    dest += dest_pitch;
  }
}
