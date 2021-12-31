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
