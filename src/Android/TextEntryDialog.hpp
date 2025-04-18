// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/event/Notify.hpp"
#include "Form/Form.hpp"

#include <jni.h>
#include <optional>
#include <string>

class Context;

class AndroidTextEntryDialog {
  // constants from https://developer.android.com/reference/android/text/InputType
  static constexpr int TYPE_CLASS_TEXT = 0x00000001;
  static constexpr int TYPE_CLASS_NUMBER = 0x00000002;
  static constexpr int TYPE_NUMBER_VARIATION_NORMAL = 0x00000000;
  static constexpr int TYPE_TEXT_VARIATION_NORMAL = 0x00000000;
  static constexpr int TYPE_TEXT_VARIATION_PASSWORD = 0x00000080;

  std::optional<std::string> new_value;

  WndForm fake_dialog;

  UI::Notify notify{[this](){
    fake_dialog.SetModalResult(mrCancel);
  }};

public:
  enum class Type : int {
    TEXT = TYPE_CLASS_TEXT|TYPE_TEXT_VARIATION_NORMAL,
    PASSWORD = TYPE_CLASS_TEXT|TYPE_TEXT_VARIATION_PASSWORD,
    INTEGER = TYPE_CLASS_NUMBER|TYPE_NUMBER_VARIATION_NORMAL,
  };

  /**
   * Throws on error.
   */
  AndroidTextEntryDialog() noexcept;

  static void Initialise(JNIEnv *env) noexcept;
  static void Deinitialise(JNIEnv *env) noexcept;

  std::optional<std::string> ShowModal(JNIEnv *env, Context &context,
                                       const char *title, const char *value,
                                       Type type);

  void OnResult(JNIEnv *env, jstring value) noexcept;
};
