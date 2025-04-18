// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SolidWidget.hpp"
#include "util/StaticArray.hxx"

#include <functional>

#include <tchar.h>

/**
 * A #Widget that displays a message and a number of buttons.  It is
 * used by XCSoar to display context-sensitive dialogs in the "bottom
 * area".
 */
class QuestionWidget : public SolidWidget {
  struct Button {
    const TCHAR *caption;
    std::function<void()> callback;
  };

  const TCHAR *const message;

  StaticArray<Button, 8> buttons;

public:
  explicit QuestionWidget(const TCHAR *_message) noexcept;

  void SetMessage(const TCHAR *_message) noexcept;

  void AddButton(const TCHAR *caption,
                 std::function<void()> callback) noexcept {
    buttons.append({caption, std::move(callback)});
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
