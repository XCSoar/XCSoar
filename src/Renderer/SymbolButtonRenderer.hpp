/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_SYMBOL_BUTTON_RENDERER_HPP
#define XCSOAR_SYMBOL_BUTTON_RENDERER_HPP

#include "ButtonRenderer.hpp"
#include "Util/StaticString.hxx"

/**
 * A #ButtonRenderer instance that renders a regular button frame and
 * a symbol.
 */
class SymbolButtonRenderer : public ButtonRenderer {
  ButtonFrameRenderer frame_renderer;

  const StaticString<16> caption;

public:
  SymbolButtonRenderer(const ButtonLook &_look,
                       StaticString<64>::const_pointer _caption)
    :frame_renderer(_look), caption(_caption) {}

  const ButtonLook &GetLook() const {
    return frame_renderer.GetLook();
  }

  StaticString<64>::const_pointer GetCaption() const {
    return caption;
  }

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  bool enabled, bool focused, bool pressed) const override;

private:
  void DrawSymbol(Canvas &canvas, PixelRect rc,
                  bool enabled, bool focused, bool pressed) const;
};

#endif
