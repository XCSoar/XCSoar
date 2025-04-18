// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ButtonLook.hpp"
#include "Colors.hpp"
#include "Asset.hpp"

void
ButtonLook::Initialise(const Font &_font)
{
  font = &_font;

  standard.foreground_color = COLOR_BLACK;
  standard.foreground_brush.Create(standard.foreground_color);
  standard.background_color = IsDithered() ? COLOR_WHITE : COLOR_LIGHT_GRAY;
  if (IsDithered()) {
    standard.CreateBorder(COLOR_BLACK, COLOR_BLACK);
  } else if (!HasColors()) {
    standard.CreateBorder(LightColor(COLOR_DARK_GRAY), COLOR_BLACK);
  } else {
    standard.CreateBorder(LightColor(standard.background_color),
                          DarkColor(standard.background_color));
  }

  selected.foreground_color = COLOR_BLACK;
  selected.foreground_brush.Create(selected.foreground_color);
  selected.background_color = IsDithered()
    ? COLOR_VERY_LIGHT_GRAY
    : COLOR_XCSOAR_LIGHT;
  if (IsDithered()) {
    selected.CreateBorder(COLOR_WHITE, COLOR_WHITE);
  } else if (!HasColors()) {
    selected.CreateBorder(LightColor(COLOR_DARK_GRAY), COLOR_BLACK);
  } else {
    selected.CreateBorder(LightColor(selected.background_color),
                          DarkColor(selected.background_color));
  }

  focused.foreground_color = COLOR_WHITE;
  focused.foreground_brush.Create(focused.foreground_color);
  focused.background_color = IsDithered() ? COLOR_BLACK : COLOR_XCSOAR_DARK;
  if (IsDithered()) {
    focused.CreateBorder(COLOR_WHITE, COLOR_WHITE);
  } else if (!HasColors()) {
    focused.CreateBorder(LightColor(COLOR_DARK_GRAY), COLOR_BLACK);
  } else {
    focused.CreateBorder(LightColor(focused.background_color),
                         DarkColor(focused.background_color));
  }

  disabled.color = COLOR_GRAY;
  disabled.brush.Create(disabled.color);
}
