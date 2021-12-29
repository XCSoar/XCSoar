/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_ANDROID_DOWNLOAD_MANAGER_HPP
#define XCSOAR_ANDROID_DOWNLOAD_MANAGER_HPP

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

#endif
