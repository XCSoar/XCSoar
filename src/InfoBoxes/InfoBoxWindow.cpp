/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Look/InfoBoxLook.hpp"
#include "InputEvents.hpp"
#include "Compatibility/string.h"
#include "Screen/UnitSymbol.hpp"
#include "Screen/Layout.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Appearance.hpp"
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/SubCanvas.hpp"
#endif

#include <algorithm>
#include <stdio.h>
#include <assert.h>

using std::max;

#define SELECTORWIDTH IBLSCALE(5)

InfoBoxWindow::InfoBoxWindow(ContainerWindow &_parent, int X, int Y, int Width, int Height,
                 int border_flags,
                 const InfoBoxLook &_look)
  :content(NULL),
   parent(_parent),
   look(_look),
   mBorderKind(border_flags),
   focus_timer(0)
{
  colorValue = 0;
  colorTitle = 0;
  colorComment = 0;

  WindowStyle style;
  style.enable_double_clicks();
  set(parent, X, Y, Width, Height, style);

  mValueUnit = unUndef;

  mTitle.clear();
  mValue.clear();
  mComment.clear();
  mID = -1;
}

void
InfoBoxWindow::SetValueUnit(Units_t Value)
{
  if (mValueUnit == Value)
    return;

  mValueUnit = Value;
  invalidate(recValue);
}

void
InfoBoxWindow::SetID(const int id)
{
  mID = id;
}

void
InfoBoxWindow::SetTitle(const TCHAR *Value)
{
  if (!mTitle.equals(Value)) {
    mTitle = Value;
    invalidate(recTitle);
  }
}

void
InfoBoxWindow::SetValue(const TCHAR *Value)
{
  if (!mValue.equals(Value)) {
    mValue = Value;
    invalidate(recValue);
  }
}

void
InfoBoxWindow::SetValue(Angle value, const TCHAR *suffix)
{
  assert(suffix != NULL);

  TCHAR tmp[32];
  _stprintf(tmp, _T("%d")_T(DEG)_T("%s"),
            iround(value.value_degrees()), suffix);
  SetValue(tmp);
}

void
InfoBoxWindow::SetColor(int value)
{
  if (!Appearance.InfoBoxColors)
    value = 0;

  if (colorValue == value)
    return;

  colorValue = value;
  invalidate(recValue);
}

void
InfoBoxWindow::SetColorBottom(int value)
{
  if (!Appearance.InfoBoxColors)
    value = 0;

  if (colorComment == value)
    return;

  colorComment = value;
  invalidate(recComment);
}

void
InfoBoxWindow::SetColorTop(int value)
{
  if (!Appearance.InfoBoxColors)
    value = 0;

  if (colorTitle == value)
    return;

  colorTitle = value;
  invalidate(recTitle);
}

void
InfoBoxWindow::SetComment(const TCHAR *Value)
{
  if (!mComment.equals(Value)) {
    mComment = Value;
    invalidate(recComment);
  }
}

void
InfoBoxWindow::SetComment(Angle value, const TCHAR *suffix)
{
  assert(suffix != NULL);

  TCHAR tmp[32];
  _stprintf(tmp, _T("%d")_T(DEG)_T("%s"),
            iround(value.value_degrees()), suffix);
  SetComment(tmp);
}

void
InfoBoxWindow::SetInvalid()
{
  SetColor(0);
  SetValueInvalid();
  SetCommentInvalid();
}

void
InfoBoxWindow::SetValueInvalid()
{
  SetValue(_T("---"));
  SetValueUnit(unUndef);
}

void
InfoBoxWindow::SetCommentInvalid()
{
  SetComment(_T(""));
}


void
InfoBoxWindow::PaintTitle(Canvas &canvas)
{
  if (mTitle.empty())
    return;

  canvas.set_text_color(look.get_title_color(colorTitle));

  const Font &font = *look.title.font;
  canvas.select(font);

  PixelSize tsize = canvas.text_size(mTitle.c_str());

  int halftextwidth = (recTitle.left + recTitle.right - tsize.cx) / 2;
  int x = max(1, (int)recTitle.left + halftextwidth);
  int y = recTitle.top + 1 + font.get_capital_height() -
    font.get_ascent_height();

  canvas.TextAutoClipped(x, y, mTitle.c_str());

  if (Appearance.InfoBoxBorder == apIbTab && halftextwidth > IBLSCALE(3)) {
    int ytop = recTitle.top + font.get_capital_height() / 2;
    int ytopedge = ytop + IBLSCALE(2);
    int ybottom = recTitle.top + IBLSCALE(6) + font.get_capital_height();

    canvas.select(look.border_pen);

    RasterPoint tab[8];
    tab[0].x = tab[1].x = recTitle.left;
    tab[0].y = tab[7].y = ybottom;
    tab[2].x = recTitle.left + IBLSCALE(2);
    tab[2].y = tab[5].y = tab[3].y = tab[4].y = ytop;
    tab[1].y = tab[6].y = ytopedge;
    tab[5].x = recTitle.right - IBLSCALE(2);
    tab[6].x = tab[7].x = recTitle.right;
    tab[3].x = recTitle.left + halftextwidth - IBLSCALE(1);
    tab[4].x = recTitle.right - halftextwidth + IBLSCALE(1);

    canvas.polyline(tab, 4);
    canvas.polyline(tab + 4, 4);
  }
}

void
InfoBoxWindow::PaintValue(Canvas &canvas)
{
  if (mValue.empty())
    return;

  canvas.set_text_color(look.get_value_color(colorValue));

  canvas.select(*look.value.font);
  unsigned ascent_height = look.value.font->get_ascent_height();
  unsigned capital_height = look.value.font->get_capital_height();

  PixelSize tsize = canvas.text_size(mValue.c_str());
  if (tsize.cx > recValue.right - recValue.left) {
    canvas.select(*look.small_font);
    ascent_height = look.small_font->get_ascent_height();
    capital_height = look.small_font->get_capital_height();
    tsize = canvas.text_size(mValue.c_str());
  }

  PixelSize unit_size;
  const UnitSymbol *unit_symbol = GetUnitSymbol(mValueUnit);
  if (unit_symbol != NULL) {
    unit_size = unit_symbol->get_size();
  } else {
    unit_size.cx = 0;
    unit_size.cy = 0;
  }

  int x = max(1, (int)(recValue.left + recValue.right - tsize.cx
                   - Layout::FastScale(unit_size.cx)) / 2);

  int y = recValue.top + 1 - ascent_height +
    (recValue.bottom - recValue.top + capital_height) / 2;

  canvas.TextAutoClipped(x, y, mValue.c_str());

  if (unit_symbol != NULL && colorValue >= 0) {
#ifndef HAVE_CLIPPING
    /* sort-of clipping */
    if (x + tsize.cx >= (int)canvas.get_width())
      return;
#endif

    RasterPoint origin = unit_symbol->get_origin(Appearance.InverseInfoBox
                                           ? UnitSymbol::INVERSE
                                           : UnitSymbol::NORMAL);

    canvas.scale_copy(x + tsize.cx,
                      y + ascent_height
                      - Layout::FastScale(unit_size.cy),
                      *unit_symbol,
                      origin.x, origin.y,
                      unit_size.cx, unit_size.cy);
  }
}

void
InfoBoxWindow::PaintComment(Canvas &canvas)
{
  if (mComment.empty())
    return;

  canvas.set_text_color(look.get_comment_color(colorComment));

  const Font &font = *look.comment.font;
  canvas.select(font);

  PixelSize tsize = canvas.text_size(mComment);

  int x = max(1, (int)(recComment.left + recComment.right - tsize.cx) / 2);
  int y = recComment.top + 1 + font.get_capital_height()
    - font.get_ascent_height();

  canvas.TextAutoClipped(x, y, mComment);
}

void
InfoBoxWindow::PaintSelector(Canvas &canvas)
{
  canvas.select(look.selector_pen);

  const unsigned width = canvas.get_width(), height = canvas.get_height();

  canvas.two_lines(width - SELECTORWIDTH - 1, 0,
                   width - 1, 0,
                   width - 1, SELECTORWIDTH + 1);

  canvas.two_lines(width - 1, height - SELECTORWIDTH - 2,
                   width - 1, height - 1,
                   width - SELECTORWIDTH - 1, height - 1);

  canvas.two_lines(SELECTORWIDTH + 1, height - 1,
                   0, height - 1,
                   0, height - SELECTORWIDTH - 2);

  canvas.two_lines(0, SELECTORWIDTH + 1,
                   0, 0,
                   SELECTORWIDTH + 1, 0);
}

void
InfoBoxWindow::Paint(Canvas &canvas)
{
  canvas.clear(look.background_brush);

  if (content != NULL)
    content->on_custom_paint(*this, canvas);

  canvas.background_transparent();

  PaintTitle(canvas);
  PaintComment(canvas);
  PaintValue(canvas);

  if (mBorderKind != 0) {
    canvas.select(look.border_pen);

    const unsigned width = canvas.get_width(), height = canvas.get_height();

    if (mBorderKind & BORDERTOP) {
      canvas.line(0, 0, width - 1, 0);
    }

    if (mBorderKind & BORDERRIGHT) {
      canvas.line(width - 1, 0, width - 1, height);
    }

    if (mBorderKind & BORDERBOTTOM) {
      canvas.line(0, height - 1, width - 1, height - 1);
    }

    if (mBorderKind & BORDERLEFT) {
      canvas.line(0, 0, 0, height - 1);
    }
  }
}

void
InfoBoxWindow::PaintInto(Canvas &dest, int xoff, int yoff, int width, int height)
{
#ifdef ENABLE_OPENGL
  SubCanvas canvas(dest, xoff, yoff, width, height);
  Paint(canvas);
#else
  PixelSize size = get_size();
  BufferCanvas buffer(dest, size.cx, size.cy);

  Paint(buffer);
  dest.stretch(xoff, yoff, width, height, buffer, 0, 0, size.cx, size.cy);
#endif
}

void
InfoBoxWindow::SetContentProvider(InfoBoxContent *_content)
{
  delete content;
  content = _content;

  SetColor(0);
  SetColorTop(0);
  SetColorBottom(0);

  SetInvalid();
}

bool
InfoBoxWindow::UpdateContent()
{
  if (content != NULL) {
    content->Update(*this);
    return true;
  }
  return false;
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

bool
InfoBoxWindow::HandleQuickAccess(const TCHAR *Value)
{
  if (content != NULL && content->HandleQuickAccess(Value)) {
    UpdateContent();
    return true;
  }
  return false;
}

InfoBoxContent::DialogContent*
InfoBoxWindow::GetDialogContent()
{
  if (content != NULL)
    return content->GetDialogContent();

  return NULL;
}

bool
InfoBoxWindow::on_resize(unsigned width, unsigned height)
{
  PaintWindow::on_resize(width, height);

  PixelRect rc = get_client_rect();

  if (mBorderKind & BORDERLEFT)
    rc.left += look.BORDER_WIDTH;

  if (mBorderKind & BORDERRIGHT)
    rc.right -= look.BORDER_WIDTH;

  if (mBorderKind & BORDERTOP)
    rc.top += look.BORDER_WIDTH;

  if (mBorderKind & BORDERBOTTOM)
    rc.bottom -= look.BORDER_WIDTH;

  recTitle = rc;
  recTitle.bottom = rc.top + look.title.font->get_capital_height() + 2;

  recComment = rc;
  recComment.top = recComment.bottom
    - (look.comment.font->get_capital_height() + 2);

  recValue = rc;
  recValue.top = recTitle.bottom;
  recValue.bottom = recComment.top;

  recValueAndComment = recValue;
  recValueAndComment.bottom = recComment.bottom;

  return true;
}

bool
InfoBoxWindow::on_key_down(unsigned key_code)
{
  // Get the input event_id of the event
  unsigned event_id = InputEvents::key_to_event(InputEvents::MODE_INFOBOX,
                                                key_code);
  if (event_id > 0) {
    // If input event exists -> process it
    InputEvents::processGo(event_id);

    // restart focus timer if not idle
    if (focus_timer != 0)
      kill_timer(focus_timer);

    focus_timer = set_timer(100, FOCUSTIMEOUTMAX * 500);
    return true;
  }

  return PaintWindow::on_key_down(key_code);
}

bool
InfoBoxWindow::on_mouse_down(int x, int y)
{
  click_clock.update();

  // if single clicked -> focus the InfoBoxWindow
  set_focus();
  return true;
}

bool
InfoBoxWindow::on_mouse_up(int x, int y)
{
  if (!has_focus()) return PaintWindow::on_mouse_up(x, y);

  if (click_clock.check(1000)) {
    InfoBoxManager::ShowDlgInfoBox(mID);
    return true;
  } else
    return PaintWindow::on_mouse_up(x, y);
}

bool
InfoBoxWindow::on_mouse_double(int x, int y)
{
  if (!is_altair()) {
    // JMW capture double click, so infoboxes double clicked also bring up menu
    // VENTA3: apparently this is working only on PC ! Disable it to let PC work
    // with same timeout of PDA and PNA versions with synthetic DBLCLK
    InputEvents::ShowMenu();
  }

  return true;
}

void
InfoBoxWindow::on_paint(Canvas &canvas)
{
  // Call the parent function
  Paint(canvas);

  // Paint the selector
  if (has_focus())
    PaintSelector(canvas);
}

bool
InfoBoxWindow::on_setfocus()
{
  // Call the parent function
  PaintWindow::on_setfocus();

  // Start the focus-auto-return timer
  // to automatically return focus back to MapWindow if idle
  focus_timer = set_timer(100, FOCUSTIMEOUTMAX * 500);

  // Redraw fast to paint the selector
  invalidate();

  return true;
}

bool
InfoBoxWindow::on_killfocus()
{
  // Call the parent function
  PaintWindow::on_killfocus();

  // Destroy the time if it exists
  if (focus_timer != 0) {
    kill_timer(focus_timer);
    focus_timer = 0;
  }

  // Redraw fast to remove the selector
  invalidate();

  return true;
}

bool
InfoBoxWindow::on_timer(timer_t id)
{
  if (id != focus_timer)
    return PaintWindow::on_timer(id);

  kill_timer(focus_timer);
  focus_timer = 0;

  parent.set_focus();

  return true;
}
