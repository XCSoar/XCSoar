/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#pragma once

#include "ui/window/NativeWindow.hpp"

#ifndef USE_WINUSER
#include "Renderer/TextRenderer.hpp"
#include "util/tstring.hpp"
#endif

#include <tchar.h>

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

  tstring value;

  /**
   * The first visible line.
   */
  unsigned origin;

  TextRenderer renderer;
#endif

public:
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

  void SetText(const TCHAR *text);

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
#endif /* !USE_WINUSER */
};
