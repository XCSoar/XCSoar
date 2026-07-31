// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SolidWidget.hpp"
#include "ui/canvas/Color.hpp"
#include "util/StaticArray.hxx"

#include <functional>
#include <optional>

/**
 * A #Widget that displays a message and a number of buttons.  It is
 * used by XCSoar to display context-sensitive dialogs in the "bottom
 * area".
 */
class QuestionWidget : public SolidWidget {
  struct Button {
    const char *caption;
    std::function<void()> callback;
  };

  struct MessageColors {
    Color background, text;
  };

  const char *const message;

  StaticArray<Button, 8> buttons;

  std::optional<MessageColors> message_colors;

  bool prepared = false;

  void ApplyMessageColors() noexcept;

public:
  explicit QuestionWidget(const char *_message) noexcept;

  void SetMessage(const char *_message) noexcept;

  /**
   * Paint the message area in these colours, e.g. to show the
   * severity of a warning.  May be called before #Prepare.
   */
  void SetMessageColors(Color background, Color text) noexcept;

  void AddButton(const char *caption,
                 std::function<void()> callback) noexcept {
    buttons.append({caption, std::move(callback)});
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
