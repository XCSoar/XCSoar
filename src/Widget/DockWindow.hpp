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

#ifndef XCSOAR_DOCK_WINDOW_HPP
#define XCSOAR_DOCK_WINDOW_HPP

#include "Screen/ContainerWindow.hpp"

class Widget;

/**
 * A window that docks one #Widget.  It may be used as a simple
 * container to place one #Widget on the screen.
 */
class DockWindow : public ContainerWindow {
  Widget *widget;

public:
  DockWindow():widget(nullptr) {}

  /**
   * Show the specified #Widget.
   *
   * This method is only legal after this Window has been created.
   *
   * @param widget the new Widget (must not be initialised/prepared)
   */
  void SetWidget(Widget *widget);

  Widget *GetWidget() {
    return widget;
  }

  const Widget *GetWidget() const {
    return widget;
  }

  /**
   * Call Widget::Unprepare().
   */
  void UnprepareWidget();

  /**
   * Delete the given #Widget instance.
   */
  void DeleteWidget();

  /**
   * Call Widget::Move() again.  This should never be needed, as the
   * Widget is supposed to be at the position already.  It's a hack to
   * force RowFormWidget to reconsider the visibility of all rows.
   */
  void MoveWidget();

  /**
   * Wrapper for Widget::Save().
   */
  bool SaveWidget(bool &changed);

protected:
  void OnResize(PixelSize new_size) override;
};

#endif
