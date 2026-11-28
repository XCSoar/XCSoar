// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SolidWidget.hpp"
#include "ui/canvas/Color.hpp"
#include "util/StaticArray.hxx"

#include <functional>
#include <optional>

class ButtonPanel;

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

  ButtonPanel &GetButtonPanel() noexcept;

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

  /**
   * Take the cursor keys with this button highlighted (the first one
   * if the index is out of range).  The widget is shown over the map,
   * which keeps the keyboard focus and offers keys to KeyPress().
   * Must be called after #Prepare.
   *
   * @return false if there is no button
   */
  bool FocusButton(unsigned index) noexcept;

  /**
   * @return the highlighted button, or -1 if this widget does not
   * have the cursor keys
   */
  int GetFocusedButton() noexcept;

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  /**
   * With the cursor keys: KEY_LEFT / KEY_RIGHT move the highlight,
   * KEY_RETURN hands the keys back and presses the highlighted
   * button, and KEY_ESCAPE drops the highlight and hands the keys
   * back.  Other keys are not handled.
   */
  bool KeyPress(unsigned key_code) noexcept override;
};
