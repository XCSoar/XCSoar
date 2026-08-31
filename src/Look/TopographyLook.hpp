// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Font.hpp"

#include <array>
#include <cassert>

struct TopographyLook {
  enum class LabelSize : unsigned {
    SMALL,
    MEDIUM,
    LARGE,
    COUNT
  };

  std::array<Font, unsigned(LabelSize::COUNT)> regular_label_font;

  /** for big/medium cities */
  std::array<Font, unsigned(LabelSize::COUNT)> important_label_font;

  void Initialise();

  [[gnu::pure]]
  const Font &GetLabelFont(bool important, LabelSize size) const noexcept {
    assert(size < LabelSize::COUNT);
    const auto &fonts = important
      ? important_label_font
      : regular_label_font;
    return fonts[unsigned(size)];
  }
};
