// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <span>

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

  /**
   * The space a button leaves around its face on each side.  Two
   * adjacent buttons are therefore two margins apart.
   */
  [[gnu::const]]
  static unsigned GetMargin() noexcept;

  /**
   * The gap a strip of buttons needs towards the edges so that it
   * matches the gap between two neighbours; zero if too small.
   */
  [[gnu::pure]]
  static unsigned GetEdgeMargin(const PixelRect &rc) noexcept;

  /**
   * Corner ellipse of a button face, passed to
   * Canvas::DrawRoundRectangle().  OpenGL treats it as a diameter.
   */
  [[gnu::pure]]
  static unsigned GetCornerDiameter(const PixelRect &face) noexcept;

  void DrawButton(Canvas &canvas, PixelRect rc,
                  ButtonState state) const noexcept;

  /**
   * The dialog drop shadow around the button face inside @p rc.
   * @p neighbors are other buttons' client rectangles in the same
   * coordinates; the shadow stays off their rounded faces.
   * Nothing on a dithered display, and nothing without OpenGL.
   */
  static void DrawFaceShadow(Canvas &canvas, const PixelRect &rc,
                             std::span<const PixelRect> neighbors = {})
    noexcept;

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
