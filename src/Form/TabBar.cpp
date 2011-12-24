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

#include "Form/TabBar.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/PaintWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/Icon.hpp"
#include "Screen/Canvas.hpp"
#include "Asset.hpp"

#include <assert.h>
#include <winuser.h>

TabBarControl::TabBarControl(ContainerWindow &_parent, const DialogLook &look,
                             PixelScalar x, PixelScalar y,
                             UPixelScalar _width, UPixelScalar _height,
                             const WindowStyle style, bool _flipOrientation,
                             bool _clientOverlapTabs)
  :tab_display(NULL),
   tab_line_height((Layout::landscape ^ _flipOrientation)
                 ? (Layout::Scale(TabLineHeightInitUnscaled) * 0.75)
                 : Layout::Scale(TabLineHeightInitUnscaled)),
   flip_orientation(_flipOrientation),
   client_overlap_tabs(_clientOverlapTabs)
{
  set(_parent, 0, 0, _parent.get_width(), _parent.get_height(), style),

  tab_display = new TabDisplay(*this, look, *this,
                                 x, y, _width, _height,
                                 flip_orientation);

  PixelRect rc = get_client_rect();
  if (!_clientOverlapTabs) {
    if (Layout::landscape ^ flip_orientation)
      rc.left += tab_display->GetTabWidth();
    else
      rc.top += tab_display->GetTabHeight();
  }

  WindowStyle pager_style;
  pager_style.control_parent();
  pager.set(*this, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
            pager_style);
}

TabBarControl::~TabBarControl()
{
  delete tab_display;

  for (auto i = buttons.begin(), end = buttons.end(); i != end; ++i)
    delete *i;
}

void
TabBarControl::SetClientOverlapTabs(bool value)
{
  if (client_overlap_tabs == value)
    return;

  client_overlap_tabs = value;

  PixelRect rc = get_client_rect();
  if (!client_overlap_tabs) {
    if (Layout::landscape ^ flip_orientation)
      rc.left += tab_display->GetTabWidth();
    else
      rc.top += tab_display->GetTabHeight();
  }

  pager.move(rc);
}

bool
TabBarControl::GetButtonIsButtonOnly(unsigned i) const
{

  assert(i < buttons.size());

  return buttons[i]->is_button_only;
}

const TCHAR*
TabBarControl::GetButtonCaption(unsigned i) const
{
  assert(i < buttons.size());

  return buttons[i]->caption.c_str();
}

const Bitmap *
TabBarControl::GetButtonIcon(unsigned i) const
{
  assert(i < buttons.size());

  return buttons[i]->bmp;
}

unsigned
TabBarControl::AddTab(Widget *widget, const TCHAR *caption,
                      bool button_only, const Bitmap *bmp)
{
  pager.AddPage(widget);

  OneTabButton *b = new OneTabButton(caption, button_only, bmp);
  buttons.append(b);

  return buttons.size() - 1;
}

void
TabBarControl::ClickPage(unsigned i)
{
  assert(i < buttons.size());

  pager.ClickPage(i);

  if (tab_display != NULL)
    tab_display->invalidate();
}

void
TabBarControl::SetCurrentPage(unsigned i)
{
  assert(i < buttons.size());

  pager.SetCurrentPage(i);

  if (tab_display != NULL)
    tab_display->invalidate();
}

void
TabBarControl::NextPage()
{
  if (buttons.size() < 2)
    return;

  assert(GetCurrentPage() < buttons.size());

  if (!HasPointer() && (GetCurrentPage()+1 >= buttons.size()))
    // prevent wraparound
    return;

  SetCurrentPage((GetCurrentPage() + 1) % buttons.size());
}

void
TabBarControl::PreviousPage()
{
  if (buttons.size() < 2)
    return;

  assert(GetCurrentPage() < buttons.size());

  if (!HasPointer() && (GetCurrentPage() == 0))
    // prevent wraparound
    return;

  SetCurrentPage((GetCurrentPage() + buttons.size() - 1) % buttons.size());
}

const PixelRect &
TabBarControl::GetButtonSize(unsigned i) const
{
  const static PixelRect rcFallback = {0, 0, 0, 0};

  if (i >= buttons.size())
    return rcFallback; // should never be used

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
       (tab_display->GetTabHeight() - finalmargin) / buttons.size() - margin;

    rc.left = 0;
    rc.right = tab_display->GetTabWidth() - tab_line_height;

    rc.top = finalmargin + (margin + but_height) * i;
    rc.bottom = rc.top + but_height;

  } else {
    const unsigned portraitRows = (buttons.size() > 4) ? 2 : 1;

    const unsigned portraitColumnsRow0 = ((portraitRows == 1)
       ? buttons.size() : buttons.size() / 2);
    const unsigned portraitColumnsRow1 = ((portraitRows == 1)
       ? 0 : buttons.size() - buttons.size() / 2);

    const unsigned row = (i > (portraitColumnsRow0 - 1)) ? 1 : 0;

    const UPixelScalar rowheight = (tab_display->GetTabHeight() - tab_line_height)
        / portraitRows - margin;

    const UPixelScalar but_width =
          (tab_display->GetTabWidth() - finalmargin) /
          ((row == 0) ? portraitColumnsRow0 : portraitColumnsRow1) - margin;

    rc.top = row * (rowheight + margin);
    rc.bottom = rc.top + rowheight;

    rc.left = finalmargin + (margin + but_width) * (row ? (i - portraitColumnsRow0) : i);
    rc.right = rc.left + but_width;
  }

  buttons[i]->but_size = rc;
  return buttons[i]->but_size;
}

UPixelScalar
TabBarControl::GetTabHeight() const
{
  return tab_display->GetTabHeight();
}

UPixelScalar
TabBarControl::GetTabWidth() const
{
  return tab_display->GetTabWidth();
}

// TabDisplay Functions
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
  mystyle.tab_stop();
  set(parent, left, top, width, height, mystyle);
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
      canvas.copy_not(rc.left + offsetx,
                  rc.top + offsety,
                  bitmap_size.cx / 2,
                  bitmap_size.cy,
                  *bmp,
                  bitmap_size.cx / 2, 0);

    else
      canvas.copy(rc.left + offsetx,
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

int
TabDisplay::GetButtonIndexAt(RasterPoint p) const
{
  for (unsigned i = 0; i < tab_bar.GetTabCount(); i++) {
    const PixelRect &rc = tab_bar.GetButtonSize(i);
    if (PtInRect(&rc, p))
      return i;
  }

  return -1;
}

void
TabDisplay::on_paint(Canvas &canvas)
{
  canvas.clear(COLOR_BLACK);
  canvas.Select(*look.button.font);

  const unsigned CaptionStyle = DT_EXPANDTABS | DT_CENTER | DT_NOCLIP
      | DT_WORDBREAK;

  for (unsigned i = 0; i < tab_bar.GetTabCount(); i++) {
    bool inverse = false;
    if (((int)i == down_index) && (drag_off_button == false)) {
      canvas.SetTextColor(COLOR_BLACK);
      canvas.SetBackgroundColor(COLOR_YELLOW);

    } else if (i == tab_bar.GetCurrentPage()) {
        canvas.SetTextColor(COLOR_WHITE);
        if (has_focus() && !HasPointer()) {
          canvas.SetBackgroundColor(COLOR_GRAY.Highlight());
        } else {
          canvas.SetBackgroundColor(COLOR_BLACK);
        }
        inverse = true;

    } else {
      canvas.SetTextColor(COLOR_BLACK);
      canvas.SetBackgroundColor(COLOR_WHITE);
    }
    const PixelRect &rc = tab_bar.GetButtonSize(i);
    PaintButton(canvas, CaptionStyle,
                tab_bar.GetButtonCaption(i),
                rc,
                tab_bar.GetButtonIsButtonOnly(i),
                tab_bar.GetButtonIcon(i),
                (int)i == down_index, inverse);
  }
  if (has_focus()) {
    PixelRect rcFocus;
    rcFocus.top = rcFocus.left = 0;
    rcFocus.right = canvas.get_width();
    rcFocus.bottom = canvas.get_height();
    canvas.DrawFocusRectangle(rcFocus);
  }
}

void
TabDisplay::on_killfocus()
{
  invalidate();
  PaintWindow::on_killfocus();
}

void
TabDisplay::on_setfocus()
{
  invalidate();
  PaintWindow::on_setfocus();
}

bool
TabDisplay::on_key_check(unsigned key_code) const
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
    return tab_bar.GetCurrentPage() < tab_bar.GetTabCount() - 1;

  case VK_DOWN:
    return false;

  case VK_UP:
    return false;

  default:
    return false;
  }
}


bool
TabDisplay::on_key_down(unsigned key_code)
{
  switch (key_code) {

  case VK_APP1:
    if (tab_bar.GetTabCount() > 0)
      tab_bar.ClickPage(0);
    return true;

  case VK_APP2:
    if (tab_bar.GetTabCount() > 1)
      tab_bar.ClickPage(1);
    return true;

  case VK_APP3:
    if (tab_bar.GetTabCount() > 2)
      tab_bar.ClickPage(2);
    return true;

  case VK_APP4:
    if (tab_bar.GetTabCount() > 3)
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
  return PaintWindow::on_key_down(key_code);
}

bool
TabDisplay::on_mouse_down(PixelScalar x, PixelScalar y)
{
  drag_end();

  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  // If possible -> Give focus to the Control
  set_focus();

  int i = GetButtonIndexAt(Pos);
  if (i >= 0) {
    dragging = true;
    down_index = i;
    set_capture();
    invalidate();
    return true;
  }

  return PaintWindow::on_mouse_down(x, y);
}

bool
TabDisplay::on_mouse_up(PixelScalar x, PixelScalar y)
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
      invalidate();
    down_index = -1;
    return true;
  } else {
    return PaintWindow::on_mouse_up(x, y);
  }
}

bool
TabDisplay::on_mouse_move(PixelScalar x, PixelScalar y, unsigned keys)
{
  if (down_index == -1)
    return false;

  const PixelRect rc = tab_bar.GetButtonSize(down_index);
  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  const bool tmp = !PtInRect(&rc, Pos);
  if (drag_off_button != tmp) {
    drag_off_button = tmp;
    invalidate(rc);
  }
  return true;
}

void
TabDisplay::drag_end()
{
  if (dragging) {
    dragging = false;
    drag_off_button = false;
    release_capture();
  }
}
