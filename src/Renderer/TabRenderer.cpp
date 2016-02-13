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

#include "TabRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Icon.hpp"
#include "Look/DialogLook.hpp"

void
TabRenderer::Draw(Canvas &canvas, const PixelRect &rc,
                  const DialogLook &look,
                  const TCHAR *caption, const MaskedIcon *icon,
                  bool focused, bool pressed, bool selected) const
{
  canvas.DrawFilledRectangle(rc,
                             look.list.GetBackgroundColor(selected, focused,
                                                          pressed));

  canvas.Select(*look.button.font);
  canvas.SetTextColor(look.list.GetTextColor(selected, focused, pressed));
  canvas.SetBackgroundTransparent();

  if (icon != nullptr) {
    icon->Draw(canvas, rc, selected);
  } else {
    text_renderer.Draw(canvas, rc, caption);
  }
}
