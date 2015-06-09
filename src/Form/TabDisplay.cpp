/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Form/TabDisplay.hpp"
#include "Widget/TabWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Key.h"
#include "Screen/Icon.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"

#include <assert.h>
#include <winuser.h>

TabDisplay::TabDisplay(TabWidget &_pager, const DialogLook &_look,
                       ContainerWindow &parent, PixelRect rc,
                       bool _vertical,
                       WindowStyle style)
  :pager(_pager),
   look(_look),
   vertical(_vertical),
   dragging(false),
   tab_line_height(Layout::VptScale(5))
{
  style.TabStop();
  Create(parent, rc, style);
}

TabDisplay::~TabDisplay()
{
  for (const auto i : buttons)
    delete i;
}

inline unsigned
TabButton::GetRecommendedWidth(const DialogLook &look) const
{
  if (icon != nullptr)
    return icon->GetSize().cx + 2 * Layout::GetTextPadding();

  return look.button.font->TextSize(caption).cx + 2 * Layout::GetTextPadding();
}

inline unsigned
TabButton::GetRecommendedHeight(const DialogLook &look) const
{
  if (icon != nullptr)
    return icon->GetSize().cy + 2 * Layout::GetTextPadding();

  return look.button.font->GetHeight() + 2 * Layout::GetTextPadding();
}

unsigned
TabDisplay::GetRecommendedColumnWidth() const
{
  unsigned width = Layout::GetMaximumControlHeight();
  for (auto *i : buttons) {
    unsigned w = i->GetRecommendedWidth(GetLook()) + tab_line_height;
    if (w > width)
      width = w;
  }

  return width;
}

unsigned
TabDisplay::GetRecommendedRowHeight() const
{
  unsigned height = Layout::GetMaximumControlHeight();
  for (auto *i : buttons) {
    unsigned h = i->GetRecommendedHeight(GetLook()) + tab_line_height;
    if (h > height)
      height = h;
  }

  return height;
}

void
TabDisplay::UpdateLayout(const PixelRect &rc, bool _vertical)
{
  vertical = _vertical;
  Move(rc);
}

const PixelRect &
TabDisplay::GetButtonSize(unsigned i) const
{
  assert(i < GetSize());

  if (buttons[i]->rc.left < buttons[i]->rc.right)
    return buttons[i]->rc;

  const unsigned margin = 1;

  /*
  const bool partialTab = vertial
    ? tab_display->GetTabHeight() < GetHeight()
    : tab_display->GetTabWidth() < GetWidth();
  */

  const unsigned finalmargin = 1; //partialTab ? tab_line_height - 1 * margin : margin;
  // Todo make the final margin display on either beginning or end of tab bar
  // depending on position of tab bar

  PixelRect rc;

  if (vertical) {
    const unsigned but_height =
       (GetHeight() - finalmargin) / GetSize() - margin;

    rc.left = 0;
    rc.right = GetWidth() - tab_line_height;

    rc.top = finalmargin + (margin + but_height) * i;
    rc.bottom = rc.top + but_height;

  } else {
    const unsigned portraitRows = (GetSize() > 4) ? 2 : 1;

    const unsigned portraitColumnsRow0 = ((portraitRows == 1)
       ? GetSize() : GetSize() / 2);
    const unsigned portraitColumnsRow1 = ((portraitRows == 1)
       ? 0 : GetSize() - GetSize() / 2);

    const unsigned row = (i > (portraitColumnsRow0 - 1)) ? 1 : 0;

    const unsigned rowheight = (GetHeight() - tab_line_height)
        / portraitRows - margin;

    const unsigned but_width =
          (GetWidth() - finalmargin) /
          ((row == 0) ? portraitColumnsRow0 : portraitColumnsRow1) - margin;

    rc.top = row * (rowheight + margin);
    rc.bottom = rc.top + rowheight;

    rc.left = finalmargin + (margin + but_width) * (row ? (i - portraitColumnsRow0) : i);
    rc.right = rc.left + but_width;
  }

  buttons[i]->rc = rc;
  return buttons[i]->rc;
}

void
TabDisplay::PaintButton(Canvas &canvas,
                        const TCHAR *caption, const PixelRect &rc,
                        const MaskedIcon *icon, bool isDown, bool inverse)
{
  canvas.DrawFilledRectangle(rc, canvas.GetBackgroundColor());

  if (icon != nullptr) {
    icon->Draw(canvas, rc, inverse);
  } else {
    PixelRect rcTextFinal = rc;
    const unsigned buttonheight = rc.bottom - rc.top;
    const PixelSize text_size = canvas.CalcTextSize(caption);
    const int textwidth = text_size.cx;
    const int textheight = text_size.cy;
    unsigned textheightoffset = 0;

    if (textwidth > (rc.right - rc.left)) // assume 2 lines
      textheightoffset = std::max(0, (int)(buttonheight - textheight * 2) / 2);
    else
      textheightoffset = std::max(0, (int)(buttonheight - textheight) / 2);

    rcTextFinal.top += textheightoffset;

    unsigned CaptionStyle = DT_NOPREFIX | DT_CENTER | DT_NOCLIP | DT_WORDBREAK;
#ifndef USE_GDI
    if (IsDithered())
      CaptionStyle |= DT_UNDERLINE;
#endif

    canvas.DrawFormattedText(&rcTextFinal, caption, CaptionStyle);
  }
}

void
TabDisplay::Add(const TCHAR *caption, const MaskedIcon *icon)
{
  TabButton *b = new TabButton(caption, icon);
  buttons.append(b);
}

int
TabDisplay::GetButtonIndexAt(RasterPoint p) const
{
  for (unsigned i = 0; i < GetSize(); i++) {
    const PixelRect &rc = GetButtonSize(i);
    if (rc.IsInside(p))
      return i;
  }

  return -1;
}

void
TabDisplay::OnResize(PixelSize new_size)
{
  PaintWindow::OnResize(new_size);

  for (auto button : buttons)
    button->InvalidateLayout();
}

void
TabDisplay::OnPaint(Canvas &canvas)
{
  canvas.Clear(COLOR_BLACK);
  canvas.Select(*look.button.font);

  const bool is_focused = !HasCursorKeys() || HasFocus();
  for (unsigned i = 0; i < buttons.size(); i++) {
    const TabButton &button = *buttons[i];

    const bool is_down = dragging && i == down_index && !drag_off_button;
    const bool is_selected = i == pager.GetCurrentIndex();

    canvas.SetTextColor(look.list.GetTextColor(is_selected, is_focused,
                                               is_down));
    canvas.SetBackgroundColor(look.list.GetBackgroundColor(is_selected,
                                                           is_focused,
                                                           is_down));

    const PixelRect &rc = GetButtonSize(i);
    PaintButton(canvas, button.caption, rc, button.icon,
                is_down, is_selected);
  }
}

void
TabDisplay::OnKillFocus()
{
  Invalidate();
  PaintWindow::OnKillFocus();
}

void
TabDisplay::OnSetFocus()
{
  Invalidate();
  PaintWindow::OnSetFocus();
}

void
TabDisplay::OnCancelMode()
{
  PaintWindow::OnCancelMode();
  EndDrag();
}

bool
TabDisplay::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {

  case KEY_APP1:
  case KEY_APP2:
  case KEY_APP3:
  case KEY_APP4:
    return true;

  case KEY_RETURN:
    return true;

  case KEY_LEFT:
    return pager.GetCurrentIndex() > 0;

  case KEY_RIGHT:
    return pager.GetCurrentIndex() < GetSize() - 1;

  case KEY_DOWN:
    return false;

  case KEY_UP:
    return false;

  default:
    return false;
  }
}


bool
TabDisplay::OnKeyDown(unsigned key_code)
{
  switch (key_code) {

  case KEY_APP1:
    if (GetSize() > 0)
      pager.ClickPage(0);
    return true;

  case KEY_APP2:
    if (GetSize() > 1)
      pager.ClickPage(1);
    return true;

  case KEY_APP3:
    if (GetSize() > 2)
      pager.ClickPage(2);
    return true;

  case KEY_APP4:
    if (GetSize() > 3)
      pager.ClickPage(3);
    return true;

  case KEY_RETURN:
    pager.ClickPage(pager.GetCurrentIndex());
    return true;

  case KEY_DOWN:
    break;

  case KEY_RIGHT:
    pager.NextPage();
    return true;

  case KEY_UP:
    break;

  case KEY_LEFT:
    pager.PreviousPage();
    return true;
  }
  return PaintWindow::OnKeyDown(key_code);
}

bool
TabDisplay::OnMouseDown(PixelScalar x, PixelScalar y)
{
  EndDrag();

  // If possible -> Give focus to the Control
  SetFocus();

  int i = GetButtonIndexAt({ x, y });
  if (i >= 0) {
    dragging = true;
    drag_off_button = false;
    down_index = i;
    SetCapture();
    Invalidate();
    return true;
  }

  return PaintWindow::OnMouseDown(x, y);
}

bool
TabDisplay::OnMouseUp(PixelScalar x, PixelScalar y)
{
  if (dragging) {
    EndDrag();

    if (!drag_off_button)
      pager.ClickPage(down_index);

    return true;
  } else {
    return PaintWindow::OnMouseUp(x, y);
  }
}

bool
TabDisplay::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  if (!dragging)
    return false;

  const PixelRect rc = GetButtonSize(down_index);

  bool not_on_button = !rc.IsInside({ x, y });
  if (drag_off_button != not_on_button) {
    drag_off_button = not_on_button;
    Invalidate(rc);
  }
  return true;
}

void
TabDisplay::EndDrag()
{
  if (dragging) {
    dragging = false;
    ReleaseCapture();
    Invalidate();
  }
}
