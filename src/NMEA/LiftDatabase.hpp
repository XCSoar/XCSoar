// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <type_traits>
#include <array>

class LiftDatabase : public std::array<double, 36> {
public:
  void Clear() {
    fill(0);
  }
};

static_assert(std::is_trivial<LiftDatabase>::value, "type is not trivial");

#ifdef __clang__
#pragma GCC diagnostic push
/* necessary with clang due to bug in libstdc++ */
#pragma GCC diagnostic ignored "-Wmismatched-tags"
#endif

namespace std {
  template<>
  class tuple_size<LiftDatabase> : public integral_constant<size_t, 36> {
  };
}

#ifdef __clang__
#pragma GCC diagnostic pop
#endif
