// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
struct ButtonLook;
class Canvas;

enum class ButtonState : int {
  /**
   * The button is disabled, i.e. inaccessible.
   */
  DISABLED,

  /**
   * The button is enabled (but not focused).
   */
  ENABLED,

  /**
   * The button is selected, but is not the currently focused control.
   */
  SELECTED,

  /**
   * The button is the currently focused control.
   */
  FOCUSED,

  /**
   * The button is currently pressed down.
   */
  PRESSED,
};

class ButtonFrameRenderer {
  const ButtonLook &look;

public:
  explicit ButtonFrameRenderer(const ButtonLook &_look) noexcept:look(_look) {}

  const ButtonLook &GetLook() const noexcept {
    return look;
  }

  [[gnu::const]]
  static unsigned GetMargin() noexcept;

  void DrawButton(Canvas &canvas, PixelRect rc,
                  ButtonState state) const noexcept;

  [[gnu::pure]]
  PixelRect GetDrawingRect(PixelRect rc, ButtonState state) const noexcept;
};

class ButtonRenderer {
public:
  virtual ~ButtonRenderer() noexcept = default;

  [[gnu::pure]]
  virtual unsigned GetMinimumButtonWidth() const noexcept;

  virtual void DrawButton(Canvas &canvas, const PixelRect &rc,
                          ButtonState state) const noexcept = 0;
};
