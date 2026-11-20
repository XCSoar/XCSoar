// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>

/** Bounded text editing shared by the keyboard and rotary dialogs. */
class TextEntryBuffer {
public:
  static constexpr std::size_t CAPACITY = 256;

private:
  char text[CAPACITY];
  std::size_t capacity;
  std::size_t cursor;

public:
  /** A zero width selects CAPACITY; widths include the terminator. */
  TextEntryBuffer(const char *value, std::size_t width) noexcept;

  const char *GetValue() const noexcept { return text; }
  std::size_t GetCursor() const noexcept { return cursor; }
  std::size_t GetSequenceLength() const noexcept;
  /** Previous UTF-8 boundary, or zero at the beginning. */
  std::size_t GetPreviousCursor() const noexcept;

  void Clear() noexcept;
  void Home() noexcept { cursor = 0; }
  void End() noexcept;
  bool MoveLeft() noexcept;
  bool MoveRight() noexcept;
  bool Insert(char ch) noexcept;
  bool Replace(char ch) noexcept;
  bool Backspace() noexcept;
  bool Delete() noexcept;

  /** Filter ASCII input, preserving the existing upper-case fallback. */
  static char Filter(unsigned ch, const char *allowed) noexcept;
};
