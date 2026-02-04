// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "QuickGuideLinkWindow.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "Look/FontDescription.hpp"
#include "Look/DialogLook.hpp"
#include "Form/CheckBox.hpp"
#include "Renderer/TextRenderer.hpp"
#include "UIGlobals.hpp"

#include <tchar.h>
#include <winuser.h>

/**
 * Helper class for laying out Quick Guide pages.
 * Provides common drawing utilities for text blocks and links.
 */
class QuickGuideLayoutContext {
  Canvas *canvas;
  const PixelRect rc;
  QuickGuideLinkWindow *window;

  const DialogLook &look;
  Font font_mono;
  TextRenderer renderer;

public:
  const int margin;
  const int x;
  const int x_indent;
  const int right;
  int y;

  QuickGuideLayoutContext(Canvas *_canvas, const PixelRect &_rc,
                          QuickGuideLinkWindow *_window) noexcept
    : canvas(_canvas),
      rc(_rc),
      window(_window),
      look(UIGlobals::GetDialogLook()),
      margin(Layout::FastScale(10)),
      x(rc.left + margin),
      x_indent(x + Layout::FastScale(17)),
      right(rc.right - margin),
      y(rc.top + margin)
  {
    font_mono.Load(FontDescription(Layout::VptScale(10), false, false, true));

    if (canvas != nullptr) {
      canvas->Select(look.text_font);
      canvas->SetBackgroundTransparent();
      canvas->SetTextColor(COLOR_BLACK);
    }
  }

  [[gnu::pure]]
  const Font &GetTextFont() const noexcept { return look.text_font; }

  [[gnu::pure]]
  const Font &GetSmallFont() const noexcept { return look.small_font; }

  [[gnu::pure]]
  const Font &GetBoldFont() const noexcept { return look.bold_font; }

  [[gnu::pure]]
  const Font &GetMonoFont() const noexcept { return font_mono; }

  /**
   * Draw a text block at the specified left position.
   * Updates y position after drawing.
   * @return Height of the drawn text
   */
  unsigned DrawTextBlock(const Font &font, int left, const TCHAR *text,
                         unsigned format = DT_LEFT) noexcept {
    if (canvas != nullptr) {
      canvas->Select(font);
      PixelRect text_rc{left, y, right, rc.bottom};
      return canvas->DrawFormattedText(text_rc, text, format);
    }

    const int width = right > left ? right - left : 0;
    return renderer.GetHeight(font, width, text);
  }

  /**
   * Draw a text block at the default left margin.
   */
  unsigned DrawTextBlock(const TCHAR *text) noexcept {
    return DrawTextBlock(look.text_font, x, text);
  }

  /**
   * Draw a clickable link at the indented position.
   * @return Height of the drawn link
   */
  template<typename LinkAction>
  unsigned DrawLinkLine(LinkAction link, const TCHAR *text) noexcept {
    if (canvas != nullptr && window != nullptr) {
      canvas->Select(font_mono);
      PixelRect link_rc{x_indent, y, right, rc.bottom};
      return window->DrawLink(*canvas, static_cast<std::size_t>(link),
                              link_rc, text);
    }

    const int width = right > x_indent ? right - x_indent : 0;
    return renderer.GetHeight(font_mono, width, text);
  }

  /**
   * Draw a clickable link at a custom left position.
   */
  template<typename LinkAction>
  unsigned DrawLinkLine(LinkAction link, int left,
                        const TCHAR *text) noexcept {
    if (canvas != nullptr && window != nullptr) {
      canvas->Select(font_mono);
      PixelRect link_rc{left, y, right, rc.bottom};
      return window->DrawLink(*canvas, static_cast<std::size_t>(link),
                              link_rc, text);
    }

    const int width = right > left ? right - left : 0;
    return renderer.GetHeight(font_mono, width, text);
  }

  /**
   * Draw a numbered item prefix (e.g., "1.)").
   */
  void DrawNumber(int number) noexcept {
    if (canvas != nullptr) {
      TCHAR buf[8];
      _stprintf(buf, _T("%d.)"), number);
      canvas->DrawText({x, y}, buf);
    }
  }

  /**
   * Draw a checkbox indicator at the current position.
   * Uses the text font height for sizing.
   */
  void DrawCheckbox(bool checked) noexcept {
    if (canvas != nullptr) {
      const int size = look.text_font.GetHeight();
      PixelRect box_rc{x, y, x + size, y + size};
      ::DrawCheckBox(*canvas, look, box_rc, checked, false, false, true);
    }
  }

  /**
   * Get the x position for text after a checkbox.
   */
  [[gnu::pure]]
  int GetCheckboxTextX() const noexcept {
    return x + look.text_font.GetHeight() + Layout::FastScale(8);
  }

  /**
   * Get the final height after layout.
   */
  [[gnu::pure]]
  unsigned GetHeight() const noexcept {
    return static_cast<unsigned>(y);
  }
};
