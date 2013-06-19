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
#include <assert.h>
#include <string.h>

// Code adapted from imx.60 linux kernel EPD driver by Daiyu Ko <dko@freescale.com>
//

void Dither::dither_luminosity8_to_uint16(uint8_t* __restrict src, 
					  uint16_t* __restrict dest, 
					  int width, int height)
{
  if (width> buffer_width) 
    ResizeBuffer(width);

#ifdef ATKINSON
  int width_3 = width + 3;
  memset(error_dist_buffer, 0, (width+3)*3*sizeof(int16_t));
#else
  int width_2 = width + 2;
  memset(error_dist_buffer, 0, (width+2)*2*sizeof(int16_t));
#endif

  for (int row = 0; row < height; ++row) {

#ifdef ATKINSON      
    int16_t * err_dist_l0 = error_dist_buffer + width_3 * (row % 3) + 1;
    int16_t * err_dist_l1 = error_dist_buffer + width_3 * ((row + 1) % 3) + 1;
    int16_t * err_dist_l2 = error_dist_buffer + width_3 * ((row + 2) % 3) + 1;
#else
    int16_t * err_dist_l0 = error_dist_buffer + width_2 * (row % 2) + 1;
    int16_t * err_dist_l1 = error_dist_buffer + width_2 * ((row + 1) % 2) + 1;
#endif

    /* scan the line and convert the Y8 to BW */
    for (int col = width; col; --col) {
      int16_t bwPix = (*err_dist_l0) + (*src++);

      if (bwPix >= 128) {
	*dest++ = 0xffff;
	bwPix -= 255;
      } else {
	*dest++ = 0;
      }
      /* modify the error distribution buffer */

#ifdef ATKINSON      
      bwPix = bwPix >> 3;
      *(err_dist_l0 + 1) += bwPix;
      *(err_dist_l0 + 2) += bwPix;
      *(err_dist_l1 + 1) += bwPix;
      *(err_dist_l1 - 1) += bwPix;
      *err_dist_l1++ += bwPix;
      *err_dist_l2++ = bwPix;
      ++err_dist_l0;
#else
      // SIERRA LITE
      bwPix >>= 1;
      *(++err_dist_l0) += bwPix;
      bwPix >>= 1;
      *(err_dist_l1 - 1) += bwPix;
      *err_dist_l1++ = bwPix;
#endif
    }
  }
}
