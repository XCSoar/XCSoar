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
 * - xcsoar:// URLs trigger internal dialogs (see util/InternalLink.hpp)
 */
class RichTextWidget : public WindowWidget {
  const DialogLook &look;
  const TCHAR *text;

public:
  explicit RichTextWidget(const DialogLook &_look,
                          const TCHAR *_text = nullptr) noexcept
    : look(_look), text(_text) {}

  void SetText(const TCHAR *text) noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
};
