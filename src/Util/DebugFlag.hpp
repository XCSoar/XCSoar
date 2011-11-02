/*
 * Copyright (C) 2011 Max Kellermann <max@duempel.org>
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

#ifndef XCSOAR_UTIL_DEBUG_FLAG_HPP
#define XCSOAR_UTIL_DEBUG_FLAG_HPP

#include "Compiler.h"

/**
 * A flag that is only available in the debug build (#ifndef NDEBUG).
 * It is initialised to false.  In the release build, this type
 * disappears without any overhead.
 */
struct DebugFlag {
#ifdef NDEBUG
  DebugFlag() = default;

  gcc_constexpr_ctor
  DebugFlag(bool _value) {}
#else
  bool value;

  gcc_constexpr_ctor
  DebugFlag():value(false) {}

  gcc_constexpr_ctor
  DebugFlag(bool _value):value(_value) {}

  operator bool() const {
    return value;
  }
#endif

  DebugFlag &operator=(bool _value) {
#ifndef NDEBUG
    value = _value;
#endif
    return *this;
  }
};

#endif
