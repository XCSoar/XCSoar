// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/TextEntry.hpp"
#include "WidgetDialog.hpp"
#include "Form/TextEntryBuffer.hpp"
#include "Asset.hpp"
#include "ui/event/TextInput.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Widget/WindowWidget.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/KeyCode.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "util/Macros.hpp"
#include "util/TruncateString.hpp"
#include "util/UTF8.hpp"

#include <algorithm>
#include <utility>

#include <string.h>

enum Buttons {
  DOWN,
  UP,
  LEFT,
  RIGHT,
};

static constexpr size_t MAX_TEXTENTRY = TextEntryBuffer::CAPACITY;

static constexpr char EntryLetters[] =
  " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
  "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

static constexpr unsigned MAXENTRYLETTERS = ARRAY_SIZE(EntryLetters) - 1;

/**
 * Find a letter in the list and return its index. Return the space
 * character index (zero) if the given letter is unknown.
 */
[[gnu::const]]
static unsigned
FindEntryLetter(char ch)
{
  for (unsigned i = 0; i < (int)MAXENTRYLETTERS; ++i)
    if (EntryLetters[i] == ch)
      return i;

  return 0;
}

class KnobTextEntryWindow final : public PaintWindow {
  TextEntryBuffer buffer;
  AllowedCharacters allowed_characters;
  int lettercursor = 0;
  bool continue_after_cycle = false;

public:
  KnobTextEntryWindow(const char *text, size_t width, AllowedCharacters allowed)
    :buffer(text, width), allowed_characters(std::move(allowed)) {
    buffer.Home();
    MoveCursor();
  }

  const char *GetValue() const noexcept { return buffer.GetValue(); }

private:
  const char *GetAllowedCharacters() const {
    if (!allowed_characters)
      return nullptr;
    char prefix[MAX_TEXTENTRY];
    CopyTruncateString(prefix, sizeof(prefix), buffer.GetValue());
    /* The cursor always lands on a complete UTF-8 character boundary. */
    prefix[buffer.GetCursor()] = 0;
    return allowed_characters(prefix);
  }

  void MoveCursor() {
    continue_after_cycle = false;
    lettercursor = buffer.GetSequenceLength() == 1
      ? FindEntryLetter(buffer.GetValue()[buffer.GetCursor()])
      : 0;
    if (IsDefined())
      Invalidate();
  }

  void CycleLetter(int direction) {
    const char *allowed = GetAllowedCharacters();
    for (unsigned i = 0; i < MAXENTRYLETTERS; ++i) {
      lettercursor = (lettercursor + direction + MAXENTRYLETTERS) %
        MAXENTRYLETTERS;
      const char ch = TextEntryBuffer::Filter(EntryLetters[lettercursor],
                                             allowed);
      if (ch != 0) {
        const bool replaced = buffer.Replace(ch);
        MoveCursor();
        continue_after_cycle = replaced;
        return;
      }
    }
  }

public:
  bool MoveCursorLeft() {
    const bool moved = buffer.MoveLeft();
    MoveCursor();
    return moved;
  }

  bool MoveCursorRight() {
    const bool moved = buffer.MoveRight();
    MoveCursor();
    return moved;
  }

  void IncrementLetter() { CycleLetter(1); }
  void DecrementLetter() { CycleLetter(-1); }

  bool Character(unsigned ch) {
    if (ch < 0x20 || ch == 0x7f)
      return false;
    const bool moved = continue_after_cycle && buffer.MoveRight();
    const char filtered = TextEntryBuffer::Filter(ch, GetAllowedCharacters());
    if (filtered == 0 || !buffer.Insert(filtered)) {
      if (moved)
        buffer.MoveLeft();
      return true;
    }
    MoveCursor();
    return true;
  }

  bool KeyPress(unsigned key_code) {
    switch (key_code) {
    case KEY_BACK:
    case KEY_F1:
      buffer.Backspace();
      break;
    case KEY_DELETE:
      buffer.Delete();
      break;
    case KEY_HOME:
      buffer.Home();
      break;
    case KEY_END:
      buffer.End();
      break;
    case KEY_SPACE:
      return true; // text is delivered separately by some backends
    default:
      return false;
    }
    MoveCursor();
    return true;
  }

protected:
  void OnPaint(Canvas &canvas) noexcept override;
};

void
KnobTextEntryWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();
  const char *value = buffer.GetValue();
  const std::string_view text{value};
  const auto cursor = buffer.GetCursor();

  canvas.Clear(COLOR_BLACK);

  // Do the actual painting of the text
  const DialogLook &look = UIGlobals::GetDialogLook();
  canvas.Select(look.text_font);

  PixelSize tsize = canvas.CalcTextSize(text);
  PixelSize tsizec = canvas.CalcTextSize({value, cursor});
  PixelSize tsizea = canvas.CalcTextSize(
    {value, cursor + buffer.GetSequenceLength()});

  BulkPixelPoint p[5];
  const int padding = Layout::Scale(10);
  const int cursor_right = tsizea.width + padding;
  p[0].x = padding - std::max(0, cursor_right - int(rc.GetWidth()) + padding);
  p[0].y = (rc.GetHeight() - tsize.height - 5) / 2;

  p[2].x = p[0].x + tsizec.width;
  p[2].y = p[0].y + tsize.height + 5;

  p[3].x = p[0].x + (buffer.GetSequenceLength() > 0
    ? tsizea.width : tsizec.width + Layout::Scale(10));
  p[3].y = p[0].y + tsize.height + 5;

  p[1].x = p[2].x;
  p[1].y = p[2].y - 2;

  p[4].x = p[3].x;
  p[4].y = p[3].y - 2;

  canvas.SelectWhitePen();
  canvas.DrawPolyline(p + 1, 4);

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_WHITE);
  canvas.DrawText(p[0], text);
}

class KnobTextEntryWidget final : public WindowWidget {
  const char *const text;
  const size_t width;
  AllowedCharacters allowed;

public:
  KnobTextEntryWidget(const char *_text, size_t _width,
                      AllowedCharacters _allowed) noexcept
    :text(_text), width(_width), allowed(std::move(_allowed)) {}

  auto &GetWindow() noexcept {
    return (KnobTextEntryWindow &)WindowWidget::GetWindow();
  }

  const char *GetValue() {
    return GetWindow().GetValue();
  }

  void CreateButtons(WidgetDialog &dialog);

  bool KeyPress(unsigned key_code) noexcept override {
    return GetWindow().KeyPress(key_code);
  }

  /* virtual methods from class Widget */

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();

    auto w = std::make_unique<KnobTextEntryWindow>(text, width, allowed);
    w->Create(parent, rc, style);
    SetWindow(std::move(w));
  }
};

inline void
KnobTextEntryWidget::CreateButtons(WidgetDialog &dialog)
{
  dialog.AddButton("A+", [this](){ GetWindow().IncrementLetter(); });
  dialog.AddButtonKey(KEY_UP);

  dialog.AddButton("A-", [this](){ GetWindow().DecrementLetter(); });
  dialog.AddButtonKey(KEY_DOWN);

  dialog.AddSymbolButton("<", [this](){ GetWindow().MoveCursorLeft(); });
  dialog.AddButtonKey(KEY_LEFT);

  dialog.AddSymbolButton(">", [this](){ GetWindow().MoveCursorRight(); });
  dialog.AddButtonKey(KEY_RIGHT);
}

bool
KnobTextEntry(char *text, size_t width,
              const char *caption, AllowedCharacters allowed)
{
  if (width == 0)
    width = MAX_TEXTENTRY;

  TWidgetDialog<KnobTextEntryWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), caption);
  dialog.SetWidget(text, width, std::move(allowed));
  dialog.SetCharacterFunction([&](unsigned ch) {
    return dialog.GetWidget().GetWindow().Character(ch);
  });
  dialog.AddButton(_("Close"), mrOK);
  dialog.GetWidget().CreateButtons(dialog);

  /* iOS can stop text events when the preceding editor hides its system
     keyboard. Enable them again for an attached hardware keyboard. */
  const bool system_text_input = UI::TextInput::HasScreenKeyboard() &&
    HasKeyboard();
  if (system_text_input)
    UI::TextInput::ShowScreenKeyboard();
  const bool accepted = dialog.ShowModal() == mrOK;
  if (system_text_input)
    UI::TextInput::HideScreenKeyboard();

  if (accepted) {
    CopyTruncateString(text, width, dialog.GetWidget().GetValue());
    return true;
  }

  return false;
}
