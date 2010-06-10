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

#include "Screen/RawBitmap.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>

#include <assert.h>

/**
 * Returns minimum width that is greater then the given width and
 * that is acceptable as image width (not all numbers are acceptable)
 */
static inline unsigned
CorrectedWidth(unsigned nWidth)
{
  return ((nWidth + 3) / 4) * 4;
}

RawBitmap::RawBitmap(unsigned nWidth, unsigned nHeight, const Color clr)
  :width(nWidth), height(nHeight),
   corrected_width(CorrectedWidth(nWidth)),
   second_buffer(new BGRColor[height * corrected_width])
{
  assert(nWidth > 0);
  assert(nHeight > 0);

  buffer = (BGRColor *)create(corrected_width, height);
  assert(defined());
  assert(buffer);

  BGRColor bgrColor = BGRColor(clr.blue(), clr.green(), clr.red());
  for (BGRColor *p = buffer,
         *end = buffer + corrected_width * height;
       p < end; ++p)
    *p++ = bgrColor;
}

RawBitmap::~RawBitmap()
{
  delete[](second_buffer);
}

void
RawBitmap::Zoom(unsigned int step)
{
  const BGRColor *src = buffer;

  const unsigned int smallx = corrected_width / step;
  const unsigned int smally = height / step;
  const unsigned int rowsize = corrected_width * sizeof(BGRColor);
  const unsigned int wstep = corrected_width * step;
  const unsigned int stepmo = step-1;

  BGRColor *dst_start = second_buffer + (smally - 1) * wstep;
  for (unsigned int y = smally; y--; dst_start -= wstep) {
    BGRColor *dst = dst_start;
    for (unsigned int x = smallx; x--; src++, dst += step)
      std::fill(dst, dst + step, *src);

    // done first row, now copy each row
    for (unsigned int k = stepmo; k--; dst += corrected_width)
      memcpy(dst, dst_start, rowsize);
  }

  // copy it back to main buffer
  memcpy(buffer, second_buffer, rowsize * height);
}

void
RawBitmap::HorizontalBlur(unsigned int boxw)
{
  const unsigned int muli = boxw * 2 + 1;
  const BGRColor *src = buffer;
  BGRColor *dst = second_buffer;

  const unsigned int off1 = boxw+1;
  const unsigned int off2 = corrected_width - boxw - 1;
  const unsigned int right = corrected_width - boxw;

  for (unsigned int y = height; y--; ) {
    unsigned int tot_r=0;
    unsigned int tot_g=0;
    unsigned int tot_b=0;
    unsigned int x;

    const BGRColor *c = src + boxw - 1;

    for (x = boxw; x--; c--) {
      tot_r += c->m_R;
      tot_g += c->m_G;
      tot_b += c->m_B;
    }

    for (x = 0; x < corrected_width; x++) {
      unsigned int acc = muli;
      if (x > boxw) {
        c = src-off1;
        tot_r -= c->m_R;
        tot_g -= c->m_G;
        tot_b -= c->m_B;
      } else
        acc += x-boxw;

      if (x < right) {
        c = src+boxw;
        tot_r += c->m_R;
        tot_g += c->m_G;
        tot_b += c->m_B;
      } else
        acc += off2 - x;

      dst->m_R = (unsigned char)(tot_r / acc);
      dst->m_G = (unsigned char)(tot_g / acc);
      dst->m_B = (unsigned char)(tot_b / acc);

      src++;
      dst++;
    }
  }

  // copy it back to main buffer
  memcpy(buffer, second_buffer, corrected_width * height * sizeof(buffer[0]));
}

void
RawBitmap::VerticalBlur(unsigned int boxh)
{
  const BGRColor *src = buffer;
  BGRColor *dst = second_buffer;

  const unsigned int muli = (boxh * 2 + 1);
  const unsigned int iboxh = corrected_width * boxh;
  const unsigned int off1 = iboxh + corrected_width;
  const unsigned int off2 = height - boxh - 1;
  const unsigned int bottom = height - boxh;

  for (unsigned int x = corrected_width; x--;) {
    unsigned int tot_r = 0;
    unsigned int tot_g = 0;
    unsigned int tot_b = 0;
    unsigned int y;

    const BGRColor *c, *d;
    c = d = src + x;
    BGRColor *e = dst + x;

    for (y = boxh; y--; c += corrected_width) {
      tot_r += c->m_R;
      tot_g += c->m_G;
      tot_b += c->m_B;
    }

    for (y = 0; y < height; y++) {
      unsigned int acc = muli;
      if (y > boxh) {
        c = d - off1;
        tot_r -= c->m_R;
        tot_g -= c->m_G;
        tot_b -= c->m_B;
      } else
        acc += y-boxh;

      if (y < bottom) {
        c = d + iboxh;
        tot_r += c->m_R;
        tot_g += c->m_G;
        tot_b += c->m_B;
      } else
        acc += off2- y;

      e->m_R = (unsigned char)(tot_r / acc);
      e->m_G = (unsigned char)(tot_g / acc);
      e->m_B = (unsigned char)(tot_b / acc);
      d += corrected_width;
      e += corrected_width;
    }
  }

  // copy it back to main buffer
  memcpy(buffer, second_buffer, corrected_width * height * sizeof(buffer[0]));
}
