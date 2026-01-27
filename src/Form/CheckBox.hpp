// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "util/tstring.hpp"

#include <cassert>
#include <functional>

struct DialogLook;
class ContainerWindow;
class Canvas;
struct PixelRect;

/**
 * Draw the checkbox box and optional check mark into the given rectangle.
 * Reusable helper used by `CheckBoxControl::OnPaint` and list renderers.
 */
void DrawCheckBox(Canvas &canvas, const DialogLook &look,
                  const PixelRect &box_rc,
                  bool checked, bool focused, bool pressed,
                  bool enabled) noexcept;

/**
 * This class is used for creating buttons.
 */
class CheckBoxControl : public PaintWindow {
  bool checked, dragging, pressed;

  const DialogLook *look;
  tstring caption;

  using Callback = std::function<void(bool)>;
  Callback callback;

public:
  void Create(ContainerWindow &parent, const DialogLook &look,
              tstring::const_pointer caption,
              const PixelRect &rc,
              const WindowStyle style,
              Callback _callback) noexcept;

  [[gnu::pure]]
  static unsigned GetMinimumWidth(const DialogLook &look, unsigned height,
                                  tstring::const_pointer caption) noexcept;

  /**
   * Set the function that will receive click events.
   */
  void SetCallback(Callback _callback) noexcept {
    assert(!_callback);

    callback = std::move(_callback);
  }

  bool GetState() const {
    return checked;
  }

  void SetState(bool value);

protected:
  void SetPressed(bool value);

  virtual bool OnClicked() noexcept;

  /* virtual methods from class Window */
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  void OnSetFocus() noexcept override;
  void OnKillFocus() noexcept override;
  void OnCancelMode() noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
};
