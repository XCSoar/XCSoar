/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_FORM_TABBAR_HPP
#define XCSOAR_FORM_TABBAR_HPP

#include "Screen/ContainerWindow.hpp"
#include "Widget/PagerWidget.hpp"

#include <functional>

#include <tchar.h>

struct DialogLook;
class Bitmap;
class TabDisplay;

/** TabBarControl displays tabs that show/hide the windows
 * associated with each tab.  For example a "Panel" control.
 * It can also display buttons with no associated Window.
 * Supports pre and post- tab click callbacks
 * Each tab must be added via code (not via XML)
 * ToDo: support lazy loading
 */
class TabBarControl : public ContainerWindow {
  PagerWidget pager;

  TabDisplay * tab_display;

public:
  /**
   * Constructor used for stand-alone TabBarControl
   * @param parent
   * @param style
   * @return
   */
  TabBarControl(ContainerWindow &parent, const DialogLook &look,
                PixelRect tab_rc,
                const WindowStyle style,
                bool vertical);

  ~TabBarControl();

  void UpdateLayout(const PixelRect &rc, const PixelRect &tab_rc,
                    bool vertical);

public:
  unsigned AddTab(Widget *widget, const TCHAR *caption,
                  const Bitmap *bmp=nullptr);

public:
  gcc_pure
  PixelSize GetMinimumSize() const;

  gcc_pure
  PixelSize GetMaximumSize() const;

  gcc_pure
  unsigned GetTabCount() const {
    return pager.GetSize();
  }

  gcc_pure
  unsigned GetCurrentPage() const {
    return pager.GetCurrentIndex();
  }

  gcc_pure
  const Widget &GetCurrentWidget() const {
    return pager.GetCurrentWidget();
  }

  void ClickPage(unsigned i);

  void SetCurrentPage(unsigned i);
  void NextPage();
  void PreviousPage();
};

#endif
