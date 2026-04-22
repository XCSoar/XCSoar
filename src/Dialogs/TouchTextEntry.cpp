// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/TextEntry.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Widget/KeyboardWidget.hpp"
#include "Asset.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "util/StringCompare.hxx"
#include "util/TruncateString.hpp"
#include "ui/window/Window.hpp"

#include <algorithm>

namespace {
struct TextEntryLayout {
  PixelRect editor;
  PixelRect backspace;
  PixelRect keyboard;
  PixelRect ok, cancel, clear;
};

static void
ComputeTextEntryLayout(const PixelRect &rc, TextEntryLayout &o) noexcept
{
  const int client_height = rc.GetHeight();
  const int padding = Layout::Scale(2);
  const int backspace_width = Layout::Scale(36);
  const int backspace_left = rc.right - padding - backspace_width;
  const int editor_height = Layout::Scale(22);
  const int editor_bottom = padding + editor_height;
  const int button_height = Layout::Scale(40);
  constexpr unsigned keyboard_rows = 5u;
  const int keyboard_top = editor_bottom + padding;
  const int keyboard_height = int(keyboard_rows) * button_height;
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

  o.editor = {0, padding, backspace_left - padding, editor_bottom};
  o.backspace = {backspace_left, padding, rc.right - padding, editor_bottom};
  o.keyboard = {padding, keyboard_top, rc.right - padding, keyboard_bottom};
  o.ok = {ok_left, button_top, ok_right, button_bottom};
  o.cancel = {cancel_left, button_top, cancel_right, button_bottom};
  o.clear = {clear_left, button_top, clear_right, button_bottom};
}

static void
ApplyTextEntryLayout(const TextEntryLayout &L, WndProperty &editor, Button &ok,
                     Button &cancel, Button &clear, KeyboardWidget &keyboard,
                     Button &backspace, ContainerWindow &client_area) noexcept
{
  editor.Move(L.editor);
  ok.Move(L.ok);
  cancel.Move(L.cancel);
  clear.Move(L.clear);
  keyboard.Move(L.keyboard);
  backspace.Move(L.backspace);
  client_area.Invalidate();
}
} // namespace

static WndProperty *editor;
static KeyboardWidget *kb = NULL;
static ContainerWindow *textentry_client = NULL;
static Button *textentry_backspace = NULL;
static Button *textentry_ok = NULL;
static Button *textentry_cancel = NULL;
static Button *textentry_clear = NULL;

static AllowedCharacters AllowedCharactersCallback;

static constexpr size_t MAX_TEXTENTRY = 256;
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
  /* On devices with cursor keys, first let the on-screen keyboard
     move focus between key buttons; use Backspace for delete.  On
     others (e.g. Kobo), Left and Back both act as backspace. */
  if (HasCursorKeys() && kb != nullptr &&
      kb->KeyPress(key_code, textentry_backspace, textentry_ok))
    return true;

  if (HasCursorKeys() && textentry_client != nullptr && textentry_ok != nullptr &&
      textentry_cancel != nullptr && textentry_clear != nullptr) {
    Window *const w = textentry_client->GetFocusedWindow();
    if (key_code == KEY_RIGHT) {
      if (w == static_cast<Window *>(textentry_ok)) {
        textentry_cancel->SetFocus();
        return true;
      }
      if (w == static_cast<Window *>(textentry_cancel)) {
        textentry_clear->SetFocus();
        return true;
      }
    } else if (key_code == KEY_LEFT) {
      if (w == static_cast<Window *>(textentry_clear)) {
        textentry_cancel->SetFocus();
        return true;
      }
      if (w == static_cast<Window *>(textentry_cancel)) {
        textentry_ok->SetFocus();
        return true;
      }
    }
  }

  /* @c KEY_DOWN on @em Clear used to run tab order (e.g. first to @em 1);
     go to the on-screen back key instead. */
  if (HasCursorKeys() && key_code == KEY_DOWN &&
      textentry_client != nullptr && textentry_clear != nullptr &&
      textentry_backspace != nullptr) {
    Window *const w = textentry_client->GetFocusedWindow();
    if (w == static_cast<Window *>(textentry_clear)) {
      textentry_backspace->SetFocus();
      return true;
    }
  }

  if (key_code == KEY_BACK) {
    DoBackspace();
    return true;
  }

  if (!HasCursorKeys() && key_code == KEY_LEFT) {
    DoBackspace();
    return true;
  }

  if (!HasCursorKeys() && key_code == KEY_RIGHT)
    return true;

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
  textentry_client = &client_area;
  const PixelRect rc0 = client_area.GetClientRect();
  TextEntryLayout L;
  ComputeTextEntryLayout(rc0, L);

  WndProperty _editor(client_area, look, "",
                      L.editor,
                      0, WindowStyle());
  _editor.SetReadOnly();
  editor = &_editor;
  _editor.SetAlignment(WndProperty::Alignment::AUTO);

  WindowStyle button_style;
  button_style.TabStop();

  Button ok_button(client_area, look.button, _("OK"),
                   L.ok,
                   button_style, form.MakeModalResultCallback(mrOK));
  textentry_ok = &ok_button;

  Button cancel_button(client_area, look.button, _("Cancel"),
                       L.cancel,
                       button_style, form.MakeModalResultCallback(mrCancel));
  textentry_cancel = &cancel_button;

  Button clear_button(client_area, look.button, _("Clear"),
                      L.clear,
                      button_style,
                      [](){ ClearText(); });
  textentry_clear = &clear_button;

  KeyboardWidget keyboard(look.button, FormCharacter, !accb,
                          default_shift_state);

  keyboard.Initialise(client_area, L.keyboard);
  keyboard.Prepare(client_area, L.keyboard);
  keyboard.Show(L.keyboard);

  kb = &keyboard;

  Button backspace_button(client_area, look.button, "<-",
                          L.backspace,
                          button_style, [](){ OnBackspace(); });

  textentry_backspace = &backspace_button;

  form.SetClientLayoutFunction([&]() {
    const PixelRect rc = client_area.GetClientRect();
    TextEntryLayout layout;
    ComputeTextEntryLayout(rc, layout);
    ApplyTextEntryLayout(layout, _editor, ok_button, cancel_button, clear_button,
                        keyboard, backspace_button, client_area);
  });

  AllowedCharactersCallback = accb;

  cursor = 0;
  ClearText();

  if (!StringIsEmpty(text)) {
    CopyTruncateString(edittext, max_width, text);
    cursor = strlen(edittext);
  }

  UpdateTextboxProp();
  const bool result = form.ShowModal() == mrOK;

  textentry_backspace = NULL;
  textentry_ok = NULL;
  textentry_cancel = NULL;
  textentry_clear = NULL;
  textentry_client = NULL;

  keyboard.Hide();
  keyboard.Unprepare();

  if (result) {
    CopyTruncateString(text, max_width, edittext);
  }

  return result;
}
