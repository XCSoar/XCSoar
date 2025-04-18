// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <concepts>
#include <cstddef>

template<typename T>
concept AnyPixelTraits = requires (const T &t) {
  typename T::color_type;
  typename T::channel_type;
  typename T::integer_type;
  typename T::pointer;
  typename T::rpointer;
  typename T::const_pointer;
  typename T::const_rpointer;

  t.WritePixel(typename T::pointer{}, typename T::color_type{});
  {t.ReadPixel(typename T::const_pointer{})} -> std::same_as<typename T::color_type>;
};

template<typename T>
concept AnyPixelOperation = requires (const T &t) {
  requires AnyPixelTraits<typename T::PixelTraits>;
  requires AnyPixelTraits<typename T::SourcePixelTraits>;
};

template<typename T>
concept AnyWritePixelOperation = requires (const T &t) {
  requires AnyPixelOperation<T>;

  t.WritePixel(typename T::PixelTraits::pointer{},
               typename T::SourcePixelTraits::color_type{});
};

template<typename T>
concept AnyFillPixelOperation = requires
  (const T &t,
   typename T::PixelTraits::pointer p) {
    requires AnyPixelOperation<T>;

  t.FillPixels(p, std::size_t{},
               typename T::SourcePixelTraits::color_type{});
};

template<typename T>
concept AnyCopyPixelOperation = requires
  (const T &t,
   typename T::PixelTraits::pointer dest,
   typename T::PixelTraits::pointer src) {
  requires AnyPixelOperation<T>;

  t.CopyPixels(dest, src, std::size_t{});
};

template<typename T>
concept AnyBulkPixelOperation = requires {
  requires (AnyFillPixelOperation<T> || AnyCopyPixelOperation<T>);
};

template<typename T>
concept AnyFullPixelOperation = requires {
  requires AnyWritePixelOperation<T>;
  requires AnyBulkPixelOperation<T>;
};
