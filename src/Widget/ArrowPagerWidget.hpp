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

#ifndef XCSOAR_ARROW_PAGER_WIDGET_HPP
#define XCSOAR_ARROW_PAGER_WIDGET_HPP

#include "PagerWidget.hpp"
#include "Form/SymbolButton.hpp"
#include "Form/ActionListener.hpp"

#include <functional>

struct ButtonLook;

/**
 * A wrapper for #PagerWidget that adds arrow buttons on the
 * left/bottom for page navigation.
 */
class ArrowPagerWidget : public PagerWidget, ActionListener {
  typedef std::function<void()> PageFlippedCallback;

  enum Buttons {
    PREVIOUS,
    NEXT,
  };

  struct Layout {
    PixelRect previous_button, next_button;
    PixelRect close_button;
    PixelRect main;

    Layout(PixelRect rc);
  };

  ActionListener &action_listener;

  WndSymbolButton previous_button, next_button;
  WndButton close_button;

  PageFlippedCallback page_flipped_callback;

public:
  ArrowPagerWidget(ActionListener &_action_listener,
                   const ButtonLook &look)
    :action_listener(_action_listener),
     previous_button(look), next_button(look), close_button(look) {}

  void SetPageFlippedCallback(PageFlippedCallback &&_page_flipped_callback) {
    assert(!page_flipped_callback);
    assert(_page_flipped_callback);

    page_flipped_callback = std::move(_page_flipped_callback);
  }

  /* virtual methods from Widget */
  virtual PixelSize GetMinimumSize() const override;
  virtual PixelSize GetMaximumSize() const override;
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;
  virtual void Show(const PixelRect &rc) override;
  virtual void Hide() override;
  virtual void Move(const PixelRect &rc) override;
  virtual bool SetFocus() override;
  virtual bool KeyPress(unsigned key_code) override;

private:
  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;

  void OnPageFlipped() {
    if (page_flipped_callback)
      page_flipped_callback();
  }
};

#endif
