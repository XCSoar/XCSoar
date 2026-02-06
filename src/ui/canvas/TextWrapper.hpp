// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

class Canvas;
class Font;

/**
 * A single line of text after word wrapping.
 */
struct WrappedTextLine {
  /** Character offset in original text where this line starts */
  std::size_t start;

  /** Number of characters in this line (excluding trailing spaces removed) */
  std::size_t length;

  /**
   * Get this line's text from the original string.
   */
  [[gnu::pure]]
  std::string_view GetText(std::string_view original) const noexcept {
    return original.substr(start, length);
  }
};

/**
 * Result of wrapping text into lines.
 */
struct WrappedText {
  std::vector<WrappedTextLine> lines;

  /**
   * Get total height in pixels.
   */
  [[gnu::pure]]
  unsigned GetHeight(unsigned line_height) const noexcept {
    return static_cast<unsigned>(lines.size()) * line_height;
  }
};

/**
 * Wrap text into lines that fit within the given width.
 *
 * Breaks at word boundaries (spaces) when possible, otherwise
 * breaks at character boundaries for very long words.
 *
 * @param canvas Canvas with font selected for text measurement
 * @param width Maximum width in pixels for each line
 * @param text The text to wrap
 * @return Wrapped lines with character offsets into original text
 */
[[gnu::pure]]
WrappedText
WrapText(Canvas &canvas, unsigned width, std::string_view text) noexcept;

/**
 * Wrap text using a specific font.
 * Creates a temporary canvas for measurement.
 */
[[gnu::pure]]
WrappedText
WrapText(const Font &font, unsigned width, std::string_view text) noexcept;
