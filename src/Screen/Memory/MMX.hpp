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

#ifndef XCSOAR_SCREEN_MMX_HPP
#define XCSOAR_SCREEN_MMX_HPP

#include "Screen/PortableColor.hpp"

#ifndef __MMX__
#error MMX required
#endif

#include <mmintrin.h>

#if CLANG_OR_GCC_VERSION(4,8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#endif

/**
 * Implementation of AlphaPixelOperations using Intel MMX
 * instructions.
 */
class MMXAlphaPixelOperations {
  uint8_t alpha;

public:
  constexpr MMXAlphaPixelOperations(uint8_t _alpha):alpha(_alpha) {}

  gcc_hot gcc_always_inline
  static __m64 FillPixel(__m64 x, __m64 v_alpha, __m64 v_color) {
    x = _mm_mullo_pi16(x, v_alpha);
    x = _mm_add_pi16(x, v_color);
    return _mm_srli_pi16(x, 8);
  }

  gcc_hot gcc_flatten gcc_nonnull_all
  void FillPixels(__m64 *p, unsigned n, __m64 v_color) const {
    const __m64 v_alpha = _mm_set1_pi16(alpha ^ 0xff);
    const __m64 zero = _mm_setzero_si64();

    for (unsigned i = 0; i < n; ++i) {
      __m64 x = p[i];

      __m64 lo = FillPixel(_mm_unpacklo_pi8(x, zero), v_alpha, v_color);
      __m64 hi = FillPixel(_mm_unpackhi_pi8(x, zero), v_alpha, v_color);

      p[i] = _mm_packs_pu16(lo, hi);
    }
  }

  gcc_hot gcc_flatten gcc_nonnull_all
  void FillPixels(Luminosity8 *p, unsigned n, Luminosity8 c) const {
    _mm_empty();

    FillPixels((__m64 *)p, n / 8,
               _mm_set1_pi16(c.GetLuminosity() * alpha));

    _mm_empty();
  }

  gcc_hot
  void FillPixels(BGRA8Color *p, unsigned n, BGRA8Color c) const {
    _mm_empty();

    __m64 v_alpha = _mm_set1_pi16(alpha);
    __m64 v_color = _mm_setr_pi16(c.Blue(), c.Green(), c.Red(), c.Alpha());

    FillPixels((__m64 *)p, n / 2, _mm_mullo_pi16(v_color, v_alpha));

    _mm_empty();
  }

  gcc_hot gcc_always_inline
  static __m64 AlphaBlend4(__m64 p, __m64 q,
                           __m64 alpha, __m64 inverse_alpha) {
    p = _mm_mullo_pi16(p, inverse_alpha);
    q = _mm_mullo_pi16(q, alpha);
    return _mm_srli_pi16(_mm_add_pi16(p, q), 8);
  }

  gcc_flatten
  void CopyPixels(uint8_t *gcc_restrict p,
                  const uint8_t *gcc_restrict q, unsigned n) const {
    _mm_empty();

    const __m64 v_alpha = _mm_set1_pi16(alpha);
    const __m64 inverse_alpha = _mm_set1_pi16(alpha ^ 0xff);
    const __m64 zero = _mm_setzero_si64();

    __m64 *p2 = (__m64 *)p;
    const __m64 *q2 = (const __m64 *)q;

    for (unsigned i = 0; i < n / 8; ++i) {
      __m64 pv = p2[i], qv = q2[i];

      __m64 lo = AlphaBlend4(_mm_unpacklo_pi8(pv, zero),
                             _mm_unpacklo_pi8(qv, zero),
                             v_alpha, inverse_alpha);

      __m64 hi = AlphaBlend4(_mm_unpackhi_pi8(pv, zero),
                             _mm_unpackhi_pi8(qv, zero),
                             v_alpha, inverse_alpha);

      p2[i] = _mm_packs_pu16(lo, hi);
    }

    _mm_empty();
  }

  void CopyPixels(Luminosity8 *p, const Luminosity8 *q, unsigned n) const {
    CopyPixels((uint8_t *)p, (const uint8_t *)q, n);
  }

  void CopyPixels(BGRA8Color *p, const BGRA8Color *q, unsigned n) const {
    CopyPixels((uint8_t *)p, (const uint8_t *)q, n * 4);
  }
};

#if CLANG_OR_GCC_VERSION(4,8)
#pragma GCC diagnostic pop
#endif

#endif
