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

#ifndef XCSOAR_FORM_FRAME_HPP
#define XCSOAR_FORM_FRAME_HPP

#include "Screen/PaintWindow.hpp"
#include "Screen/Color.hpp"
#include "Renderer/TextRenderer.hpp"
#include "Util/tstring.hpp"

#include <tchar.h>

struct DialogLook;

class WndFrame : public PaintWindow {
  const DialogLook &look;

  Color caption_color;

  TextRenderer text_renderer;

  tstring text;

public:
  explicit WndFrame(const DialogLook &look);

  WndFrame(ContainerWindow &parent, const DialogLook &look,
           PixelRect rc,
           const WindowStyle style=WindowStyle());

  const DialogLook &GetLook() const {
    return look;
  }

  void SetAlignCenter();
  void SetVAlignCenter();

  void SetText(const TCHAR *_text);

  const TCHAR *GetCaption() const {
    return text.c_str();
  }

  void SetCaption(const TCHAR *_text) {
    SetText(_text);
    text_renderer.InvalidateLayout();
  }

  void SetCaptionColor(const Color &color) {
    caption_color = color;
  }

  gcc_pure
  unsigned GetTextHeight() const;

protected:
  /** from class PaintWindow */
  void OnPaint(Canvas &canvas) override;
};

#endif
