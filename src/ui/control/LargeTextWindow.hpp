// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/NativeWindow.hpp"
#include "ui/canvas/Color.hpp"

#ifndef USE_WINUSER
#include "Renderer/TextRenderer.hpp"
#endif
#include <string>

class LargeTextWindowStyle : public WindowStyle {
public:
  LargeTextWindowStyle() {
    VerticalScroll();
#ifdef USE_WINUSER
    style |= ES_LEFT | ES_MULTILINE | ES_READONLY;
#endif
  }

  LargeTextWindowStyle(const WindowStyle other):WindowStyle(other) {
    VerticalScroll();
#ifdef USE_WINUSER
    style |= ES_LEFT | ES_MULTILINE | ES_READONLY;
#endif
  }
};

/**
 * A window showing large multi-line text.
 */
class LargeTextWindow : public NativeWindow {
#ifndef USE_WINUSER
  const Font *font = nullptr;

  std::string value;

  /**
   * The first visible line.
   */
  unsigned origin;

  TextRenderer renderer;
#endif

  Color background_color = COLOR_WHITE;
  Color text_color = COLOR_BLACK;
  Color border_color = COLOR_BLACK;

#ifdef USE_WINUSER
  /**
   * Background brush for WM_CTLCOLORSTATIC; lazily created by
   * SetColors().
   */
  HBRUSH background_brush = nullptr;
#endif

public:
#ifdef USE_WINUSER
  ~LargeTextWindow() noexcept;
#endif

  void Create(ContainerWindow &parent, PixelRect rc,
              const LargeTextWindowStyle style=LargeTextWindowStyle());

#ifndef USE_WINUSER
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
#else
  [[gnu::pure]]
  unsigned GetRowCount() const {

    return ::SendMessage(hWnd, EM_GETLINECOUNT, 0, 0);
  }
#endif

  void SetColors(Color _background, Color _text, Color _border) noexcept;

  void SetText(const char *text);

  /**
   * Scroll the contents of a multi-line control by the specified
   * number of lines.
   */
  void ScrollVertically(int delta_lines);

#ifndef USE_WINUSER
  void ScrollTo(unsigned new_origin) noexcept;

protected:
  void OnResize(PixelSize new_size) noexcept override;
  void OnSetFocus() noexcept override;
  void OnKillFocus() noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
#else
protected:
  LRESULT OnChildColor(HDC hdc) noexcept override;
#endif /* !USE_WINUSER */
};
