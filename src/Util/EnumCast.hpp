/*
 * Copyright (C) 2012 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ENUM_CAST_HPP
#define ENUM_CAST_HPP

#include <stdint.h>

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

#endif
