/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "InfoBoxWindow.hpp"
#include "InfoBoxManager.hpp"
#include "InfoBoxSettings.hpp"
#include "Border.hpp"
#include "Look/InfoBoxLook.hpp"
#include "Input/InputEvents.hpp"
#include "Renderer/GlassRenderer.hpp"
#include "Renderer/UnitSymbolRenderer.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Canvas.hpp"
#include "Event/KeyCode.hpp"
#include "Dialogs/dlgInfoBoxAccess.hpp"

#include <algorithm>

InfoBoxWindow::InfoBoxWindow(ContainerWindow &parent, PixelRect rc,
                             unsigned border_flags,
                             const InfoBoxSettings &_settings,
                             const InfoBoxLook &_look,
                             unsigned _id,
                             WindowStyle style)
  :content(NULL),
   settings(_settings), look(_look),
   border_kind(border_flags),
   id(_id),
   dragging(false), pressed(false),
   force_draw_selector(false),
   focus_timer(*this), dialog_timer(*this)
{
  data.Clear();

  Create(parent, rc, style);
}

InfoBoxWindow::~InfoBoxWindow() {
  delete content;
  Destroy();
}

void
InfoBoxWindow::SetTitle(const TCHAR *_title)
{
  data.SetTitle(_title);
  Invalidate(title_rect);
}

void
InfoBoxWindow::PaintTitle(Canvas &canvas)
{
  if (data.title.empty())
    return;

  if (!pressed && !HasFocus() && !dragging && !force_draw_selector &&
      settings.border_style == InfoBoxSettings::BorderStyle::SHADED)
    canvas.DrawFilledRectangle(title_rect, look.caption_background_color);

  canvas.SetTextColor(look.GetTitleColor(data.title_color));

  const Font &font = look.title_font;
  canvas.Select(font);

  PixelSize tsize = canvas.CalcTextSize(data.title);

  int halftextwidth = (title_rect.left + title_rect.right - tsize.cx) / 2;
  int x = std::max(1, title_rect.left + halftextwidth);
  int y = title_rect.top;

  canvas.TextAutoClipped(x, y, data.title);

  if (settings.border_style == InfoBoxSettings::BorderStyle::TAB &&
      halftextwidth > Layout::Scale(3)) {

    const auto pad = Layout::Scale(2);
    const auto text_pad = Layout::GetTextPadding()*2;

    int ytop = title_rect.top + font.GetCapitalHeight() / 2;
    int ytopedge = ytop + pad;
    int ybottom = title_rect.top + Layout::Scale(6)
      + font.GetCapitalHeight();

    canvas.Select(look.border_pen);

    BulkPixelPoint tab[8];
    tab[0].x = tab[1].x = title_rect.left;
    tab[0].y = tab[7].y = ybottom;
    tab[2].x = title_rect.left + pad;
    tab[2].y = tab[5].y = tab[3].y = tab[4].y = ytop;
    tab[1].y = tab[6].y = ytopedge;
    tab[5].x = title_rect.right - pad;
    tab[6].x = tab[7].x = title_rect.right;
    tab[3].x = title_rect.left + halftextwidth - text_pad;
    tab[4].x = title_rect.right - halftextwidth + text_pad;

    canvas.DrawPolyline(tab, 4);
    canvas.DrawPolyline(tab + 4, 4);
  }
}

void
InfoBoxWindow::PaintValue(Canvas &canvas, Color background_color)
{
  if (data.value.empty())
    return;

  canvas.SetTextColor(look.GetValueColor(data.value_color));

  canvas.Select(look.unit_font);
  int unit_width =
    UnitSymbolRenderer::GetSize(canvas, data.value_unit).cx;

  canvas.Select(look.value_font);
  int ascent_height = look.value_font.GetAscentHeight();

  PixelSize value_size = canvas.CalcTextSize(data.value);
  if (unsigned(value_size.cx + unit_width) > value_rect.GetWidth()) {
    canvas.Select(look.small_value_font);
    ascent_height = look.small_value_font.GetAscentHeight();
    value_size = canvas.CalcTextSize(data.value);
  }

  int x = std::max(0,
                   (value_rect.left + value_rect.right
                    - value_size.cx - unit_width) / 2);

  int y = (value_rect.top + value_rect.bottom - value_size.cy) / 2;

  canvas.TextAutoClipped(x, y, data.value);

  if (unit_width != 0) {
    const int unit_height =
      UnitSymbolRenderer::GetAscentHeight(look.unit_font, data.value_unit);

    canvas.Select(look.unit_font);
    UnitSymbolRenderer::Draw(canvas,
                             { x + value_size.cx,
                                 y + ascent_height - unit_height },
                             data.value_unit, look.unit_fraction_pen);
  }
}

void
InfoBoxWindow::PaintComment(Canvas &canvas)
{
  if (data.comment.empty())
    return;

  canvas.SetTextColor(look.GetCommentColor(data.comment_color));

  const Font &font = look.title_font;
  canvas.Select(font);

  PixelSize tsize = canvas.CalcTextSize(data.comment);

  int x = std::max(1,
                   (comment_rect.left + comment_rect.right - tsize.cx) / 2);
  int y = comment_rect.top;

  canvas.TextAutoClipped(x, y, data.comment);
}

void
InfoBoxWindow::Paint(Canvas &canvas)
{
  const Color background_color = pressed
    ? look.pressed_background_color
    : (HasFocus() || dragging || force_draw_selector
       ? look.focused_background_color
       : look.background_color);
  if (settings.border_style == InfoBoxSettings::BorderStyle::GLASS)
    DrawGlassBackground(canvas, canvas.GetRect(), background_color);
  else
    canvas.Clear(background_color);

  if (data.GetCustom() && content != NULL) {
    /* if there's no comment, the content object may paint that area,
       too */
    const PixelRect &rc = data.comment.empty()
      ? value_and_comment_rect
      : value_rect;
    content->OnCustomPaint(canvas, rc);
  }

  canvas.SetBackgroundTransparent();

  PaintTitle(canvas);
  PaintComment(canvas);
  PaintValue(canvas, background_color);

  if (border_kind != 0) {
    canvas.Select(look.border_pen);

    const unsigned width = canvas.GetWidth(),
      height = canvas.GetHeight();

    if (border_kind & BORDERTOP) {
      canvas.DrawExactLine(0, 0, width - 1, 0);
    }

    if (border_kind & BORDERRIGHT) {
      canvas.DrawExactLine(width - 1, 0, width - 1, height);
    }

    if (border_kind & BORDERBOTTOM) {
      canvas.DrawExactLine(0, height - 1, width - 1, height - 1);
    }

    if (border_kind & BORDERLEFT) {
      canvas.DrawExactLine(0, 0, 0, height - 1);
    }
  }
}

void
InfoBoxWindow::SetContentProvider(InfoBoxContent *_content)
{
  delete content;
  content = _content;

  data.SetInvalid();
  Invalidate();
}

void
InfoBoxWindow::UpdateContent()
{
  if (content == NULL)
    return;

  InfoBoxData old = data;
  content->Update(data);

  if (old.GetCustom() || data.GetCustom())
    /* must Invalidate everything when custom painting is/was
       enabled */
    Invalidate();
  else {
#ifdef ENABLE_OPENGL
    if (!data.CompareTitle(old) || !data.CompareValue(old) ||
        !data.CompareComment(old))
      Invalidate();
#else
    if (!data.CompareTitle(old))
      Invalidate(title_rect);
    if (!data.CompareValue(old))
      Invalidate(value_rect);
    if (!data.CompareComment(old))
      Invalidate(comment_rect);
#endif
  }
}

void
InfoBoxWindow::ShowDialog()
{
  force_draw_selector = true;

  dlgInfoBoxAccessShowModeless(id, GetDialogContent());

  force_draw_selector = false;
}

bool
InfoBoxWindow::HandleKey(InfoBoxContent::InfoBoxKeyCodes keycode)
{
  if (content != NULL && content->HandleKey(keycode)) {
    UpdateContent();
    return true;
  }
  return false;
}

const InfoBoxPanel *
InfoBoxWindow::GetDialogContent() const
{
  if (content != NULL)
    return content->GetDialogContent();

  return NULL;
}

void
InfoBoxWindow::OnDestroy()
{
  focus_timer.Cancel();
  dialog_timer.Cancel();
  PaintWindow::OnDestroy();
}

void
InfoBoxWindow::OnResize(PixelSize new_size)
{
  PaintWindow::OnResize(new_size);

  PixelRect rc = GetClientRect();

  if (border_kind & BORDERLEFT)
    rc.left += look.BORDER_WIDTH;

  if (border_kind & BORDERRIGHT)
    rc.right -= look.BORDER_WIDTH;

  if (border_kind & BORDERTOP)
    rc.top += look.BORDER_WIDTH;

  if (border_kind & BORDERBOTTOM)
    rc.bottom -= look.BORDER_WIDTH;

  title_rect = rc;
  title_rect.bottom = rc.top + look.title_font.GetHeight();

  comment_rect = rc;
  comment_rect.bottom -= Layout::Scale(2);
  comment_rect.top = comment_rect.bottom - (look.title_font.GetHeight() + Layout::Scale(2));

  value_rect = rc;
  value_rect.top = title_rect.bottom;
  value_rect.bottom = comment_rect.top;

  value_and_comment_rect = value_rect;
  value_and_comment_rect.bottom = comment_rect.bottom;
}

bool
InfoBoxWindow::OnKeyDown(unsigned key_code)
{
  /* handle local hot key */
  if (HasFocus()) {
    /* Let the InfoBoxManager decide what is to do */
    InfoBoxManager::OnKeyDown(this, key_code);
  }

  switch (key_code) {
  case KEY_UP:
    focus_timer.Schedule(FOCUS_TIMEOUT_MAX);
    return HandleKey(InfoBoxContent::ibkUp);

  case KEY_DOWN:
    focus_timer.Schedule(FOCUS_TIMEOUT_MAX);
    return HandleKey(InfoBoxContent::ibkDown);

  case KEY_LEFT:
    focus_timer.Schedule(FOCUS_TIMEOUT_MAX);
    return HandleKey(InfoBoxContent::ibkLeft);

  case KEY_RIGHT:
    focus_timer.Schedule(FOCUS_TIMEOUT_MAX);
    return HandleKey(InfoBoxContent::ibkRight);

  case KEY_RETURN:
    ShowDialog();
    return true;

  case KEY_ESCAPE:
    focus_timer.Cancel();
    FocusParent();
    return true;
  }

  /* handle global hot key */

  if (InputEvents::ProcessKey(InputEvents::MODE_INFOBOX, key_code))
    return true;

  /* call super class */

  return PaintWindow::OnKeyDown(key_code);
}

bool
InfoBoxWindow::OnMouseDown(PixelPoint p)
{
  dialog_timer.Cancel();

  if (!dragging) {
    dragging = true;
    SetCapture();

    pressed = true;
    Invalidate();

    /* start "long click" detection */
    dialog_timer.Schedule(1000);
  }

  return true;
}

bool
InfoBoxWindow::OnMouseUp(PixelPoint p)
{
  dialog_timer.Cancel();

  if (dragging) {
    const bool was_pressed = pressed;

    dragging = false;
    pressed = false;
    Invalidate();

    ReleaseCapture();

    if (was_pressed) {
      SetFocus();

      if (GetDialogContent() != nullptr)
        /* delay the dialog, so double click detection works */
        dialog_timer.Schedule(300);
    }

    return true;
  }

  return false;
}

bool
InfoBoxWindow::OnMouseDouble(PixelPoint p)
{
  dialog_timer.Cancel();
  InputEvents::ShowMenu();
  return true;
}

bool
InfoBoxWindow::OnMouseMove(PixelPoint p, unsigned keys)
{
  SetFocus();

  if (dragging) {
    SetPressed(IsInside(p));
    if (!pressed)
      dialog_timer.Cancel();
    return true;
  }

  return false;
}

void
InfoBoxWindow::OnPaintBuffer(Canvas &canvas)
{
  Paint(canvas);
}

void
InfoBoxWindow::OnCancelMode()
{
  if (dragging) {
    dragging = false;
    pressed = false;
    Invalidate();
    ReleaseCapture();
  }

  dialog_timer.Cancel();

  PaintWindow::OnCancelMode();
}

void
InfoBoxWindow::OnSetFocus()
{
  // Call the parent function
  PaintWindow::OnSetFocus();

  // Start the focus-auto-return timer
  // to automatically return focus back to MapWindow if idle
  focus_timer.Schedule(HasCursorKeys() ? FOCUS_TIMEOUT_MAX : 1100);

  // Redraw fast to paint the selector
  Invalidate();
}

void
InfoBoxWindow::OnKillFocus()
{
  // Call the parent function
  PaintWindow::OnKillFocus();

  // Destroy the time if it exists
  focus_timer.Cancel();

  // Redraw fast to remove the selector
  Invalidate();
}

bool
InfoBoxWindow::OnTimer(WindowTimer &timer)
{
  if (timer == focus_timer) {
    focus_timer.Cancel();
    FocusParent();
    return true;
  } else if (timer == dialog_timer) {
    dragging = pressed = false;
    Invalidate();
    ReleaseCapture();

    dialog_timer.Cancel();
    ShowDialog();
    return true;
  } else
    return PaintWindow::OnTimer(timer);
}
