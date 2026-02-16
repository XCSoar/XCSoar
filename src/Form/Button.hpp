// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"

#include <functional>
#include <memory>
#include <tchar.h>

enum class ButtonState : int;
struct ButtonLook;
class ContainerWindow;
class ButtonRenderer;

/**
 * This class is used for creating buttons.
 */
class Button : public PaintWindow {
  bool dragging, down;

  std::unique_ptr<ButtonRenderer> renderer;

public:
  using Callback = std::function<void()>;

private:
  Callback callback;

  /**
   * This flag specifies whether the button is "selected".  The
   * "selected" button in a #ButtonPanel is the button that will be
   * triggered by the #KEY_RETURN.  On some devices without touch
   * screen, cursor keys left/right can be used to navigate the
   * #ButtonPanel.
   */
  bool selected;

public:
  Button(ContainerWindow &parent, const PixelRect &rc,
         WindowStyle style, std::unique_ptr<ButtonRenderer> _renderer,
         Callback _callback) noexcept;

  Button(ContainerWindow &parent, const ButtonLook &look,
         const char *caption, const PixelRect &rc,
         WindowStyle style,
         Callback _callback) noexcept;

  Button();

  ~Button() noexcept override;

  void Create(ContainerWindow &parent, const PixelRect &rc,
              WindowStyle style, std::unique_ptr<ButtonRenderer> _renderer);

  void Create(ContainerWindow &parent, const ButtonLook &look,
              const char *caption, const PixelRect &rc,
              WindowStyle style);

  void Create(ContainerWindow &parent, const PixelRect &rc,
              WindowStyle style, std::unique_ptr<ButtonRenderer> _renderer,
              Callback _callback) noexcept;

  void Create(ContainerWindow &parent, const ButtonLook &look,
              const char *caption, const PixelRect &rc,
              WindowStyle style,
              Callback _callback) noexcept;

  /**
   * Set the object that will receive click events.
   */
  void SetCallback(Callback _callback) noexcept {
    callback = std::move(_callback);
  }

  ButtonRenderer &GetRenderer() noexcept {
    return *renderer;
  }

  const ButtonRenderer &GetRenderer() const noexcept {
    return *renderer;
  }

  /**
   * Set a new caption.  This method is a wrapper for
   * #TextButtonRenderer and may only be used if created with a
   * #TextButtonRenderer instance.
   */
  void SetCaption(const char *caption);

  void SetSelected(bool _selected);

  [[gnu::pure]]
  unsigned GetMinimumWidth() const;

  /**
   * Simulate a click on this button.
   */
  void Click();

protected:
  /**
   * Called when the button is clicked (either by mouse or by
   * keyboard).  The default implementation invokes the OnClick
   * callback.
   */
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

private:
  void SetDown(bool _down);

  [[gnu::pure]]
  ButtonState GetState() const noexcept;
};
