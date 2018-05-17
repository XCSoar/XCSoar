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

#ifndef XCSOAR_SCREEN_NEON_HPP
#define XCSOAR_SCREEN_NEON_HPP

#include "Screen/PortableColor.hpp"

#ifndef __ARM_NEON__
#error ARM NEON required
#endif

#include <arm_neon.h>

/**
 * Implementation of BitOrPixelOperations using ARM NEON instructions.
 */
class NEONBitOrPixelOperations {
public:
  gcc_always_inline
  static void Blend16(uint8_t *gcc_restrict p,
                      const uint8_t *gcc_restrict q) {
    uint8x16_t pv = vld1q_u8(p);
    uint8x16_t qv = vld1q_u8(q);

    uint8x16_t r = vorrq_u8(pv, qv);
    vst1q_u8(p, r);
  }

  gcc_flatten
  void CopyPixels(uint8_t *gcc_restrict p,
                  const uint8_t *gcc_restrict q, unsigned n) const {
    for (unsigned i = 0; i < n / 16; ++i, p += 16, q += 16)
      Blend16(p, q);
  }

  void CopyPixels(Luminosity8 *p, const Luminosity8 *q, unsigned n) const {
    CopyPixels((uint8_t *)p, (const uint8_t *)q, n);
  }
};

/**
 * Implementation of TransparentPixelOperations using ARM NEON
 * instructions.
 */
class NEONTransparentPixelOperations {
  uint8_t key;

public:
  constexpr NEONTransparentPixelOperations(Luminosity8 _key)
    :key(_key.GetLuminosity()) {}

  gcc_always_inline
  static void Blend32(uint8_t *gcc_restrict p,
                      const uint8_t *gcc_restrict q,
                      uint8x16_t key) {
    uint8x16x2_t q2 = vld2q_u8(q);

    uint8x16_t mask0 = vceqq_u8(q2.val[0], key);
    uint8x16_t mask1 = vceqq_u8(q2.val[1], key);
    uint8x16_t q0 = vbicq_u8(q2.val[0], mask0);
    uint8x16_t q1 = vbicq_u8(q2.val[1], mask1);

    uint8x16x2_t p2 = vld2q_u8(p);
    uint8x16_t p0 = vandq_u8(p2.val[0], mask0);
    uint8x16_t p1 = vandq_u8(p2.val[1], mask1);

    uint8x16_t r0 = vorrq_u8(p0, q0);
    uint8x16_t r1 = vorrq_u8(p1, q1);

    uint8x16x2_t r = { { r0, r1 } };

    vst2q_u8(p, r);
  }

  gcc_flatten
  void CopyPixels(uint8_t *gcc_restrict p,
                  const uint8_t *gcc_restrict q, unsigned n) const {
    const uint8x16_t v_key = vdupq_n_u8(key);

    for (unsigned i = 0; i < n / 32; ++i, p += 32, q += 32)
      Blend32(p, q, v_key);
  }

  void CopyPixels(Luminosity8 *p, const Luminosity8 *q, unsigned n) const {
    CopyPixels((uint8_t *)p, (const uint8_t *)q, n);
  }
};

/**
 * Implementation of AlphaPixelOperations using ARM NEON instructions.
 */
class NEONAlphaPixelOperations {
  uint8_t alpha;

public:
  constexpr NEONAlphaPixelOperations(uint8_t _alpha):alpha(_alpha) {}

  gcc_hot gcc_flatten gcc_nonnull_all
  void FillPixels(uint8_t *p, unsigned n, uint8_t c) const {
    const uint8x8_t v_alpha = vdup_n_u8(~alpha);
    const uint16x8_t v_color = vdupq_n_u16(c * alpha);

    for (unsigned i = 0; i < n / 16; ++i, p += 16) {
      uint8x8x2_t p2 = vld2_u8(p);
      uint16x8_t p0 = vmull_u8(p2.val[0], v_alpha);
      uint16x8_t p1 = vmull_u8(p2.val[1], v_alpha);

      uint8x8_t r0 = vraddhn_u16(p0, v_color);
      uint8x8_t r1 = vraddhn_u16(p1, v_color);

      uint8x8x2_t r = { { r0, r1 } };
      vst2_u8(p, r);
    }
  }

  gcc_hot
  void FillPixels(Luminosity8 *p, unsigned n, Luminosity8 c) const {
    FillPixels((uint8_t *)p, n, c.GetLuminosity());
  }

  gcc_always_inline
  static void AlphaBlend16(uint8_t *gcc_restrict p,
                           const uint8_t *gcc_restrict q,
                           uint8x8_t alpha, uint8x8_t inverse_alpha) {
    uint8x8x2_t p2 = vld2_u8(p);
    uint16x8_t p0 = vmull_u8(p2.val[0], alpha);
    uint16x8_t p1 = vmull_u8(p2.val[1], alpha);

    uint8x8x2_t q2 = vld2_u8(q);
    uint16x8_t q0 = vmull_u8(q2.val[0], inverse_alpha);
    uint16x8_t q1 = vmull_u8(q2.val[1], inverse_alpha);

    uint8x8_t r0 = vraddhn_u16(p0, q0);
    uint8x8_t r1 = vraddhn_u16(p1, q1);

    uint8x8x2_t r = { { r0, r1 } };

    vst2_u8(p, r);
  }

  gcc_flatten
  void CopyPixels(uint8_t *gcc_restrict p,
                  const uint8_t *gcc_restrict q, unsigned n) const {
    const uint8x8_t v_alpha = vdup_n_u8(alpha);
    const uint8x8_t inverse_alpha = vdup_n_u8(~alpha);

    for (unsigned i = 0; i < n / 16; ++i, p += 16, q += 16)
      AlphaBlend16(p, q, inverse_alpha, v_alpha);
  }

  void CopyPixels(Luminosity8 *p, const Luminosity8 *q, unsigned n) const {
    CopyPixels((uint8_t *)p, (const uint8_t *)q, n);
  }
};

/**
 * Read bytes and emit each byte twice.  This class reads 16 bytes at
 * a time, and writes 32 bytes at a time.
 */
struct NEONBytesTwice {
  static void Copy16(uint8_t *gcc_restrict p, const uint8_t *gcc_restrict q) {
    const uint8x16_t a1 = vld1q_u8(q);
    const uint8x16x2_t a2 = {{ a1, a1 }};

    /* vst2 interleaves the two parts, which is exactly what we need
       here */
    vst2q_u8(p, a2);
  }

  gcc_flatten
  void CopyPixels(uint8_t *gcc_restrict p,
                  const uint8_t *gcc_restrict q, unsigned n) const {
    for (unsigned i = 0; i < n / 16; ++i, p += 32, q += 16)
      Copy16(p, q);
  }

  /**
   * @param n the number of source pixels (multiple of 16)
   */
  void CopyPixels(Luminosity8 *p, const Luminosity8 *q, unsigned n) const {
    CopyPixels((uint8_t *)p, (const uint8_t *)q, n);
  }
};

#endif
