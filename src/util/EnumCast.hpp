// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <cstdint>

template<typename T, unsigned size>
struct EnumCastInternal {};

template<typename T, typename U>
struct EnumCastHelper {
  constexpr U &operator()(T &x) const {
    return (U &)x;
  }
};

template<typename T>
struct EnumCastInternal<T, sizeof(int)>
  : public EnumCastHelper<T, int> {};

template<typename T>
struct EnumCastInternal<T, sizeof(uint8_t)>
  : public EnumCastHelper<T, uint8_t> {};

template<typename T>
struct EnumCastInternal<T, sizeof(uint16_t)>
  : public EnumCastHelper<T, uint16_t> {};

/**
 * This class helps casting an enum reference safely into the
 * according integer reference type.  May be used as an optimisation
 * to pass enum references to overloaded integer functions.
 */
template<typename T>
struct EnumCast : public EnumCastInternal<T, sizeof(T)> {};
