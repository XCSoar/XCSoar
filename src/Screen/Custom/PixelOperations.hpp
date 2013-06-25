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

template<typename PixelTraits>
class BitNotPixelOperations {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::const_pointer_type const_pointer_type;

  typedef typename PixelTraits::color_type color_type;

public:
  inline void SetPixel(pointer_type p, color_type c) const {
    *p = ~c;
  }

  gcc_hot
  void FillPixels(pointer_type p, unsigned n, color_type c) const {
    for (; n > 0; --n, ++p)
      SetPixel(p, c);
  }

  gcc_hot
  void CopyPixels(pointer_type gcc_restrict p,
                  const_pointer_type gcc_restrict src, unsigned n) const {
    for (; n > 0; --n, ++p, ++src)
      SetPixel(p, *src);
  }
};

template<typename PixelTraits>
class BitOrPixelOperations {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::const_pointer_type const_pointer_type;

  typedef typename PixelTraits::color_type color_type;

public:
  inline void SetPixel(pointer_type p, color_type c) const {
    *p |= c;
  }

  gcc_hot
  void FillPixels(pointer_type p, unsigned n, color_type c) const {
    for (; n > 0; --n, ++p)
      SetPixel(p, c);
  }

  gcc_hot
  void CopyPixels(pointer_type gcc_restrict p,
                  const_pointer_type gcc_restrict src, unsigned n) const {
    for (; n > 0; --n, ++p, ++src)
      SetPixel(p, *src);
  }
};

template<typename PixelTraits>
class BitNotOrPixelOperations {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::const_pointer_type const_pointer_type;

  typedef typename PixelTraits::color_type color_type;

public:
  inline void SetPixel(pointer_type p, color_type c) const {
    *p |= ~c;
  }

  gcc_hot
  void FillPixels(pointer_type p, unsigned n, color_type c) const {
    for (; n > 0; --n, ++p)
      SetPixel(p, c);
  }

  gcc_hot
  void CopyPixels(pointer_type gcc_restrict p,
                  const_pointer_type gcc_restrict src, unsigned n) const {
    for (; n > 0; --n, ++p, ++src)
      SetPixel(p, *src);
  }
};

template<typename PixelTraits>
class BitAndPixelOperations {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::const_pointer_type const_pointer_type;

  typedef typename PixelTraits::color_type color_type;

public:
  inline void SetPixel(pointer_type p, color_type c) const {
    *p &= c;
  }

  gcc_hot
  void FillPixels(pointer_type p, unsigned n, color_type c) const {
    for (; n > 0; --n, ++p)
      SetPixel(p, c);
  }

  gcc_hot
  void CopyPixels(pointer_type gcc_restrict p,
                  const_pointer_type gcc_restrict src, unsigned n) const {
    for (; n > 0; --n, ++p, ++src)
      SetPixel(p, *src);
  }
};

template<typename PixelTraits>
class AlphaPixelOperations {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::const_pointer_type const_pointer_type;

  typedef typename PixelTraits::color_type color_type;

  int alpha;

  color_type BlendPixel(color_type a, color_type b) const {
    return a + ((int(b - a) * alpha) >> 8);
  }

public:
  constexpr AlphaPixelOperations(uint8_t _alpha):alpha(_alpha) {}

  inline void SetPixel(pointer_type p, color_type c) const {
    *p = BlendPixel(*p, c);
  }

  gcc_hot
  void FillPixels(pointer_type p, unsigned n, color_type c) const {
    for (; n > 0; --n, ++p)
      *p = BlendPixel(*p, c);
  }

  gcc_hot
  void CopyPixels(pointer_type gcc_restrict p,
                  const_pointer_type gcc_restrict src, unsigned n) const {
    for (; n > 0; --n, ++p, ++src)
      *p = BlendPixel(*p, *src);
  }
};

template<typename PixelTraits>
class OpaqueTextPixelOperations {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::const_pointer_type const_pointer_type;

  typedef typename PixelTraits::color_type color_type;

  color_type background_color, text_color;

public:
  constexpr OpaqueTextPixelOperations(color_type _b, color_type _t)
    :background_color(_b), text_color(_t) {}

  inline void SetPixel(pointer_type p, color_type c) const {
    if (c == 0)
      *p = background_color;
    else
      *p = text_color;
  }

  void CopyPixels(pointer_type gcc_restrict p,
                  const_pointer_type gcc_restrict src, unsigned n) const {
    for (; n > 0; --n, ++p, ++src) {
      SetPixel(p, *src);
    }
  }
};

template<typename PixelTraits>
class TransparentTextPixelOperations {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::const_pointer_type const_pointer_type;

  typedef typename PixelTraits::color_type color_type;

  color_type text_color;

public:
  constexpr TransparentTextPixelOperations(color_type _t)
    :text_color(_t) {}

  void CopyPixels(pointer_type gcc_restrict p,
                  const_pointer_type gcc_restrict src, unsigned n) const {
    for (; n > 0; --n, ++p, ++src) {
      if (*src != 0)
        *p = text_color;
    }
  }
};

/**
 * The input buffer contains alpha values, and each pixel is blended
 * using the alpha value, the existing color and the given color.
 */
template<typename PixelTraits>
class ColoredAlphaPixelOperations {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::const_pointer_type const_pointer_type;

  typedef typename PixelTraits::color_type color_type;

  color_type color;

  constexpr color_type BlendPixel(color_type a, uint8_t alpha) const {
    return a + ((int(color - a) * alpha) >> 8);
  }

public:
  constexpr ColoredAlphaPixelOperations(color_type _color):color(_color) {}

  void CopyPixels(pointer_type gcc_restrict p,
                  const_pointer_type gcc_restrict src, unsigned n) const {
    for (; n > 0; --n, ++p, ++src)
      *p = BlendPixel(*p, *src);
  }
};

/**
 * The input buffer contains alpha values, and each pixel is blended
 * using the alpha value between the two given colors.
 */
template<typename PixelTraits>
class OpaqueAlphaPixelOperations {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::const_pointer_type const_pointer_type;

  typedef typename PixelTraits::color_type color_type;

  int base_color, delta_color;

  constexpr color_type BlendPixel(uint8_t alpha) const {
    return base_color + ((delta_color * alpha) >> 8);
  }

public:
  constexpr OpaqueAlphaPixelOperations(color_type _a, color_type _b)
    :base_color(_a), delta_color(_b - _a) {}

  void CopyPixels(pointer_type gcc_restrict p,
                  const_pointer_type gcc_restrict src, unsigned n) const {
    for (; n > 0; --n, ++p, ++src)
      *p = BlendPixel(*src);
  }
};

template<typename PixelTraits>
class TransparentInvertPixelOperations {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::const_pointer_type const_pointer_type;

  typedef typename PixelTraits::color_type color_type;

  color_type key;

public:
  constexpr TransparentInvertPixelOperations(color_type _key):key(_key) {}

  void SetPixel(pointer_type p, color_type c) const {
    if (c != key)
      *p = ~c;
  }
};

#endif
