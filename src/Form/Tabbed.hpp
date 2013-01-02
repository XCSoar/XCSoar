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

#ifndef XCSOAR_FORM_TABBED_HPP
#define XCSOAR_FORM_TABBED_HPP

#include "Screen/ContainerWindow.hpp"
#include "Util/StaticArray.hpp"
#include "PagerWidget.hpp"

class Widget;

class TabbedControl : public ContainerWindow {
public:
  typedef void (*PageFlippedCallback)();

protected:
  PagerWidget pager;

  PageFlippedCallback page_flipped_callback;

public:
  /**
   * Create an instance without actually creating the Window.  Call
   * set() to do that.
   */
  TabbedControl():page_flipped_callback(NULL) {};

  TabbedControl(ContainerWindow &parent,
                PixelScalar x, PixelScalar y,
                UPixelScalar width, UPixelScalar height,
                const WindowStyle style=WindowStyle());

  virtual ~TabbedControl();

  void SetPageFlippedCallback(PageFlippedCallback _page_flipped_callback) {
    assert(page_flipped_callback == NULL);
    assert(_page_flipped_callback != NULL);

    page_flipped_callback = _page_flipped_callback;
  }

  /**
   * Append a page to the end.  The program will abort when the list
   * of pages is already full.
   */
  void AddPage(Widget *w);

  void AddClient(Window *w);

  unsigned GetTabCount() const {
    return pager.GetSize();
  }

  Widget &GetPage(unsigned i) {
    return pager.GetWidget(i);
  }

  const Widget &GetPage(unsigned i) const {
    return pager.GetWidget(i);
  }

  void PreparePage(unsigned i) {
    pager.PrepareWidget(i);
  }

  unsigned GetCurrentPage() const {
    return pager.GetCurrentIndex();
  }

  const Widget &GetCurrentWidget() const {
    return pager.GetCurrentWidget();
  }

  /**
   * Attempts to display page.  Follows Widget API rules
   * @param i Tab that is requested to be shown.
   * @param click true if Widget's Click() or ReClick() is to be called.
   * @return true if specified page is now visible
   */
  bool SetCurrentPage(unsigned i, bool click = false);

  void NextPage();
  void PreviousPage();

  /**
   * Calls SetCurrentPage() with click=true parameter.
   * Call this to indicate that the user has clicked on the "handle
   * area" of a page (e.g. a tab).  It will invoke Widget::ReClick()
   * if the page was already visible, or Widget::Leave() then Widget::Click()
   * and switch to that page.
   *
   * @return true if the specified page is now visible
   */
  bool ClickPage(unsigned i);

  /**
   * Invoke Widget::Save() on all pages.  Stops when the first one
   * returns false.
   *
   * @return true if all Widget::Save() were successful
   */
  bool Save(bool &changed, bool &require_restart);

  /**
   * Calls Widget::Move() again on the current Widget.  This may be
   * used to trick the Widget into updating its layout.
   */
  void UpdateLayout();

protected:
  virtual void OnResize(UPixelScalar width, UPixelScalar height);
  virtual void OnCreate();
  virtual void OnDestroy();
};

#endif
