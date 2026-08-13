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
#include "ui/event/TextInput.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "util/CharUtil.hxx"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/TruncateString.hpp"
#include "ui/window/Window.hpp"

#include <algorithm>

namespace {
struct TextEntryLayout {
  PixelRect editor;
  PixelRect paste;
  PixelRect backspace;
  PixelRect keyboard;
  PixelRect ok, cancel, clear;
};

/**
 * @param with_keyboard show XCSoar's own on-screen keyboard?  If not,
 * the operating system's keyboard covers the lower part of the screen,
 * and everything is moved up below the editor.
 * @param with_paste reserve room for the @em Paste button
 */
static void
ComputeTextEntryLayout(const PixelRect &rc, bool with_keyboard, bool with_paste,
                       TextEntryLayout &o) noexcept
{
  const int client_height = rc.GetHeight();
  const int padding = Layout::Scale(2);
  const int backspace_width = Layout::Scale(36);
  const int backspace_left = rc.right - padding - backspace_width;
  const int paste_width = with_paste ? Layout::Scale(60) : 0;
  const int paste_left = backspace_left - (with_paste ? padding : 0) -
    paste_width;
  const int editor_height = Layout::Scale(22);
  const int editor_bottom = padding + editor_height;
  const int button_height = Layout::Scale(40);
  constexpr unsigned keyboard_rows = 5u;
  const int keyboard_top = editor_bottom + padding;
  const int keyboard_height = with_keyboard
    ? int(keyboard_rows) * button_height
    : 0;
  const int keyboard_bottom = keyboard_top + keyboard_height;

  /* without our own keyboard, the action row stays right below the
     editor, where the system keyboard cannot cover it */
  const bool vertical = with_keyboard &&
    client_height >= keyboard_bottom + button_height;

  const int button_top = !with_keyboard
    ? keyboard_top
    : (vertical
       ? rc.bottom - button_height
       : keyboard_bottom - button_height);
  const int button_bottom = button_top + button_height;

  /* the action row spans the whole width unless it has to share its
     row with the last keyboard row */
  const bool spread = vertical || !with_keyboard;

  const int ok_left = spread ? 0 : padding;
  const int ok_right = spread
    ? rc.right / 3
    : ok_left + Layout::Scale(80);

  const int cancel_left = spread
    ? ok_right
    : Layout::Scale(175);
  const int cancel_right = spread
    ? rc.right * 2 / 3
    : cancel_left + Layout::Scale(60);

  const int clear_left = spread
    ? cancel_right
    : Layout::Scale(235);
  const int clear_right = spread
    ? rc.right
    : clear_left + Layout::Scale(50);

  o.editor = {0, padding, paste_left - padding, editor_bottom};
  o.paste = {paste_left, padding, paste_left + paste_width, editor_bottom};
  o.backspace = {backspace_left, padding, rc.right - padding, editor_bottom};
  o.keyboard = {padding, keyboard_top, rc.right - padding, keyboard_bottom};
  o.ok = {ok_left, button_top, ok_right, button_bottom};
  o.cancel = {cancel_left, button_top, cancel_right, button_bottom};
  o.clear = {clear_left, button_top, clear_right, button_bottom};
}

/**
 * The caption of the backspace key: the "erase to the left" symbol
 * where the font has it, else an arrow made of ASCII.
 */
[[gnu::pure]]
static const char *
BackspaceCaption(const ButtonLook &look) noexcept
{
  return look.font != nullptr && look.font->HasGlyph(0x232B)
    ? "⌫"
    : "<-";
}

static void
ApplyTextEntryLayout(const TextEntryLayout &L, WndProperty &editor, Button &ok,
                     Button &cancel, Button &clear, KeyboardWidget *keyboard,
                     Button &backspace, Button *paste,
                     ContainerWindow &client_area) noexcept
{
  editor.Move(L.editor);
  ok.Move(L.ok);
  cancel.Move(L.cancel);
  clear.Move(L.clear);
  if (keyboard != nullptr)
    keyboard->Move(L.keyboard);
  backspace.Move(L.backspace);
  if (paste != nullptr)
    paste->Move(L.paste);
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
static Button *textentry_paste = NULL;

static AllowedCharacters AllowedCharactersCallback;

static constexpr size_t MAX_TEXTENTRY = 256;
static unsigned int cursor = 0;
static size_t max_width;
static char edittext[MAX_TEXTENTRY];

static void
UpdateAllowedCharacters()
{
  if (kb != nullptr && AllowedCharactersCallback)
    kb->SetAllowedCharacters(AllowedCharactersCallback(edittext));
}

/**
 * Check a character against the AllowedCharacters callback.  Without
 * our own on-screen keyboard, nothing else enforces it.
 *
 * @return the character to be inserted (may have been converted to
 * upper case), or 0 if it is not allowed
 */
static char
FilterCharacter(char ch)
{
  if (!AllowedCharactersCallback)
    return ch;

  const char *allowed = AllowedCharactersCallback(edittext);
  if (allowed == nullptr || StringFind(allowed, ch) != nullptr)
    return ch;

  /* the allowed set is usually upper case only */
  if (const char upper = ToUpperASCII(ch);
      upper != ch && StringFind(allowed, upper) != nullptr)
    return upper;

  return 0;
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

/**
 * Append the system clipboard contents to the edit field.
 */
static void
OnPaste()
{
  for (const char ch : UI::TextInput::GetClipboardText()) {
    if (!IsPrintableASCII(ch))
      /* TODO: ASCII only for now, because we don't have proper UTF-8
         support yet */
      continue;

    const char filtered = FilterCharacter(ch);
    if (filtered == 0)
      continue;

    if (!DoCharacter(filtered))
      /* the edit field is full */
      break;
  }
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

  /* the @em Paste button sits left of the on-screen backspace */
  if (HasCursorKeys() && textentry_client != nullptr &&
      textentry_paste != nullptr && textentry_backspace != nullptr) {
    Window *const w = textentry_client->GetFocusedWindow();
    if (key_code == KEY_LEFT && w == static_cast<Window *>(textentry_backspace)) {
      textentry_paste->SetFocus();
      return true;
    }
    if (key_code == KEY_RIGHT && w == static_cast<Window *>(textentry_paste)) {
      textentry_backspace->SetFocus();
      return true;
    }
  }

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

  if (key_code == KEY_BACK || key_code == KEY_F1) {
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

  if (kb == nullptr) {
    /* without our own on-screen keyboard (which disables the buttons
       for characters that are not allowed), the filter must be
       applied here */
    const char filtered = FilterCharacter((char)ch);
    if (filtered == 0)
      return true;

    DoCharacter(filtered);
    return true;
  }

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
               bool default_shift_state,
               bool use_system_keyboard)
{
  if (width == 0)
    width = MAX_TEXTENTRY;

  max_width = std::min(MAX_TEXTENTRY, width);

  /* let the operating system provide the keyboard (giving access to
     all special characters); if it has none, fall back to our own */
  const bool system_keyboard = use_system_keyboard &&
    UI::TextInput::HasScreenKeyboard();
  const bool with_paste = UI::TextInput::HasClipboard();

  const DialogLook &look = UIGlobals::GetDialogLook();
  WndForm form(UIGlobals::GetMainWindow(), look, caption);
  form.SetKeyDownFunction(FormKeyDown);
  form.SetCharacterFunction(FormCharacter);

  ContainerWindow &client_area = form.GetClientAreaWindow();
  textentry_client = &client_area;
  const PixelRect rc0 = client_area.GetClientRect();
  TextEntryLayout L;
  ComputeTextEntryLayout(rc0, !system_keyboard, with_paste, L);

  WndProperty _editor(client_area, look, "",
                      L.editor,
                      0, WindowStyle());
  _editor.SetReadOnly();
  editor = &_editor;
  _editor.SetAlignment(WndProperty::Alignment::AUTO);

  WindowStyle button_style;
  button_style.TabStop();

  /* Create OK first so FocusFirstControl lands there: Enter confirms
     instead of typing the focused soft key (often @em 1).  Do not use
     HasKeyboard() for this — joystick remotes often appear as HID
     keyboards.  Up from OK still enters the grid (Space). */
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

  kb = nullptr;
  if (!system_keyboard) {
    keyboard.Initialise(client_area, L.keyboard);
    keyboard.Prepare(client_area, L.keyboard);
    keyboard.Show(L.keyboard);

    kb = &keyboard;
  }

  Button backspace_button(client_area, look.button,
                          BackspaceCaption(look.button),
                          L.backspace,
                          button_style, [](){ OnBackspace(); });

  textentry_backspace = &backspace_button;

  Button paste_button;
  if (with_paste) {
    paste_button.Create(client_area, look.button, _("Paste"), L.paste,
                        button_style, [](){ OnPaste(); });
    textentry_paste = &paste_button;
  }

  form.SetClientLayoutFunction([&]() {
    const PixelRect rc = client_area.GetClientRect();
    TextEntryLayout layout;
    ComputeTextEntryLayout(rc, !system_keyboard, with_paste, layout);
    ApplyTextEntryLayout(layout, _editor, ok_button, cancel_button, clear_button,
                         kb, backspace_button,
                         with_paste ? &paste_button : nullptr, client_area);
    if (system_keyboard)
      UI::TextInput::SetScreenKeyboardRect(layout.editor);
  });

  AllowedCharactersCallback = accb;

  cursor = 0;
  ClearText();

  if (!StringIsEmpty(text)) {
    CopyTruncateString(edittext, max_width, text);
    cursor = strlen(edittext);
  }

  UpdateTextboxProp();

  if (system_keyboard) {
    UI::TextInput::SetScreenKeyboardRect(L.editor);
    UI::TextInput::ShowScreenKeyboard();
  }

  const bool result = form.ShowModal() == mrOK;

  if (system_keyboard)
    UI::TextInput::HideScreenKeyboard();

  textentry_backspace = NULL;
  textentry_ok = NULL;
  textentry_cancel = NULL;
  textentry_clear = NULL;
  textentry_paste = NULL;
  textentry_client = NULL;

  if (kb != nullptr) {
    keyboard.Hide();
    keyboard.Unprepare();
    kb = nullptr;
  }

  if (result) {
    CopyTruncateString(text, max_width, edittext);
  }

  return result;
}
