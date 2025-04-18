// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget.hpp"

#include <cassert>
#include <memory>

class Window;

/**
 * A simple wrapper to turn a Window into a Widget.  The Window will
 * not be deleted in the destructor.
 */
class WindowWidget : public NullWidget {
  std::unique_ptr<Window> window;

public:
  /**
   * Initialise an empty instance.  Call SetWindow() to finish it.
   */
  WindowWidget() noexcept;

  /**
   * Initialise an instance with an existing #Window pointer.  It must
   * be hidden or undefined (i.e. not yet created).  However, it must
   * be created before the #Widget gets shown.
   */
  WindowWidget(std::unique_ptr<Window> _window) noexcept;

  ~WindowWidget() noexcept override;

protected:
  bool IsDefined() const noexcept {
    return window != nullptr;
  }

  void SetWindow(std::unique_ptr<Window> &&_window) noexcept;

  /**
   * Deletes the window object
   */
  void DeleteWindow() noexcept;

public:
  const Window &GetWindow() const noexcept {
    assert(window != nullptr);
    return *window;
  }

  Window &GetWindow() noexcept {
    assert(window != nullptr);
    return *window;
  }

  /* virtual methods from class Widget */
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool HasFocus() const noexcept override;
};
