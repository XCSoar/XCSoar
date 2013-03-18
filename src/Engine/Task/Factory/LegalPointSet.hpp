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

#ifndef XCSOAR_LEGAL_POINT_SET_HPP
#define XCSOAR_LEGAL_POINT_SET_HPP

#include "TaskPointFactoryType.hpp"

#include <stdint.h>

class LegalPointSet {
  typedef uint32_t T;

  static constexpr T ToMask(TaskPointFactoryType t) {
    return T(1) << unsigned(t);
  }

  template<typename... Args>
  static constexpr T ToMask(TaskPointFactoryType t, Args&&... args) {
    return ToMask(t) | ToMask(args...);
  }

  T value;

  constexpr LegalPointSet(T _value):value(_value) {}

public:
  static constexpr unsigned N = unsigned(TaskPointFactoryType::COUNT);

  constexpr LegalPointSet():value(0) {}

  template<typename... Args>
  constexpr LegalPointSet(TaskPointFactoryType t, Args&&... args)
    :value(ToMask(t, args...)) {}

  LegalPointSet operator|(const LegalPointSet other) const {
    return LegalPointSet(value | other.value);
  }

  LegalPointSet &operator|=(const LegalPointSet other) {
    value |= other.value;
    return *this;
  }

  bool IsEmpty() const {
    return value == 0;
  }

  gcc_pure
  TaskPointFactoryType UncheckedFirst() const {
    T t = 1;

    for (unsigned i = 0;; ++i, t <<= 1)
      if (value & t)
        return TaskPointFactoryType(i);
  }

  void Add(const TaskPointFactoryType t) {
    value |= ToMask(t);
  }

  constexpr bool Contains(TaskPointFactoryType t) const {
    return (value & ToMask(t)) != 0;
  }

  template<typename O>
  void CopyTo(O o) const {
    for (unsigned i = 0; i < N; ++i) {
      const TaskPointFactoryType type = TaskPointFactoryType(i);
      if (Contains(type))
        *o++ = type;
    }
  }
};

#endif
