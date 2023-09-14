// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StringBuffer.hxx"

#include <functional>
#include <tchar.h>

typedef std::function<const TCHAR *(const TCHAR *)> AllowedCharacters;

bool
TextEntryDialog(TCHAR *text, size_t size,
                const TCHAR *caption=nullptr,
                AllowedCharacters ac=AllowedCharacters(),
                bool default_shift_state = true);

template<size_t N>
static inline bool
TextEntryDialog(BasicStringBuffer<TCHAR, N> &text,
                const TCHAR *caption=NULL,
                AllowedCharacters accb=AllowedCharacters(),
                bool default_shift_state = true)
{
  return TextEntryDialog(text.data(), text.capacity(),
                         caption, accb, default_shift_state);
}

template<size_t N>
static inline bool
TextEntryDialog(BasicStringBuffer<TCHAR, N> &text,
                const TCHAR *caption,
                bool default_shift_state)
{
  AllowedCharacters accb=AllowedCharacters();
  return TextEntryDialog(text.data(), text.capacity(),
                         caption, accb, default_shift_state);
}

void
KnobTextEntry(TCHAR *text, size_t width,
              const TCHAR *caption);

bool
TouchTextEntry(TCHAR *text, size_t size,
               const TCHAR *caption=nullptr,
               AllowedCharacters ac=AllowedCharacters(),
               bool default_shift_state = true);
