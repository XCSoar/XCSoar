// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

#include <tchar.h>

struct DialogLook;

/**
 * A Widget that displays multi-line text with clickable links.
 *
 * Links are automatically detected:
 * - http:// and https:// URLs open in browser
 * - xcsoar:// URLs trigger internal dialogs (see Dialogs/InternalLink.hpp)
 */
class RichTextWidget : public WindowWidget {
  const DialogLook *look = nullptr;
  const TCHAR *text = nullptr;

public:
  /**
   * Constructor for subclasses that will provide look via GetLook().
   */
  RichTextWidget() noexcept = default;

  /**
   * Constructor with explicit look reference.
   */
  explicit RichTextWidget(const DialogLook &_look,
                          const TCHAR *_text = nullptr) noexcept
    : look(&_look), text(_text) {}

  void SetText(const TCHAR *text) noexcept;

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;

protected:
  /**
   * Override this to provide the look for subclasses.
   * Default returns the look passed to constructor.
   */
  [[gnu::pure]]
  virtual const DialogLook &GetLook() const noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
};
