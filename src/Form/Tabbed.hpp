/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_FORM_TABBED_HPP
#define XCSOAR_FORM_TABBED_HPP

#include "Screen/ContainerWindow.hpp"
#include "Util/StaticArray.hpp"

class Widget;

class TabbedControl : public ContainerWindow {
protected:
  struct Page {
    Widget *widget;

    /**
     * Has Widget::Prepare() been called?
     */
    bool prepared;

    Page() = default;

    gcc_constexpr_ctor
    Page(Widget *_widget):widget(_widget), prepared(false) {}
  };

  unsigned current;
  StaticArray<Page, 32> tabs;

public:
  /**
   * Create an instance without actually creating the Window.  Call
   * set() to do that.
   */
  TabbedControl() = default;

  TabbedControl(ContainerWindow &parent,
                PixelScalar x, PixelScalar y,
                UPixelScalar width, UPixelScalar height,
                const WindowStyle style=WindowStyle());

  /**
   * Append a page to the end.  The program will abort when the list
   * of pages is already full.
   */
  void AddPage(Widget *w);

  void AddClient(Window *w);

  unsigned GetTabCount() const {
    return tabs.size();
  }

  unsigned GetCurrentPage() const {
    return current;
  }

  const Widget *GetCurrentWidget() const {
    return tabs[current].widget;
  }

  void SetCurrentPage(unsigned i);
  void NextPage();
  void PreviousPage();

protected:
  virtual bool on_resize(UPixelScalar width, UPixelScalar height);
  virtual bool on_destroy();
};

#endif
