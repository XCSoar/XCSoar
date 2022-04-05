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

#ifndef SRC_FORM_NUMPAD_HPP_
#define SRC_FORM_NUMPAD_HPP_

#include "ui/window/PaintWindow.hpp"
#include "Form/DataField/NumPadWidgetInterface.hpp"
#include <tchar.h>

/*
 * Window to display the in dialog numPad
 * Works together with NumPadWidget
 */
class NumPad:  public PaintWindow {
public:
	NumPad(NumPadWidgetInterface &_numPadWidgetInterface);
  void Create(ContainerWindow &parent,
              const TCHAR *Caption,
              const PixelRect &rc, 
              const WindowStyle style) noexcept;
  /** Destructor */
  ~NumPad() noexcept;
  private:
  NumPadWidgetInterface &numPadWidgetInterface;
   /**
   * The OnPaint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  /* virtual methods from class Window */
  bool OnKeyCheck(unsigned key_code) const override;
  bool OnKeyDown(unsigned key_code) override;
  void OnSetFocus() override;
  void OnKillFocus() override{
    PaintWindow::OnKillFocus();
  }

  void OnCancelMode() override;
  void ClearFocus() noexcept override;

  void OnPaint(Canvas &canvas) override;


};

#endif /* SRC_FORM_NUMPAD_HPP_ */
