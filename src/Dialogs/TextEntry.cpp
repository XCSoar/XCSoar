// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextEntry.hpp"
#include "DialogSettings.hpp"
#include "UIGlobals.hpp"
#include "Asset.hpp"
#include "ui/event/TextInput.hpp"

bool
TextEntryDialog(char *text, size_t width,
                const char *caption, AllowedCharacters accb,
                bool default_shift_state)
{
  switch (UIGlobals::GetDialogSettings().text_input_style) {
  case DialogSettings::TextInputStyle::Default:
  case DialogSettings::TextInputStyle::SystemKeyboard:
    /* the keyboard of the operating system (iOS) is much more capable
       than ours, so use it wherever there is one */
    if (HasPointer() && UI::TextInput::HasScreenKeyboard())
      return TouchTextEntry(text, width, caption, accb, default_shift_state,
                            true);

    /* no system keyboard on this platform: fall back to our own */
    [[fallthrough]];

  case DialogSettings::TextInputStyle::Keyboard:
    if (HasPointer())
      return TouchTextEntry(text, width, caption, accb, default_shift_state);
    else
      return KnobTextEntry(text, width, caption);

  case DialogSettings::TextInputStyle::HighScore:
    return KnobTextEntry(text, width, caption);
  }

  return false;
}
