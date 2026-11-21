// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextEntry.hpp"
#include "DialogSettings.hpp"
#include "UIGlobals.hpp"
#include "Asset.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/event/TextInput.hpp"

#include <algorithm>
#include <chrono>

void
DrawTextEntryField(Canvas &canvas, const PixelRect &rc,
                   const char *value, std::size_t cursor,
                   bool cursor_on) noexcept
{
  if (value == nullptr)
    value = "";

  const DialogLook &look = UIGlobals::GetDialogLook();
  canvas.Clear(look.list.background_color);

  const Font &font = look.text_font;
  canvas.Select(font);

  const int before = (int)canvas.CalcTextSize({value, cursor}).width;
  const int underscore = (int)canvas.CalcTextSize("_").width;
  const int padding = (int)Layout::GetTextPadding() * 2;
  const int right = before + underscore;
  const int x = padding - std::max(0, right + 2 * padding -
                                    (int)rc.GetWidth());
  const int y = ((int)rc.GetHeight() - (int)font.GetHeight()) / 2;

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(look.list.text_color);
  canvas.DrawText({x, y}, value);
  /* shortcut: one underscore, not the width of a wider glyph */
  if (cursor_on)
    canvas.DrawText({x + before, y}, "_");

  canvas.SelectHollowBrush();
  canvas.Select(Pen(Layout::ScaleFinePenWidth(1),
                    look.ReadOnlyValueBorderColor()));
  canvas.DrawRectangle(rc);
}

void
TextEntryField::OnCursorTimer() noexcept
{
  cursor_on = !cursor_on;
  Invalidate();
}

void
TextEntryField::SetValue(const char *text, std::size_t offset) noexcept
{
  value = text != nullptr ? text : "";
  cursor = offset;
  cursor_on = true;
  if (cursor_timer.IsActive())
    cursor_timer.Schedule(std::chrono::milliseconds(500));
  if (IsDefined())
    Invalidate();
}

void
TextEntryField::OnCreate()
{
  PaintWindow::OnCreate();
  /* A timer Invalidate() flashes the whole Kobo page. */
  if (!HasEPaper())
    cursor_timer.Schedule(std::chrono::milliseconds(500));
}

void
TextEntryField::OnDestroy() noexcept
{
  cursor_timer.Cancel();
  PaintWindow::OnDestroy();
}

void
TextEntryField::OnPaint(Canvas &canvas) noexcept
{
  DrawTextEntryField(canvas, GetClientRect(), value, cursor, cursor_on);
}

bool
TextEntryDialog(char *text, size_t width,
                const char *caption, AllowedCharacters accb,
                bool default_shift_state)
{
  switch (UIGlobals::GetDialogSettings().text_input_style) {
  case DialogSettings::TextInputStyle::Default:
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
      return KnobTextEntry(text, width, caption, accb);

  case DialogSettings::TextInputStyle::HighScore:
    return KnobTextEntry(text, width, caption, accb);
  }

  return false;
}
