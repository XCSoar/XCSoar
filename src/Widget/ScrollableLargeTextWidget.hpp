// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LargeTextWidget.hpp"

#include <string>

struct DialogLook;

/**
 * A #LargeTextWidget variant whose height follows its text content.
 *
 * This is intended to be wrapped in #VScrollWidget when the caller
 * wants overflow to be scrollable.
 */
class ScrollableLargeTextWidget final : public LargeTextWidget {
  const DialogLook &look;
  std::string text;

public:
  explicit ScrollableLargeTextWidget(const DialogLook &_look,
                                     const char *_text=nullptr)
    :LargeTextWidget(_look), look(_look),
     text(_text != nullptr ? _text : "") {}

  void SetText(const char *_text);

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
