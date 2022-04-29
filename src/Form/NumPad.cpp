/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Form/NumPad.hpp"
#include "Form/Form.hpp"
#include "Form/DataField/NumPadAdapter.hpp"
#include "Form/DataField/NumPadAdapter.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"

NumPad::NumPad(NumPadWidgetInterface &_numPadWidgetInterface) : numPadWidgetInterface(
    _numPadWidgetInterface)
{
}

WndForm::CharacterFunction f = nullptr;

void
NumPad::Create(ContainerWindow &parent, const TCHAR *Caption,
               const PixelRect &rc, const WindowStyle style) noexcept
{
  PaintWindow::Create(parent, rc, style);
}

/** Destructor */
NumPad::~NumPad() noexcept
{
}
/**
 * The OnPaint event is called when the button needs to be drawn
 * (derived from PaintWindow)
 */
void
NumPad::OnPaint(Canvas &canvas)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  if (HasFocus()) {
    canvas.Clear(look.caption.background_color);
    canvas.SetTextColor(look.focused.text_color);
  } else
    canvas.Clear(look.caption.inactive_background_color);
  canvas.Select(look.text_font);
  canvas.SetBackgroundTransparent();
  const PixelRect rc = GetClientRect();
  BulkPixelPoint ptNumber(rc.left, rc.top);
  const TCHAR *caption = numPadWidgetInterface.numPadAdapter->GetCaption();
  if (caption != nullptr) {
    canvas.DrawText(ptNumber, caption);
  }

}
bool
NumPad::OnKeyCheck(unsigned key_code) const
{
  return numPadWidgetInterface.numPadAdapter->OnKeyCheck(key_code);
}
bool
NumPad::OnKeyDown(unsigned key_code)
{
  return numPadWidgetInterface.numPadAdapter->OnKeyDown(key_code);
}
