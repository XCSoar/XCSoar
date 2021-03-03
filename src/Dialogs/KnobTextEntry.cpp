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

#include "Dialogs/TextEntry.hpp"
#include "WidgetDialog.hpp"
#include "Language/Language.hpp"
#include "Widget/WindowWidget.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/KeyCode.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "util/CharUtil.hxx"
#include "util/Macros.hpp"
#include "util/StringStrip.hxx"
#include "util/TruncateString.hpp"
#include "util/TStringView.hxx"

#include <algorithm>

#include <string.h>

enum Buttons {
  DOWN,
  UP,
  LEFT,
  RIGHT,
};

static constexpr size_t MAX_TEXTENTRY = 40;

static constexpr TCHAR EntryLetters[] =
  _T(" ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890.-");

static constexpr unsigned MAXENTRYLETTERS = ARRAY_SIZE(EntryLetters) - 1;

/**
 * Find a letter in the list and returns its index.  Returns 0
 * (i.e. the index of the space character) if the given letter is
 * unknown.
 */
gcc_const
static unsigned
FindEntryLetter(TCHAR ch)
{
  for (unsigned i = 0; i < (int)MAXENTRYLETTERS; ++i)
    if (EntryLetters[i] == ch)
      return i;

  return 0;
}

class KnobTextEntryWindow final : public PaintWindow {
  const size_t max_width;

  unsigned int cursor;
  int lettercursor;

  TCHAR buffer[MAX_TEXTENTRY];

public:
  KnobTextEntryWindow(const TCHAR *text, size_t width)
    :max_width(std::min(MAX_TEXTENTRY, width)),
     cursor(0), lettercursor(0) {
    CopyTruncateString(buffer, max_width, text);
    MoveCursor();
  }

  TCHAR *GetValue() {
    return buffer;
  }

private:
  void UpdateCursor() {
    if (lettercursor >= (int)MAXENTRYLETTERS)
      lettercursor = 0;

    if (lettercursor < 0)
      lettercursor = MAXENTRYLETTERS - 1;

    buffer[cursor] = EntryLetters[lettercursor];

    if (IsDefined())
      Invalidate();
  }

  void MoveCursor() {
    if (cursor >= _tcslen(buffer))
      buffer[cursor + 1] = 0;

    lettercursor = FindEntryLetter(ToUpperASCII(buffer[cursor]));

    UpdateCursor();
  }

public:
  bool MoveCursorLeft() {
    if (cursor < 1)
      return false;

    --cursor;
    MoveCursor();
    return true;
  }

  bool MoveCursorRight() {
    if (cursor + 2 >= max_width)
      return false; // max width

    ++cursor;
    MoveCursor();
    return true;
  }

  void IncrementLetter() {
    ++lettercursor;
    UpdateCursor();
  }

  void DecrementLetter() {
    --lettercursor;
    UpdateCursor();
  }

protected:
  /* virtual methods from class Window */
  void OnPaint(Canvas &canvas) override;
};

void
KnobTextEntryWindow::OnPaint(Canvas &canvas)
{
  const PixelRect rc = GetClientRect();

  canvas.Clear(Color(0x40, 0x40, 0x00));

  // Do the actual painting of the text
  const DialogLook &look = UIGlobals::GetDialogLook();
  canvas.Select(look.text_font);

  PixelSize tsize = canvas.CalcTextSize(buffer);
  PixelSize tsizec = canvas.CalcTextSize({buffer, cursor});
  PixelSize tsizea = canvas.CalcTextSize({buffer, cursor + 1});

  BulkPixelPoint p[5];
  p[0].x = 10;
  p[0].y = (rc.GetHeight() - tsize.height - 5) / 2;

  p[2].x = p[0].x + tsizec.width;
  p[2].y = p[0].y + tsize.height + 5;

  p[3].x = p[0].x + tsizea.width;
  p[3].y = p[0].y + tsize.height + 5;

  p[1].x = p[2].x;
  p[1].y = p[2].y - 2;

  p[4].x = p[3].x;
  p[4].y = p[3].y - 2;

  canvas.SelectWhitePen();
  canvas.DrawPolyline(p + 1, 4);

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_WHITE);
  canvas.DrawText(p[0], buffer);
}

class KnobTextEntryWidget final : public WindowWidget {
  const TCHAR *const text;
  const size_t width;

public:
  KnobTextEntryWidget(const TCHAR *_text, size_t _width) noexcept
    :text(_text), width(_width) {}

  auto &GetWindow() noexcept {
    return (KnobTextEntryWindow &)WindowWidget::GetWindow();
  }

  TCHAR *GetValue() {
    return GetWindow().GetValue();
  }

  void CreateButtons(WidgetDialog &dialog);

  /* virtual methods from class Widget */

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();

    auto w = std::make_unique<KnobTextEntryWindow>(text, width);
    w->Create(parent, rc, style);
    SetWindow(std::move(w));
  }
};

inline void
KnobTextEntryWidget::CreateButtons(WidgetDialog &dialog)
{
  dialog.AddButton(_T("A+"), [this](){ GetWindow().IncrementLetter(); });
  dialog.AddButtonKey(KEY_UP);

  dialog.AddButton(_T("A-"), [this](){ GetWindow().DecrementLetter(); });
  dialog.AddButtonKey(KEY_DOWN);

  dialog.AddSymbolButton(_T("<"), [this](){ GetWindow().MoveCursorLeft(); });
  dialog.AddButtonKey(KEY_LEFT);

  dialog.AddSymbolButton(_T(">"), [this](){ GetWindow().MoveCursorRight(); });
  dialog.AddButtonKey(KEY_RIGHT);
}

void
KnobTextEntry(TCHAR *text, size_t width,
              const TCHAR *caption)
{
  if (width == 0)
    width = MAX_TEXTENTRY;

  TWidgetDialog<KnobTextEntryWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), caption);
  dialog.SetWidget(text, width);
  dialog.AddButton(_("Close"), mrOK);
  dialog.GetWidget().CreateButtons(dialog);

  if (dialog.ShowModal() == mrOK) {
    StripRight(dialog.GetWidget().GetValue());
    CopyTruncateString(text, width, dialog.GetWidget().GetValue());
  }
}
