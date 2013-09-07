/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Form/CustomButton.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Canvas.hpp"
#include "Asset.hpp"

static inline ButtonWindowStyle
CustomPainting(ButtonWindowStyle style)
{
  style.EnableCustomPainting();
  return style;
}

WndCustomButton::WndCustomButton(ContainerWindow &Parent,
                                 const DialogLook &_look,
                                 tstring::const_pointer Caption,
                                 const PixelRect &rc,
                                 const ButtonWindowStyle style,
                                 ActionListener &listener, int id)
  :WndButton(Parent, _look.button, Caption, rc,
             CustomPainting(style), listener, id),
   look(_look) {}

void
WndCustomButton::OnPaint(Canvas &canvas)
{
  // Draw focus rectangle
  if (IsDown()) {
    canvas.Clear(look.list.pressed.background_color);
    canvas.SetTextColor(look.list.pressed.text_color);
  } else if (HasFocus()) {
    canvas.Clear(look.focused.background_color);
    canvas.SetTextColor(IsEnabled()
                        ? look.focused.text_color : look.button.disabled.color);
  } else {
    if (HaveClipping())
      canvas.Clear(look.background_brush);
    canvas.SetTextColor(IsEnabled() ? look.text_color : look.button.disabled.color);
  }

  // If button has text on it
  const tstring caption = GetText();
  if (caption.empty())
    return;

  // If button is pressed, offset the text for 3D effect
  PixelRect rc = GetClientRect();

  canvas.Select(*look.button.font);
  canvas.SetBackgroundTransparent();

#ifdef USE_GDI
  const unsigned text_style = DT_CENTER | DT_NOCLIP | DT_WORDBREAK;
#else
  unsigned text_style = GetTextStyle();

  if (IsDithered())
    text_style |= DT_UNDERLINE;
#endif

  canvas.DrawFormattedText(&rc, caption.c_str(), text_style);
}
