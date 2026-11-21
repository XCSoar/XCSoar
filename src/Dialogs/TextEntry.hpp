// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "util/StringBuffer.hxx"

#include <functional>
#include <cstddef>

class Canvas;
struct PixelRect;

/**
 * The text-entry field used by both the on-screen keyboard and
 * HighScore entry. The blinking mark is one underscore of the text font.
 */
class TextEntryField : public PaintWindow {
  const char *value = "";
  std::size_t cursor = 0;
  bool cursor_on = true;
  UI::PeriodicTimer cursor_timer{[this]{ OnCursorTimer(); }};

  void OnCursorTimer() noexcept;

public:
  /**
   * @p text must outlive the next paint. @p offset is a UTF-8 boundary.
   * Shows the mark and restarts the blink.
   */
  void SetValue(const char *text, std::size_t offset) noexcept;

protected:
  void OnCreate() override;
  void OnDestroy() noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
};

void
DrawTextEntryField(Canvas &canvas, const PixelRect &rc,
                   const char *value, std::size_t cursor,
                   bool cursor_on) noexcept;
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

bool
KnobTextEntry(char *text, size_t width,
              const char *caption,
              AllowedCharacters ac = AllowedCharacters());

/**
 * @param use_system_keyboard use the operating system's on-screen
 * keyboard instead of XCSoar's own one; ignored on platforms which
 * have none
 */
bool
TouchTextEntry(char *text, size_t size,
               const char *caption=nullptr,
               AllowedCharacters ac=AllowedCharacters(),
               bool default_shift_state = true,
               bool use_system_keyboard = false);
