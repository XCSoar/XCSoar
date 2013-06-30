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

#include "Dither.hpp"

#include <string.h>

// Code adapted from imx.60 linux kernel EPD driver by Daiyu Ko <dko@freescale.com>
//

void Dither::dither_luminosity8_to_uint16(const uint8_t *gcc_restrict src,
                                          unsigned src_pitch,
                                          uint16_t *gcc_restrict dest,
                                          unsigned dest_pitch,
                                          unsigned width, unsigned height)
{
  const unsigned width_2 = width + 2;
  allocated_error_dist_buffer.GrowDiscard(width_2 * 2u);
  ErrorDistType *const error_dist_buffer = allocated_error_dist_buffer.begin();

  memset(error_dist_buffer, 0, (width_2)*2*sizeof(ErrorDistType));

  src_pitch -= width;
  dest_pitch -= width;

  for (; height; --height) {

    ErrorDistType* err_dist_l0 = error_dist_buffer + ((height & 1) ? width_2 : 0) + 1;
    ErrorDistType* err_dist_l1 = error_dist_buffer + ((height & 1) ? 0 : width_2) + 1;

    /* scan the line and convert the Y8 to BW */
    for (unsigned col = width; col > 0; --col) {
      ErrorDistType bwPix = (*err_dist_l0) + (*src++);

      if (bwPix >= 128) {
	*dest++ = 0xffff;
	bwPix -= 255;
      } else {
	*dest++ = 0;
      }
      /* modify the error distribution buffer */

      // SIERRA LITE
      bwPix >>= 1;
      *(++err_dist_l0) += bwPix;
      bwPix >>= 1;
      *(err_dist_l1 - 1) += bwPix;
      *err_dist_l1++ = bwPix;

    }

    src += src_pitch;
    dest += dest_pitch;
  }
}
