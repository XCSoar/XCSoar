/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Concepts.hpp"
#include "../PortableColor.hpp"
#include "util/Compiler.h"

#include <functional>

/**
 * Convert a PixelTraits type to a PixelOperations type.
 */
template<typename PT>
struct PixelTraitsOperations : public PT {
  using PixelTraits = PT;
  using SourcePixelTraits = PT;

  constexpr PixelTraitsOperations(const PixelTraits &pt) noexcept
    :PT(pt) {}
};

/**
 * Build a PixelOperations class with a base class that implements
 * only WritePixelOperation().
 */
template<AnyWritePixelOperation WritePixelOperation>
struct PerPixelOperations : private WritePixelOperation {
  using typename WritePixelOperation::PixelTraits;
  using pointer = typename PixelTraits::pointer;
  using rpointer = typename PixelTraits::rpointer;
  using const_pointer = typename PixelTraits::const_pointer;
  using color_type = typename PixelTraits::color_type;

  using typename WritePixelOperation::SourcePixelTraits;
  using source_color_type = typename SourcePixelTraits::color_type;
  using source_const_rpointer = typename SourcePixelTraits::const_rpointer;

  using WritePixelOperation::WritePixelOperation;

  inline void WritePixel(pointer p, source_color_type c) const {
    WritePixelOperation::WritePixel(p, c);
  }

  gcc_hot
  void FillPixels(pointer p, unsigned n, source_color_type c) const {
    PixelTraits::ForHorizontal(p, n, [this, c](pointer p){
        /* requires "this->" due to gcc 4.7.2 crash bug */
        this->WritePixel(p, c);
      });
  }

  gcc_hot
  void CopyPixels(rpointer p, source_const_rpointer src,
                  unsigned n) const {
    for (unsigned i = 0; i < n; ++i)
      WritePixel(PixelTraits::Next(p, i),
                 SourcePixelTraits::ReadPixel(SourcePixelTraits::Next(src, i)));
  }
};

template<AnyPixelOperation Operation>
struct UnaryWritePixel : private Operation {
  using typename Operation::PixelTraits;
  using pointer = typename PixelTraits::pointer;

  using typename Operation::SourcePixelTraits;
  using source_color_type = typename SourcePixelTraits::color_type;

  using Operation::Operation;

  inline void WritePixel(pointer p, source_color_type c) const {
    PixelTraits::WritePixel(p, (*this)(c));
  }
};

/**
 * Build a PixelOperations class with a function object that
 * manipulates the source color.  It is called "unary" because the
 * function object has one parameter.
 */
template<typename Operation>
using UnaryPerPixelOperations =
  PerPixelOperations<UnaryWritePixel<Operation>>;

template<AnyPixelOperation Operation>
struct BinaryWritePixel : private Operation {
  using typename Operation::PixelTraits;
  using pointer = typename PixelTraits::pointer;

  using typename Operation::SourcePixelTraits;
  using source_color_type = typename SourcePixelTraits::color_type;

  using Operation::Operation;

  inline void WritePixel(pointer p, source_color_type c) const {
    PixelTraits::WritePixel(p, (*this)(PixelTraits::ReadPixel(p), c));
  }
};

/**
 * Build a PixelOperations class with a function object that
 * manipulates the source color, blending with the (old) destination
 * color.  It is called "binary" because the function object has two
 * parameters.
 */
template<AnyPixelOperation Operation>
using BinaryPerPixelOperations =
  PerPixelOperations<BinaryWritePixel<Operation>>;

/**
 * Modify a destination pixel only if the check returns true.
 */
template<typename Check, typename Operation=typename Check::PixelTraits>
struct ConditionalWritePixel : private Check, private Operation {
  using typename Check::PixelTraits;
  using rpointer = typename PixelTraits::rpointer;
  using const_rpointer = typename PixelTraits::const_rpointer;
  using color_type = typename PixelTraits::color_type;

  using SourcePixelTraits = PixelTraits;

  ConditionalWritePixel() = default;

  template<typename... Args>
  explicit constexpr ConditionalWritePixel(Args&&... args)
    :Check(std::forward<Args>(args)...) {}

  template<typename C, typename O>
  ConditionalWritePixel(C &&c, O &&o)
    :Check(std::forward<C>(c)), Operation(std::forward<O>(o)) {}

  void WritePixel(rpointer p, color_type c) const {
    if (Check::operator()(c))
      Operation::WritePixel(p, c);
  }
};

/**
 * Modify a destination pixel only if the check returns true.
 */
template<typename Check, typename Operation=typename Check::PixelTraits>
using ConditionalPixelOperations =
  PerPixelOperations<ConditionalWritePixel<Check, Operation>>;

/**
 * Wrap an existing function object that expects to operate on one
 * channel.  The resulting function object will operate on a
 * PixelTraits::color_type.
 */
template<AnyPixelTraits PT, typename Operation>
struct PixelPerChannelAdapter : private Operation {
  using PixelTraits = PT;
  using color_type = typename PixelTraits::color_type;
  using channel_type = typename PixelTraits::channel_type;

  using SourcePixelTraits = PixelTraits;

  using Operation::Operation;

  constexpr color_type operator()(color_type x) const {
    return PixelTraits::TransformChannels(x, [this](channel_type x) {
        /* requires "this->" due to gcc 4.7.2 crash bug */
        return this->Operation::operator()(x);
      });
  }

  constexpr color_type operator()(color_type a, color_type b) const {
    return PixelTraits::TransformChannels(a, b,
                                          [this](channel_type a,
                                                 channel_type b) {
        /* requires "this->" due to gcc 4.7.2 crash bug */
        return this->Operation::operator()(a, b);
      });
  }
};

/**
 * Wrapper that glues #UnaryPerPixelOperations,
 * #PixelPerChannelAdapter and a custom function class together.
 */
template<AnyPixelTraits PixelTraits, typename Operation>
using UnaryPerChannelOperations =
  UnaryPerPixelOperations<PixelPerChannelAdapter<PixelTraits, Operation>>;

/**
 * Wrapper that glues #BinaryPerPixelOperations,
 * #PixelPerChannelAdapter and a custom function class together.
 */
template<AnyPixelTraits PixelTraits, typename Operation>
using BinaryPerChannelOperations =
  BinaryPerPixelOperations<PixelPerChannelAdapter<PixelTraits, Operation>>;

/**
 * Wrap an existing function object that expects to operate on one
 * integer.  The resulting function object will operate on a
 * PixelTraits::color_type.
 */
template<AnyPixelTraits PT, typename Operation>
struct PixelIntegerAdapter : private Operation {
  using PixelTraits = PT;
  using color_type = typename PixelTraits::color_type;
  using integer_type = typename PixelTraits::integer_type;

  using SourcePixelTraits = PixelTraits;

  using argument_type = color_type;
  using first_argument_type = color_type;
  using second_argument_type = color_type;
  using result_type = color_type;

  using Operation::Operation;

  constexpr result_type operator()(argument_type x) const {
    return PixelTraits::TransformInteger(x, [this](integer_type x) {
        /* requires "this->" due to gcc 4.7.2 crash bug */
        return this->Operation::operator()(x);
      });
  }

  constexpr result_type operator()(first_argument_type a,
                                   second_argument_type b) const {
    return PixelTraits::TransformInteger(a, b,
                                         [this](integer_type a,
                                                integer_type b) {
        /* requires "this->" due to gcc 4.7.2 crash bug */
        return this->Operation::operator()(a, b);
      });
  }
};

/**
 * Wrapper that glues #UnaryPerPixelOperations, #PixelIntegerAdapter
 * and a custom function class together.
 */
template<AnyPixelTraits PixelTraits, typename Operation>
using UnaryIntegerOperations =
  UnaryPerPixelOperations<PixelIntegerAdapter<PixelTraits, Operation>>;

/**
 * Wrapper that glues #BinaryPerPixelOperations, #PixelIntegerAdapter
 * and a custom function class together.
 */
template<AnyPixelTraits PixelTraits, typename Operation>
using BinaryIntegerOperations =
  BinaryPerPixelOperations<PixelIntegerAdapter<PixelTraits, Operation>>;

/**
 * Function that inverts all bits in the given integer.
 */
template<typename integer_type>
struct PixelBitNot {
  constexpr integer_type operator()(integer_type x) const {
    return ~x;
  }
};

/**
 * Invert all source colors.
 */
template<AnyPixelTraits PixelTraits>
using BitNotPixelOperations =
  UnaryIntegerOperations<PixelTraits,
                         PixelBitNot<typename PixelTraits::integer_type>>;

/**
 * Combine source and destination color with bit-wise "or".
 */
template<AnyPixelTraits PixelTraits>
using PortableBitOrPixelOperations =
  BinaryIntegerOperations<PixelTraits,
                          std::bit_or<typename PixelTraits::integer_type>>;

template<typename integer_type>
struct PixelBitNotOr {
  constexpr integer_type operator()(integer_type a, integer_type b) const {
    return a | ~b;
  }
};

template<AnyPixelTraits PixelTraits>
using BitNotOrPixelOperations =
  BinaryIntegerOperations<PixelTraits,
                          PixelBitNotOr<typename PixelTraits::integer_type>>;

/**
 * Combine source and destination color with bit-wise "and".
 */
template<AnyPixelTraits PixelTraits>
using BitAndPixelOperations =
  BinaryIntegerOperations<PixelTraits,
                          std::bit_and<typename PixelTraits::integer_type>>;

/**
 * Blend source and destination color with a given alpha value.  This
 * is a per-channel function.
 */
template<typename T>
class PixelAlphaOperation {
  const int alpha;

public:
  constexpr explicit PixelAlphaOperation(uint8_t _alpha):alpha(_alpha) {}

  T operator()(T a, T b) const {
    return a + ((int(b - a) * alpha) >> 8);
  }
};

/**
 * Blend source and destination color with a given alpha value.
 */
template<AnyPixelTraits PixelTraits>
using PortableAlphaPixelOperations =
  BinaryPerChannelOperations<PixelTraits,
                             PixelAlphaOperation<typename PixelTraits::channel_type>>;

template<AnyPixelTraits PT>
struct NotWhiteCondition {
  using PixelTraits = PT;
  using color_type = typename PT::color_type;

  constexpr bool operator()(color_type c) const {
    return !PixelTraits::IsWhite(c);
  }
};

template<AnyPixelTraits PixelTraits>
using NotWhiteAlphaPixelOperations =
  ConditionalPixelOperations<NotWhiteCondition<PixelTraits>,
                             PortableAlphaPixelOperations<PixelTraits>>;

template<AnyPixelTraits PT, AnyPixelTraits SPT>
struct PixelOpaqueText {
  using PixelTraits = PT;
  using color_type = typename PixelTraits::color_type;

  using SourcePixelTraits = SPT;

private:
  const color_type background_color, text_color;

public:
  constexpr PixelOpaqueText(color_type _b, color_type _t)
    :background_color(_b), text_color(_t) {}

  inline color_type operator()(typename SPT::color_type x) const {
    return SPT::IsBlack(x)
      ? background_color
      : text_color;
  }
};

template<AnyPixelTraits PixelTraits, AnyPixelTraits SPT>
using OpaqueTextPixelOperations =
  UnaryPerPixelOperations<PixelOpaqueText<PixelTraits, SPT>>;

/**
 * The input buffer contains alpha values, and each pixel is blended
 * using the alpha value, the existing color and the given color.
 */
template<AnyPixelTraits PT, AnyPixelTraits SPT>
struct PixelColoredAlpha {
  using PixelTraits = PT;
  using color_type = typename PixelTraits::color_type;
  using channel_type = typename PixelTraits::channel_type;

  using SourcePixelTraits = SPT;

private:
  color_type color;

public:
  constexpr explicit PixelColoredAlpha(color_type _color):color(_color) {}

  constexpr color_type operator()(color_type a, Luminosity8 alpha) const {
    return PixelTraits::TransformChannels(a, color,
                                          [alpha](channel_type a,
                                                  channel_type color) {
        return a + ((int(color - a) * int(alpha.GetLuminosity())) >> 8);
      });
  }
};

template<AnyPixelTraits PixelTraits, AnyPixelTraits SPT>
using ColoredAlphaPixelOperations =
  BinaryPerPixelOperations<PixelColoredAlpha<PixelTraits, SPT>>;

/**
 * The input buffer contains alpha values, and each pixel is blended
 * using the alpha value between the two given colors.
 */
template<AnyPixelTraits PT, AnyPixelTraits SPT>
struct PixelOpaqueAlpha {
  using PixelTraits = PT;
  using color_type = typename PixelTraits::color_type;
  using channel_type = typename PixelTraits::channel_type;

  using SourcePixelTraits = SPT;

private:
  const color_type a, b;

public:
  constexpr PixelOpaqueAlpha(color_type _a, color_type _b):a(_a), b(_b) {}

  constexpr color_type operator()(Luminosity8 alpha) const {
    return PixelTraits::TransformChannels(a, b,
                                          [alpha](channel_type a,
                                                  channel_type b) {
        return a + ((int(b - a) * int(alpha.GetLuminosity())) >> 8);
      });
  }
};

template<typename PixelTraits, AnyPixelTraits SPT>
using OpaqueAlphaPixelOperations =
  UnaryPerPixelOperations<PixelOpaqueAlpha<PixelTraits, SPT>>;

template<AnyPixelTraits PT>
struct ColorKey {
  using PixelTraits = PT;
  using color_type = typename PixelTraits::color_type;
  using argument_type = color_type;
  using result_type = bool;

  argument_type key;

  explicit constexpr ColorKey(argument_type _key):key(_key) {}

  result_type operator()(argument_type c) const {
    return c != key;
  }
};

/**
 * Color keying: skip writing a pixel if the source color matches the
 * given color key.
 */
template<AnyPixelTraits PixelTraits>
using PortableTransparentPixelOperations =
  ConditionalPixelOperations<ColorKey<PixelTraits>>;

template<AnyPixelTraits PixelTraits>
class TransparentInvertPixelOperations
  : private PixelIntegerAdapter<PixelTraits,
                                PixelBitNot<typename PixelTraits::integer_type>> {
  using pointer = typename PixelTraits::pointer;
  using color_type = typename PixelTraits::color_type;

  color_type key;

public:
  constexpr TransparentInvertPixelOperations(color_type _key):key(_key) {}

  void WritePixel(pointer p, color_type c) const {
    if (c != key)
      PixelTraits::WritePixel(p, (*this)(c));
  }
};

#endif
