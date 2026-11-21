// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TopographyLook.hpp"
#include "FontDescription.hpp"
#include "Screen/Layout.hpp"
#include "util/Macros.hpp"

void
TopographyLook::Initialise()
{
  static constexpr unsigned points[] = { 10, 12, 14 };
  static_assert(ARRAY_SIZE(points) == unsigned(LabelSize::COUNT),
                "Label font sizes must match LabelSize::COUNT");

  for (unsigned i = 0; i < unsigned(LabelSize::COUNT); ++i) {
    regular_label_font[i].Load(FontDescription(Layout::FontScale(points[i]),
                                               false, false));
    important_label_font[i].Load(FontDescription(Layout::FontScale(points[i]),
                                                 true, false));
  }
}
