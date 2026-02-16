// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StringBuffer.hxx"

#include <functional>
#include <tchar.h>

typedef std::function<const char *(const char *)> AllowedCharacters;

bool
TextEntryDialog(char *text, size_t size,
                const char *caption=nullptr,
                AllowedCharacters ac=AllowedCharacters(),
                bool default_shift_state = true);

template<size_t N>
static inline bool
TextEntryDialog(BasicStringBuffer<char, N> &text,
                const char *caption=NULL,
                AllowedCharacters accb=AllowedCharacters(),
                bool default_shift_state = true)
{
  return TextEntryDialog(text.data(), text.capacity(),
                         caption, accb, default_shift_state);
}

template<size_t N>
static inline bool
TextEntryDialog(BasicStringBuffer<char, N> &text,
                const char *caption,
                bool default_shift_state)
{
  AllowedCharacters accb=AllowedCharacters();
  return TextEntryDialog(text.data(), text.capacity(),
                         caption, accb, default_shift_state);
}

void
KnobTextEntry(char *text, size_t width,
              const char *caption);

bool
TouchTextEntry(char *text, size_t size,
               const char *caption=nullptr,
               AllowedCharacters ac=AllowedCharacters(),
               bool default_shift_state = true);
