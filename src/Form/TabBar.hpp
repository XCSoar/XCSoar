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
  typedef std::function<void()> PageFlippedCallback;

  PagerWidget pager;

  TabDisplay * tab_display;

  PageFlippedCallback page_flipped_callback;

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

  void SetPageFlippedCallback(PageFlippedCallback _page_flipped_callback) {
    assert(!page_flipped_callback);
    assert(_page_flipped_callback);

    page_flipped_callback = _page_flipped_callback;
  }

public:
  unsigned AddTab(Widget *widget, const TCHAR *caption, const Bitmap *bmp = NULL);

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

  /**
   * Call Widget::SetFocus() on the current widget.
   */
  void FocusCurrentWidget() {
    pager.GetCurrentWidget().SetFocus();
  }

  void ClickPage(unsigned i);

  void SetCurrentPage(unsigned i);
  void NextPage();
  void PreviousPage();

  bool Save(bool &changed) {
    return pager.Save(changed);
  }

  /**
   * Pass a key press event to the active widget.
   */
  bool InvokeKeyPress(unsigned key_code) {
    return pager.KeyPress(key_code);
  }

  const PixelRect &GetPagerPosition() const {
    return pager.GetPosition();
  }

  gcc_pure
  const TCHAR *GetButtonCaption(unsigned i) const;

protected:
  virtual void OnCreate() override;
  virtual void OnDestroy() override;
  virtual void OnResize(PixelSize new_size) override;

#ifdef HAVE_CLIPPING
  virtual void OnPaint(Canvas &canvas) override;
#endif
};

#endif
