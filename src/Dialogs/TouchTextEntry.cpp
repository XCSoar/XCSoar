// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/TextEntry.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Widget/KeyboardWidget.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "util/StringCompare.hxx"
#include "util/TruncateString.hpp"

#include <algorithm>

static WndProperty *editor;
static KeyboardWidget *kb = NULL;

static AllowedCharacters AllowedCharactersCallback;

static constexpr size_t MAX_TEXTENTRY = 40;
static unsigned int cursor = 0;
static size_t max_width;
static char edittext[MAX_TEXTENTRY];

static void
UpdateAllowedCharacters()
{
  if (AllowedCharactersCallback)
    kb->SetAllowedCharacters(AllowedCharactersCallback(edittext));
}

static void
UpdateTextboxProp()
{
  editor->SetText(edittext);

  UpdateAllowedCharacters();
}

static bool
DoBackspace()
{
  if (cursor < 1)
    return false;

  cursor--;
  edittext[cursor] = 0;
  UpdateTextboxProp();
  return true;
}

static void
OnBackspace()
{
  DoBackspace();
}

static bool
DoCharacter(char character)
{
  if (cursor >= max_width - 1)
    return false;

  edittext[cursor++] = character;
  edittext[cursor] = 0;
  UpdateTextboxProp();
  return true;
}

static bool
FormKeyDown(unsigned key_code)
{
  switch (key_code) {
  case KEY_RIGHT:
    return true;
  case KEY_LEFT:
  case KEY_BACK:
    DoBackspace();
    return true;
  }

  return false;
}

static bool
FormCharacter(unsigned ch)
{
  if (ch < 0x20)
    return false;

  if (ch >= 0x80)
    /* TODO: ASCII only for now, because we don't have proper UTF-8
       support yet */
    return false;

  DoCharacter((char)ch);
  return true;
}

static void
ClearText()
{
  cursor = 0;
  edittext[0] = 0;
  UpdateTextboxProp();
}

bool
TouchTextEntry(char *text, size_t width,
               const char *caption,
               AllowedCharacters accb,
               bool default_shift_state)
{
  if (width == 0)
    width = MAX_TEXTENTRY;

  max_width = std::min(MAX_TEXTENTRY, width);

  const DialogLook &look = UIGlobals::GetDialogLook();
  WndForm form(UIGlobals::GetMainWindow(), look, caption);
  form.SetKeyDownFunction(FormKeyDown);
  form.SetCharacterFunction(FormCharacter);

  ContainerWindow &client_area = form.GetClientAreaWindow();
  const PixelRect rc = client_area.GetClientRect();

  const int client_height = rc.GetHeight();

  const int padding = Layout::Scale(2);
  const int backspace_width = Layout::Scale(36);
  const int backspace_left = rc.right - padding - backspace_width;
  const int editor_height = Layout::Scale(22);
  const int editor_bottom = padding + editor_height;
  const int button_height = Layout::Scale(40);
  constexpr unsigned keyboard_rows = 5;
  const int keyboard_top = editor_bottom + padding;
  const int keyboard_height = keyboard_rows * button_height;
  const int keyboard_bottom = keyboard_top + keyboard_height;

  const bool vertical = client_height >= keyboard_bottom + button_height;

  const int button_top = vertical
    ? rc.bottom - button_height
    : keyboard_bottom - button_height;
  const int button_bottom = vertical
    ? rc.bottom
    : keyboard_bottom;

  const int ok_left = vertical ? 0 : padding;
  const int ok_right = vertical
    ? rc.right / 3
    : ok_left + Layout::Scale(80);

  const int cancel_left = vertical
    ? ok_right
    : Layout::Scale(175);
  const int cancel_right = vertical
    ? rc.right * 2 / 3
    : cancel_left + Layout::Scale(60);

  const int clear_left = vertical
    ? cancel_right
    : Layout::Scale(235);
  const int clear_right = vertical
    ? rc.right
    : clear_left + Layout::Scale(50);

  WndProperty _editor(client_area, look, "",
                      { 0, padding, backspace_left - padding, editor_bottom },
                      0, WindowStyle());
  _editor.SetReadOnly();
  editor = &_editor;

  WindowStyle button_style;
  button_style.TabStop();

  Button ok_button(client_area, look.button, _("OK"),
                   { ok_left, button_top, ok_right, button_bottom },
                   button_style, form.MakeModalResultCallback(mrOK));

  Button cancel_button(client_area, look.button, _("Cancel"),
                       { cancel_left, button_top,
                           cancel_right, button_bottom },
                       button_style, form.MakeModalResultCallback(mrCancel));

  Button clear_button(client_area, look.button, _("Clear"),
                      { clear_left, button_top,
                          clear_right, button_bottom },
                      button_style,
                      [](){ ClearText(); });

  KeyboardWidget keyboard(look.button, FormCharacter, !accb,
                          default_shift_state);

  const PixelRect keyboard_rc = {
    padding, keyboard_top,
    rc.right - padding, keyboard_bottom
  };

  keyboard.Initialise(client_area, keyboard_rc);
  keyboard.Prepare(client_area, keyboard_rc);
  keyboard.Show(keyboard_rc);

  kb = &keyboard;

  Button backspace_button(client_area, look.button, "<-",
                          { backspace_left, padding, rc.right - padding,
                              editor_bottom },
                          button_style, [](){ OnBackspace(); });

  AllowedCharactersCallback = accb;

  cursor = 0;
  ClearText();

  if (!StringIsEmpty(text)) {
    CopyTruncateString(edittext, max_width, text);
    cursor = strlen(edittext);
    if (cursor >= max_width)
      cursor = max_width - 1;
  }

  UpdateTextboxProp();
  bool result = form.ShowModal() == mrOK;

  keyboard.Hide();
  keyboard.Unprepare();

  if (result) {
    CopyTruncateString(text, max_width, edittext);
  }

  return result;
}
