// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/Button.hpp"

/*
    Menubar button height as a fraction of the screen height
*/
static constexpr unsigned menubar_height_scale_factor = 6;

class ContainerWindow;
namespace UI { class SingleWindow; }

/**
 * A container for menu buttons.
 */
class MenuBar {
public:
  static constexpr unsigned MAX_BUTTONS = 64;

protected:
  class Button : public ::Button {
    MenuBar *bar = nullptr;
    unsigned index = 0;
    unsigned event;

  public:
    void Bind(MenuBar &_bar, unsigned _index) noexcept {
      bar = &_bar;
      index = _index;
    }

    void SetEvent(unsigned _event) {
      event = _event;
    }

    bool OnClicked() noexcept override;

    void OnPaint(Canvas &canvas) noexcept override;
  };

  Button buttons[MAX_BUTTONS];
  const ButtonLook &look;

public:
  /**
   * @param rc the area the buttons are laid out in (the InfoBox
   * stretch rect: screen border on stretched edges, safe area on
   * the others)
   */
  MenuBar(ContainerWindow &parent, const PixelRect &rc,
          const ButtonLook &look);

public:
  void ShowButton(unsigned i, bool enabled, const char *text,
                  unsigned event);
  void HideButton(unsigned i);

  /** Keep visible menu buttons above page overlays and below dialogs. */
  void BringToTop(UI::SingleWindow &parent) noexcept;

  bool IsButtonEnabled(unsigned i) const {
    return buttons[i].IsEnabled();
  }

  /**
   * Is the menu currently on screen, i.e. is at least one button
   * visible?
   */
  [[gnu::pure]]
  bool IsAnyButtonVisible() const noexcept;

  /**
   * To be called when the parent's size changes.  Moves all buttons
   * to a new position.
   */
  void OnResize(const PixelRect &rc);

  /**
   * Shorten @p rc so it ends above visible menu buttons that touch its
   * bottom edge.  Buttons elsewhere on the screen do not affect it.
   */
  [[gnu::pure]]
  PixelRect GetRemainingRectAboveBottomButtons(PixelRect rc) const noexcept;

  /**
   * Portrait: screen/6, capped at
   * Layout::GetInflightButtonHeight() for gloved use on tall phones.
   * Landscape: screen/5, uncapped.
   */
  [[gnu::pure]]
  static unsigned GetButtonHeight(unsigned screen_height,
                                  bool portrait) noexcept;
};
