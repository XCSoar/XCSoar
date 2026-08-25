// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/Button.hpp"

/*
    Menubar button height as a fraction of the screen height
*/
static constexpr unsigned menubar_height_scale_factor = 6;

class ContainerWindow;

/**
 * A container for menu buttons.
 */
class MenuBar {
public:
  static constexpr unsigned MAX_BUTTONS = 64;

protected:
  class Button : public ::Button {
    unsigned event;

  public:
    void SetEvent(unsigned _event) {
      event = _event;
    }

    bool OnClicked() noexcept override;
  };

  Button buttons[MAX_BUTTONS];
  const ButtonLook &look;

public:
  /**
   * @param rc the area the buttons are laid out in; the caller passes
   * the safe area so that the display cutout and the system bars
   * cannot hide a menu button
   */
  MenuBar(ContainerWindow &parent, const PixelRect &rc,
          const ButtonLook &look);

public:
  void ShowButton(unsigned i, bool enabled, const char *text,
                  unsigned event);
  void HideButton(unsigned i);

  bool IsButtonEnabled(unsigned i) const {
    return buttons[i].IsEnabled();
  }

  /**
   * To be called when the parent's size changes.  Moves all buttons
   * to a new position.
   */
  void OnResize(const PixelRect &rc);

  /**
   * Portrait: screen/6, capped at
   * Layout::GetInflightButtonHeight() for gloved use on tall phones.
   * Landscape: screen/5, uncapped.
   */
  [[gnu::pure]]
  static unsigned GetButtonHeight(unsigned screen_height,
                                  bool portrait) noexcept;
};
