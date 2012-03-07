/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Form/TabBar.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Key.h"
#include "Screen/Icon.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"

#include <assert.h>
#include <winuser.h>

TabDisplay::TabDisplay(TabBarControl& _theTabBar, const DialogLook &_look,
                       ContainerWindow &parent,
                       PixelScalar left, PixelScalar top,
                       UPixelScalar width, UPixelScalar height,
                       bool _flipOrientation)
  :tab_bar(_theTabBar),
   look(_look),
   dragging(false),
   down_index(-1),
   drag_off_button(false),
   flip_orientation(_flipOrientation)
{
  WindowStyle mystyle;
  mystyle.TabStop();
  set(parent, left, top, width, height, mystyle);
}

TabDisplay::~TabDisplay()
{
  for (auto i = buttons.begin(), end = buttons.end(); i != end; ++i)
    delete *i;
}

const PixelRect &
TabDisplay::GetButtonSize(unsigned i) const
{
  assert(i < GetSize());

  if (buttons[i]->but_size.left < buttons[i]->but_size.right)
    return buttons[i]->but_size;

  const UPixelScalar margin = 1;

  /*
  bool partialTab = false;
  if ( ((Layout::landscape ^ flip_orientation) && tab_display->GetTabHeight() < get_height()) ||
      ((!Layout::landscape ^ flip_orientation) && tab_display->GetTabWidth() < get_width()) )
    partialTab = true;
  */

  const UPixelScalar finalmargin = 1; //partialTab ? tab_line_height - 1 * margin : margin;
  // Todo make the final margin display on either beginning or end of tab bar
  // depending on position of tab bar

  PixelRect rc;

  if (Layout::landscape ^ flip_orientation) {
    const UPixelScalar but_height =
       (GetTabHeight() - finalmargin) / GetSize() - margin;

    rc.left = 0;
    rc.right = GetTabWidth() - tab_bar.GetTabLineHeight();

    rc.top = finalmargin + (margin + but_height) * i;
    rc.bottom = rc.top + but_height;

  } else {
    const unsigned portraitRows = (GetSize() > 4) ? 2 : 1;

    const unsigned portraitColumnsRow0 = ((portraitRows == 1)
       ? GetSize() : GetSize() / 2);
    const unsigned portraitColumnsRow1 = ((portraitRows == 1)
       ? 0 : GetSize() - GetSize() / 2);

    const unsigned row = (i > (portraitColumnsRow0 - 1)) ? 1 : 0;

    const UPixelScalar rowheight = (GetTabHeight() - tab_bar.GetTabLineHeight())
        / portraitRows - margin;

    const UPixelScalar but_width =
          (GetTabWidth() - finalmargin) /
          ((row == 0) ? portraitColumnsRow0 : portraitColumnsRow1) - margin;

    rc.top = row * (rowheight + margin);
    rc.bottom = rc.top + rowheight;

    rc.left = finalmargin + (margin + but_width) * (row ? (i - portraitColumnsRow0) : i);
    rc.right = rc.left + but_width;
  }

  buttons[i]->but_size = rc;
  return buttons[i]->but_size;
}

void
TabDisplay::PaintButton(Canvas &canvas, const unsigned CaptionStyle,
                        const TCHAR *caption, const PixelRect &rc,
                        bool isButtonOnly, const Bitmap *bmp,
                        const bool isDown, bool inverse)
{

  PixelRect rcTextFinal = rc;
  const UPixelScalar buttonheight = rc.bottom - rc.top;
  const PixelSize text_size = canvas.CalcTextSize(caption);
  const int textwidth = text_size.cx;
  const int textheight = text_size.cy;
  UPixelScalar textheightoffset = 0;

  if (textwidth > (rc.right - rc.left)) // assume 2 lines
    textheightoffset = max(0, (int)(buttonheight - textheight * 2) / 2);
  else
    textheightoffset = max(0, (int)(buttonheight - textheight) / 2);

  rcTextFinal.top += textheightoffset;

  //button-only formatting
  if (isButtonOnly
      && !isDown) {
    canvas.DrawButton(rc, false);
    canvas.SetBackgroundTransparent();
  } else {
    canvas.DrawFilledRectangle(rc, canvas.GetBackgroundColor());
  }
  if (bmp != NULL) {
    const PixelSize bitmap_size = bmp->GetSize();
    const int offsetx = (rc.right - rc.left - bitmap_size.cx / 2) / 2;
    const int offsety = (rc.bottom - rc.top - bitmap_size.cy) / 2;

    if (inverse) // black background
      canvas.CopyNotOr(rc.left + offsetx,
                       rc.top + offsety,
                       bitmap_size.cx / 2,
                       bitmap_size.cy,
                       *bmp,
                       bitmap_size.cx / 2, 0);

    else
      canvas.copy_and(rc.left + offsetx,
                      rc.top + offsety,
                      bitmap_size.cx / 2,
                      bitmap_size.cy,
                      *bmp,
                      bitmap_size.cx / 2, 0);

  } else {
    canvas.formatted_text(&rcTextFinal, caption,
        CaptionStyle);
  }
}

void
TabDisplay::Add(const TCHAR *caption, bool button_only, const Bitmap *bmp)
{
  OneTabButton *b = new OneTabButton(caption, button_only, bmp);
  buttons.append(b);
}

int
TabDisplay::GetButtonIndexAt(RasterPoint p) const
{
  for (unsigned i = 0; i < GetSize(); i++) {
    const PixelRect &rc = GetButtonSize(i);
    if (PtInRect(&rc, p))
      return i;
  }

  return -1;
}

void
TabDisplay::OnPaint(Canvas &canvas)
{
  canvas.clear(COLOR_BLACK);
  canvas.Select(*look.button.font);

  const unsigned CaptionStyle = DT_EXPANDTABS | DT_CENTER | DT_NOCLIP
      | DT_WORDBREAK;

  const bool is_focused = has_focus();
  for (unsigned i = 0; i < buttons.size(); i++) {
    const OneTabButton &button = *buttons[i];

    const bool is_down = (int)i == down_index && !drag_off_button;
    const bool is_selected = i == tab_bar.GetCurrentPage();

    canvas.SetTextColor(look.list.GetTextColor(is_selected, is_focused,
                                               is_down));
    canvas.SetBackgroundColor(look.list.GetBackgroundColor(is_selected,
                                                           is_focused,
                                                           is_down));

    const PixelRect &rc = GetButtonSize(i);
    PaintButton(canvas, CaptionStyle,
                button.caption,
                rc,
                button.is_button_only,
                button.bmp,
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

bool
TabDisplay::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {

  case VK_APP1:
  case VK_APP2:
  case VK_APP3:
  case VK_APP4:
    return true;

  case VK_RETURN:
    return true;

  case VK_LEFT:
    return (tab_bar.GetCurrentPage() > 0);

  case VK_RIGHT:
    return tab_bar.GetCurrentPage() < GetSize() - 1;

  case VK_DOWN:
    return false;

  case VK_UP:
    return false;

  default:
    return false;
  }
}


bool
TabDisplay::OnKeyDown(unsigned key_code)
{
  switch (key_code) {

  case VK_APP1:
    if (GetSize() > 0)
      tab_bar.ClickPage(0);
    return true;

  case VK_APP2:
    if (GetSize() > 1)
      tab_bar.ClickPage(1);
    return true;

  case VK_APP3:
    if (GetSize() > 2)
      tab_bar.ClickPage(2);
    return true;

  case VK_APP4:
    if (GetSize() > 3)
      tab_bar.ClickPage(3);
    return true;

  case VK_RETURN:
    tab_bar.ClickPage(tab_bar.GetCurrentPage());
    return true;

  case VK_DOWN:
    break;

  case VK_RIGHT:
    tab_bar.NextPage();
    return true;

  case VK_UP:
    break;

  case VK_LEFT:
    tab_bar.PreviousPage();
    return true;
  }
  return PaintWindow::OnKeyDown(key_code);
}

bool
TabDisplay::OnMouseDown(PixelScalar x, PixelScalar y)
{
  drag_end();

  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  // If possible -> Give focus to the Control
  SetFocus();

  int i = GetButtonIndexAt(Pos);
  if (i >= 0) {
    dragging = true;
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
  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  if (dragging) {
    drag_end();

    int i = GetButtonIndexAt(Pos);
    if (i == down_index)
      tab_bar.ClickPage(i);

    if (down_index > -1)
      Invalidate();
    down_index = -1;
    return true;
  } else {
    return PaintWindow::OnMouseUp(x, y);
  }
}

bool
TabDisplay::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  if (down_index == -1)
    return false;

  const PixelRect rc = GetButtonSize(down_index);
  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  const bool tmp = !PtInRect(&rc, Pos);
  if (drag_off_button != tmp) {
    drag_off_button = tmp;
    Invalidate(rc);
  }
  return true;
}

void
TabDisplay::drag_end()
{
  if (dragging) {
    dragging = false;
    drag_off_button = false;
    ReleaseCapture();
  }
}
