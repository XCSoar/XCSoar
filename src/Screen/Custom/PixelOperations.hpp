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

#ifndef XCSOAR_SCREEN_PIXEL_OPERATIONS_HPP
#define XCSOAR_SCREEN_PIXEL_OPERATIONS_HPP

#include <functional>

template<typename PixelTraits, template<typename T> class Operation>
class UnaryPerPixelOperations
  : private PixelTraits, private Operation<PixelTraits> {

  using typename PixelTraits::pointer_type;
  using typename PixelTraits::const_pointer_type;
  using typename PixelTraits::color_type;
  using PixelTraits::ForHorizontal;

public:
  UnaryPerPixelOperations() = default;

  template<typename... Args>
  constexpr UnaryPerPixelOperations(Args&&... args)
    :Operation<PixelTraits>(std::forward<Args>(args)...) {}

  inline void WritePixel(pointer_type p, color_type c) const {
    *p = Operation<PixelTraits>::operator()(c);
  }

  gcc_hot
  void FillPixels(pointer_type p, unsigned n, color_type c) const {
    ForHorizontal(p, n, [this, c](pointer_type p){
        this->WritePixel(p, c);
      });
  }

  gcc_hot
  void CopyPixels(pointer_type gcc_restrict p,
                  const_pointer_type gcc_restrict src, unsigned n) const {
    ForHorizontal(p, src, n, [this](pointer_type p, const_pointer_type src){
        this->WritePixel(p, *src);
      });
  }
};

template<typename PixelTraits, template<typename T> class Operation>
class BinaryPerPixelOperations
  : private PixelTraits, private Operation<PixelTraits> {
  using typename PixelTraits::pointer_type;
  using typename PixelTraits::const_pointer_type;
  using typename PixelTraits::color_type;
  using PixelTraits::ForHorizontal;

public:
  BinaryPerPixelOperations() = default;

  template<typename... Args>
  constexpr BinaryPerPixelOperations(Args&&... args)
    :Operation<PixelTraits>(std::forward<Args>(args)...) {}

  inline void WritePixel(pointer_type p, color_type c) const {
    *p = Operation<PixelTraits>::operator()(*p, c);
  }

  gcc_hot
  void FillPixels(pointer_type p, unsigned n, color_type c) const {
    ForHorizontal(p, n, [this, c](pointer_type p){
        this->WritePixel(p, c);
      });
  }

  gcc_hot
  void CopyPixels(pointer_type gcc_restrict p,
                  const_pointer_type gcc_restrict src, unsigned n) const {
    ForHorizontal(p, src, n, [this](pointer_type p, const_pointer_type src){
        this->WritePixel(p, *src);
      });
  }
};

template<typename PixelTraits>
class PixelBitNot {
  typedef typename PixelTraits::color_type color_type;

public:
  constexpr color_type operator()(color_type x) const {
    return ~x;
  }
};

template<typename PixelTraits>
using BitNotPixelOperations = UnaryPerPixelOperations<PixelTraits, PixelBitNot>;

template<typename PixelTraits>
class PixelBitOr {
  typedef typename PixelTraits::color_type color_type;

public:
  constexpr color_type operator()(color_type a, color_type b) const {
    return a | b;
  }
};

template<typename PixelTraits>
using BitOrPixelOperations = BinaryPerPixelOperations<PixelTraits, PixelBitOr>;

template<typename PixelTraits>
class PixelBitNotOr {
  typedef typename PixelTraits::color_type color_type;

public:
  constexpr color_type operator()(color_type a, color_type b) const {
    return a | ~b;
  }
};

template<typename PixelTraits>
using BitNotOrPixelOperations = BinaryPerPixelOperations<PixelTraits,
                                                         PixelBitNotOr>;

template<typename PixelTraits>
class PixelBitAnd {
  typedef typename PixelTraits::color_type color_type;

public:
  constexpr color_type operator()(color_type a, color_type b) const {
    return a & b;
  }
};

template<typename PixelTraits>
using BitAndPixelOperations = BinaryPerPixelOperations<PixelTraits,
                                                       PixelBitAnd>;

template<typename PixelTraits>
class PixelAlphaOperation {
  typedef typename PixelTraits::color_type color_type;

  const int alpha;

public:
  constexpr explicit PixelAlphaOperation(uint8_t _alpha):alpha(_alpha) {}

  color_type operator()(color_type a, color_type b) const {
    return a + ((int(b - a) * alpha) >> 8);
  }
};

template<typename PixelTraits>
using AlphaPixelOperations = BinaryPerPixelOperations<PixelTraits,
                                                      PixelAlphaOperation>;

template<typename PixelTraits>
class PixelOpaqueText {
  typedef typename PixelTraits::color_type color_type;

  const color_type background_color, text_color;

public:
  constexpr PixelOpaqueText(color_type _b, color_type _t)
    :background_color(_b), text_color(_t) {}

  inline color_type operator()(color_type x) const {
    return x == 0
      ? background_color
      : text_color;
  }
};

template<typename PixelTraits>
using OpaqueTextPixelOperations = UnaryPerPixelOperations<PixelTraits,
                                                          PixelOpaqueText>;

template<typename PixelTraits>
class TransparentTextPixelOperations : private PixelTraits {
  using typename PixelTraits::pointer_type;
  using typename PixelTraits::const_pointer_type;
  using typename PixelTraits::color_type;
  using PixelTraits::ForHorizontal;

  color_type text_color;

public:
  constexpr TransparentTextPixelOperations(color_type _t)
    :text_color(_t) {}

  void CopyPixels(pointer_type gcc_restrict p,
                  const_pointer_type gcc_restrict src, unsigned n) const {
    ForHorizontal(p, src, n, [this](pointer_type p, const_pointer_type src){
        if (*src != 0)
          *p = text_color;
      });
  }
};

/**
 * The input buffer contains alpha values, and each pixel is blended
 * using the alpha value, the existing color and the given color.
 */
template<typename PixelTraits>
class PixelColoredAlpha {
  typedef typename PixelTraits::color_type color_type;

  color_type color;

public:
  constexpr explicit PixelColoredAlpha(color_type _color):color(_color) {}

  constexpr color_type operator()(color_type a, uint8_t alpha) const {
    return a + ((int(color - a) * int(alpha)) >> 8);
  }
};

template<typename PixelTraits>
using ColoredAlphaPixelOperations = BinaryPerPixelOperations<PixelTraits,
                                                             PixelColoredAlpha>;

/**
 * The input buffer contains alpha values, and each pixel is blended
 * using the alpha value between the two given colors.
 */
template<typename PixelTraits>
class PixelOpaqueAlpha {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::color_type color_type;

  const int base_color, delta_color;

public:
  constexpr PixelOpaqueAlpha(color_type _a, color_type _b)
    :base_color(_a), delta_color(_b - _a) {}

  constexpr color_type operator()(uint8_t alpha) const {
    return base_color + ((delta_color * alpha) >> 8);
  }
};

template<typename PixelTraits>
using OpaqueAlphaPixelOperations = UnaryPerPixelOperations<PixelTraits,
                                                           PixelOpaqueAlpha>;

template<typename PixelTraits>
class TransparentInvertPixelOperations : private PixelTraits {
  using typename PixelTraits::pointer_type;
  using typename PixelTraits::color_type;

  color_type key;

public:
  constexpr TransparentInvertPixelOperations(color_type _key):key(_key) {}

  void WritePixel(pointer_type p, color_type c) const {
    if (c != key)
      *p = ~c;
  }
};

#endif
