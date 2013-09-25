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

#ifndef XCSOAR_RESOURCE_ID_HPP
#define XCSOAR_RESOURCE_ID_HPP

#include "Util/ConstBuffer.hpp"

/**
 * The identifier for a resource to be passed to
 * ResourceLoader::Load() or other resource-loading functions.
 */
class ResourceId {
#if defined(WIN32) || defined(ANDROID)
  unsigned id;
#else
  const void *begin, *size;
#endif

public:
  ResourceId() = default;

#if defined(WIN32) || defined(ANDROID)
  constexpr explicit ResourceId(unsigned _id)
    :id(_id) {}
#else
  constexpr explicit ResourceId(const void *_begin, const void *_size)
    :begin(_begin), size(_size) {}
#endif

  static constexpr ResourceId Null() {
#if defined(WIN32) || defined(ANDROID)
    return ResourceId(0);
#else
    return ResourceId(nullptr, nullptr);
#endif
  }

  constexpr bool IsDefined() const {
#if defined(WIN32) || defined(ANDROID)
    return id != 0;
#else
    return begin != nullptr;
#endif
  }

#if defined(WIN32) || defined(ANDROID)
  constexpr explicit operator unsigned() const {
    return id;
  }
#else
  gcc_pure
  operator ConstBuffer<void>() const {
    return ConstBuffer<void>(begin, (size_t)size);
  }
#endif

  constexpr bool operator==(ResourceId other) const {
#if defined(WIN32) || defined(ANDROID)
    return id == other.id;
#else
    return begin == other.begin;
#endif
  }

  constexpr bool operator!=(ResourceId other) const {
#if defined(WIN32) || defined(ANDROID)
    return id != other.id;
#else
    return begin != other.begin;
#endif
  }
};

#endif
