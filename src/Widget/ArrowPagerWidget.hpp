// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PagerWidget.hpp"
#include "Form/Button.hpp"

#include <cassert>
#include <functional>
#include <memory>

struct ButtonLook;

/**
 * A wrapper for #PagerWidget that adds arrow buttons on the
 * left/bottom for page navigation.
 *
 * Supports an optional page-advance guard: if set, forward navigation
 * (next button, RIGHT key, swipe) is blocked unless the callback
 * returns true for the current page.
 */
class ArrowPagerWidget : public PagerWidget {
public:
  /**
   * Callback to check whether forward navigation is allowed from
   * the given page index.  Return true to allow, false to block.
   */
  using CanAdvanceCallback = std::function<bool(unsigned current_page)>;

private:
  enum Buttons {
    PREVIOUS,
    NEXT,
  };

  struct Layout {
    PixelRect previous_button, next_button;
    PixelRect close_button;
    PixelRect main;
    PixelRect extra;

    Layout(const ButtonLook &look, PixelRect rc,
           const Widget *extra) noexcept;
  };

  const ButtonLook &look;
  const std::function<void()> close_callback;

  /**
   * An optional #Widget that is shown in the remaining area in the
   * buttons row/column.  This object will be deleted automatically.
   */
  const std::unique_ptr<Widget> extra;

  Button previous_button, next_button;
  Button close_button;

  /** Caption for the close button, applied during Prepare(). */
  const TCHAR *pending_close_caption = nullptr;

  /** Optional guard that blocks forward page navigation */
  CanAdvanceCallback can_advance_callback;

public:
  ArrowPagerWidget(const ButtonLook &_look,
                   std::function<void()> _close_callback,
                   std::unique_ptr<Widget> _extra=nullptr) noexcept
    :look(_look),
     close_callback(std::move(_close_callback)),
     extra(std::move(_extra)) {}

  Widget &GetExtra() noexcept {
    assert(extra != nullptr);

    return *extra;
  }

  /**
   * Set a callback to guard forward page navigation.  When set,
   * Next() via button, keyboard, or swipe will only proceed if the
   * callback returns true for the current page index.
   */
  void SetCanAdvanceCallback(CanAdvanceCallback callback) noexcept {
    can_advance_callback = std::move(callback);
  }

  /**
   * Check whether advancing from the current page is allowed.
   * @return true if no guard is set or the guard allows it
   */
  [[gnu::pure]]
  bool CanAdvance() const noexcept {
    if (!can_advance_callback)
      return true;
    return can_advance_callback(GetCurrentIndex());
  }

  /* virtual methods from Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;

  /**
   * Re-evaluate whether the next button should be enabled.
   * Call this after the advance-guard state changes (e.g. a
   * prerequisite checkbox was toggled).
   */
  void UpdateNextButtonState() noexcept;

  /**
   * Change the label of the close button.  If the button has not
   * been created yet, the caption is stored and applied during
   * Prepare().
   */
  void SetCloseButtonCaption(const TCHAR *caption) noexcept {
    if (close_button.IsDefined())
      close_button.SetCaption(caption);
    else
      pending_close_caption = caption;
  }

protected:
  void OnPageFlipped() noexcept override;

private:
  void UpdateButtons() noexcept;
};
