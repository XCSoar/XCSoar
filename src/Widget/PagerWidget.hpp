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

#ifndef XCSOAR_PAGER_WIDGET_HPP
#define XCSOAR_PAGER_WIDGET_HPP

#include "Widget.hpp"
#include "ui/dim/Rect.hpp"

#include <boost/container/static_vector.hpp>

#include <functional>
#include <memory>

/**
 * A #Widget that host multiple other widgets, displaying one at a
 * time.
 */
class PagerWidget : public Widget {
  typedef std::function<void()> PageFlippedCallback;

  struct Child {
    std::unique_ptr<Widget> widget;

    /**
     * Has Widget::Prepare() been called?
     */
    bool prepared = false;

    Child(std::unique_ptr<Widget> &&_widget) noexcept
      :widget(std::move(_widget)) {}
    ~Child() noexcept;
  };

  bool initialised = false, prepared, visible;

  ContainerWindow *parent;
  PixelRect position;

  unsigned current;
  boost::container::static_vector<Child, 32u> children;

  PageFlippedCallback page_flipped_callback;

public:
  ~PagerWidget() noexcept override;

  void SetPageFlippedCallback(PageFlippedCallback &&_page_flipped_callback) noexcept {
    assert(!page_flipped_callback);
    assert(_page_flipped_callback);

    page_flipped_callback = std::move(_page_flipped_callback);
  }

  const PixelRect &GetPosition() const noexcept {
    return position;
  }

  /**
   * Append a child #Widget to the end.  The program will abort when
   * the list of pages is already full.
   *
   * @param w a #Widget that is "uninitialised"; it will be deleted by
   * this class
   */
  void Add(std::unique_ptr<Widget> w) noexcept;

  /**
   * Delete all widgets.  This may only be called after Unprepare().
   */
  void Clear() noexcept;

  unsigned GetSize() const noexcept {
    return children.size();
  }

  const Widget &GetWidget(unsigned i) const noexcept {
    return *children[i].widget;
  }

  Widget &GetWidget(unsigned i) noexcept {
    return *children[i].widget;
  }

  /**
   * Ensure that the specified #Widget is "prepared".  Calling this
   * method is only legal if this #PagerWidget has been prepared
   * already, too.
   */
  void PrepareWidget(unsigned i) noexcept;

  unsigned GetCurrentIndex() const noexcept {
    assert(!children.empty());

    return current;
  }

  const Widget &GetCurrentWidget() const noexcept {
    assert(!children.empty());

    return GetWidget(current);
  }

  Widget &GetCurrentWidget() noexcept {
    assert(!children.empty());

    return GetWidget(current);
  }

  /**
   * Attempts to display page.  Follows Widget API rules
   * @param i Tab that is requested to be shown.
   * @param click true if Widget's Click() or ReClick() is to be called.
   * @return true if specified page is now visible
   */
  bool SetCurrent(unsigned i, bool click=false) noexcept;

  /**
   * Display the next page.
   *
   * @param wrap wrap around if on the last page
   * @return true if the page was switched, false if the current page
   * hasn't changed.
   */
  bool Next(bool wrap) noexcept;

  /**
   * Display the previous page.
   *
   * @param wrap wrap around if on the last page
   * @return true if the page was switched, false if the current page
   * hasn't changed.
   */
  bool Previous(bool wrap) noexcept;

  /**
   * Calls SetCurrentPage() with click=true parameter.
   * Call this to indicate that the user has clicked on the "handle
   * area" of a page (e.g. a tab).  It will invoke Widget::ReClick()
   * if the page was already visible, or Widget::Leave() then Widget::Click()
   * and switch to that page.
   *
   * @return true if the specified page is now visible
   */
  bool ClickPage(unsigned i) noexcept {
    return SetCurrent(i, true);
  }

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  bool Save(bool &changed) noexcept override;
  bool Click() noexcept override;
  void ReClick() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  bool Leave() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;

protected:
  virtual void OnPageFlipped() noexcept;
};

#endif
