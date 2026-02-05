// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TopWindow.hpp"

#include <forward_list>
#include <cassert>

class WndForm;

namespace UI {

struct Event;

/**
 * The single top-level window of an application.  When it is closed,
 * the process quits.
 */
class SingleWindow : public TopWindow {
#ifdef USE_WINUSER
  static constexpr const char *class_name = _T("XCSoarMain");
#endif

  std::forward_list<WndForm *> dialogs;

public:
  using TopWindow::TopWindow;

#ifdef USE_WINUSER
  /**
   * Register the WIN32 window class.
   */
  static bool RegisterClass(HINSTANCE hInstance) noexcept;
#endif

  /**
   * Throws on error.
   */
  void Create(const char *text, PixelSize size,
              TopWindowStyle style=TopWindowStyle()) {
#ifdef USE_WINUSER
    TopWindow::Create(class_name, text, size, style);
#else
    TopWindow::Create(text, size, style);
#endif
  }

  void AddDialog(WndForm *dialog) noexcept;
  void RemoveDialog(WndForm *dialog) noexcept;

  /**
   * Forcefully cancel the top-most dialog.
   */
  void CancelDialog() noexcept;

  [[gnu::pure]]
  bool HasDialog() const noexcept {
    return !dialogs.empty();
  }

  /**
   * Check whether the specified dialog is the top-most one.
   */
  [[gnu::pure]]
  bool IsTopDialog(const WndForm &dialog) const noexcept {
    assert(HasDialog());

    return &dialog == dialogs.front();
  }

  WndForm &GetTopDialog() noexcept {
    assert(HasDialog());

    return *dialogs.front();
  }

#ifndef USE_WINUSER
protected:
  [[gnu::pure]]
  bool FilterMouseEvent(PixelPoint pt, Window *allowed) const noexcept;
#endif

public:
  /**
   * Check if the specified event should be allowed.  An event may be
   * rejected when a modal dialog is active, and the event should go
   * to a window outside of the dialog.
   */
  [[gnu::pure]]
  bool FilterEvent(const Event &event, Window *allowed) const noexcept;

protected:
  bool OnClose() noexcept override;
  void OnDestroy() noexcept override;
  void OnResize(PixelSize new_size) noexcept override;
};

} // namespace UI
