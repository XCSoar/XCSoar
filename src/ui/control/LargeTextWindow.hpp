// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/NativeWindow.hpp"
#include "ui/canvas/Color.hpp"
#include "Renderer/TextRenderer.hpp"

#include <string>

class LargeTextWindowStyle : public WindowStyle {
public:
  LargeTextWindowStyle() {
    VerticalScroll();
  }

  LargeTextWindowStyle(const WindowStyle other):WindowStyle(other) {
    VerticalScroll();
  }
};

/**
 * A window showing large multi-line text.
 */
class LargeTextWindow : public NativeWindow {
  const Font *font = nullptr;

  std::string value;

  /**
   * The first visible line.
   */
  unsigned origin;

  TextRenderer renderer;

  Color background_color = COLOR_WHITE;
  Color text_color = COLOR_BLACK;
  Color border_color = COLOR_BLACK;

public:
  void Create(ContainerWindow &parent, PixelRect rc,
              const LargeTextWindowStyle style=LargeTextWindowStyle());

  void SetFont(const Font &_font) {
    AssertThread();

    font = &_font;
  }

  const Font &GetFont() const {
    AssertThread();
    assert(font != nullptr);

    return *font;
  }

  [[gnu::pure]]
  unsigned GetVisibleRows() const;

  [[gnu::pure]]
  unsigned GetRowCount() const;

  void SetColors(Color _background, Color _text, Color _border) noexcept;

  void SetText(const char *text);

  /**
   * Scroll the contents of a multi-line control by the specified
   * number of lines.
   */
  void ScrollVertically(int delta_lines);

  void ScrollTo(unsigned new_origin) noexcept;

protected:
  void OnResize(PixelSize new_size) noexcept override;
  void OnSetFocus() noexcept override;
  void OnKillFocus() noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
};
