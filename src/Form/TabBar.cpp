/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#include "Screen/PaintWindow.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"

#include <assert.h>


TabBarControl::TabBarControl(ContainerWindow &_parent,
                int x, int y, unsigned width, unsigned height,
                const WindowStyle style):
                TabbedControl(_parent, x, y, width, height, style),
                theTabDisplay(NULL)
{
  TabBarHeight = Layout::Scale(TabHeightInitUnscaled);
  TabLineHeight = Layout::Scale(TabLineHeightInitUnscaled);
  theTabDisplay = new TabDisplay(*this, TabBarHeight);
}

const RECT
TabBarControl::get_client_rectangle()
{
  RECT rc;
  rc.top = TabBarHeight;
  rc.bottom = this->get_bottom();
  rc.left = 0;
  rc.right = this->get_width();

  return rc;
}

bool
TabBarControl::GetButtonIsButtonOnly(unsigned i)
{

  assert(i < buttons.size());

  if (i >= buttons.size())
    return false;

  return buttons[i]->IsButtonOnly;
}

const TCHAR*
TabBarControl::GetButtonCaption(unsigned i)
{
  assert(i < buttons.size());

  if (i >= buttons.size())
    return _T("");

  return buttons[i]->Caption;
}

unsigned
TabBarControl::AddClient(Window *w, const TCHAR* Caption,
    bool IsButtonOnly,
    PreHideNotifyCallback_t PreHideFunction,
    PreShowNotifyCallback_t PreShowFunction,
    PostShowNotifyCallback_t PostShowFunction)
{
  if (GetCurrentPage() != buttons.size())
    w->hide();

  TabbedControl::AddClient(w);
  const RECT rc = get_client_rect();
  w->move(rc.left, rc.top + TabBarHeight, rc.right, rc.bottom  + TabBarHeight);

  OneTabButton *b = new OneTabButton(Caption, IsButtonOnly,
      PreHideFunction, PreShowFunction, PostShowFunction);

  buttons.append(b);

  return buttons.size() - 1;
}

void
TabBarControl::SetCurrentPage(unsigned i)
{
  bool Continue = true;
  assert(i < buttons.size());

  if (buttons[GetCurrentPage()]->PreHideFunction) {
    if (!buttons[GetCurrentPage()]->PreHideFunction())
        Continue = false;
  }

  if (Continue) {
    if (buttons[i]->PreShowFunction) {
      Continue = buttons[i]->PreShowFunction();
    }
  }

  if (Continue) {
    TabbedControl::SetCurrentPage(i);
    if (buttons[i]->PostShowFunction) {
      buttons[i]->PostShowFunction();
    }
  }
  theTabDisplay->trigger_invalidate();
}

void
TabBarControl::NextPage()
{
  if (buttons.size() < 2)
    return;

  assert(GetCurrentPage() < buttons.size());

  SetCurrentPage((GetCurrentPage() + 1) % buttons.size());
}

void
TabBarControl::PreviousPage()
{
  if (buttons.size() < 2)
    return;

  assert(GetCurrentPage() < buttons.size());

  SetCurrentPage((GetCurrentPage() + buttons.size() - 1) % buttons.size());
}

const RECT
TabBarControl::GetButtonSize(unsigned i)
{
  const unsigned screenwidth = get_width();
  const unsigned margin = 1;
  const unsigned but_width = (screenwidth - margin) / buttons.size() - margin;
  RECT rc;

  rc.top = 0;
  rc.bottom = rc.top + TabBarHeight - TabLineHeight;

  rc.left = margin + (margin + but_width) * i;
  rc.right = rc.left + but_width;
  if (i == buttons.size() - 1)
    rc.right = screenwidth - margin - 1;

  return rc;
}

// TabDisplay Functions
TabDisplay::TabDisplay(TabBarControl& _theTabBar,
    unsigned height) :
  PaintWindow(),
  theTabBar(_theTabBar),
  dragging(false),
  downindex(-1)
{
  WindowStyle mystyle;
  mystyle.tab_stop();
  set(theTabBar, 0, 0, theTabBar.get_width(), height, mystyle);
}

void
TabDisplay::on_paint(Canvas &canvas)
{
  canvas.clear(Color::BLACK);
  canvas.select(Fonts::MapBold);
  const unsigned CaptionStyle = DT_EXPANDTABS | DT_CENTER | DT_NOCLIP
      | DT_WORDBREAK;

  for (unsigned i = 0; i < theTabBar.GetTabCount(); i++) {
    if (i == theTabBar.GetCurrentPage()) {
      canvas.set_text_color(Color::WHITE);
      canvas.set_background_color(Color::BLACK);

    } else if ((int)i == downindex) {
      canvas.set_text_color(Color::BLACK);
      canvas.set_background_color(Color::YELLOW);

    } else {
      canvas.set_text_color(Color::BLACK);
      canvas.set_background_color(Color::WHITE);
    }
    const RECT &rc = theTabBar.GetButtonSize(i);

    RECT rcText = rc;
    RECT rcTextFinal = rc;
    const unsigned buttonheight = rc.bottom - rc.top;
    const int textwidth = canvas.text_width(theTabBar.GetButtonCaption(i));
    const int textheight = canvas.text_height(theTabBar.GetButtonCaption(i));
    unsigned textheightoffset = 0;

    if (textwidth >= rc.right - rc.left) // assume 2 lines
      textheightoffset = max(0, (int)(buttonheight - textheight * 2) / 2);
    else
      textheightoffset = max(0, (int)(buttonheight - textheight) / 2);

    rcTextFinal.top += textheightoffset;

    //button-only formatting
    if (theTabBar.GetButtonIsButtonOnly(i)
        && (int)i != downindex) {
      canvas.draw_button(rc, false);
      canvas.background_transparent();
    } else {
      canvas.fill_rectangle(rc, canvas.get_background_color());
    }
    canvas.formatted_text(&rcTextFinal, theTabBar.GetButtonCaption(i),
        CaptionStyle);
  }
  if (has_focus()) {
    RECT rcFocus;
    rcFocus.top = rcFocus.left = 0;
    rcFocus.right = canvas.get_width();
    rcFocus.bottom = canvas.get_height();
    canvas.draw_focus(rcFocus);
  }
  this->show();
}

bool
TabDisplay::on_killfocus()
{
  invalidate();
  PaintWindow::on_killfocus();

  return true;
}

bool
TabDisplay::on_setfocus()
{
  invalidate();
  PaintWindow::on_setfocus();
  return true;
}

bool
TabDisplay::on_key_check(unsigned key_code) const
{
  switch (key_code) {

  case VK_RETURN:
    return false; // ToDo
    break;

  case VK_LEFT:
    return (theTabBar.GetCurrentPage() > 0);

  case VK_RIGHT:
    return theTabBar.GetCurrentPage() < theTabBar.GetTabCount();

  case VK_DOWN:
  case VK_UP:
  default:
    return false;
  }
}


bool
TabDisplay::on_key_down(unsigned key_code)
{
  switch (key_code) {
#ifdef GNAV
  // JMW added this to make data entry easier
  case VK_F4:
#endif
  case VK_RETURN: //ToDo: support Return
    break;

  case VK_RIGHT:
    if (theTabBar.GetCurrentPage() < theTabBar.GetTabCount()) {
      theTabBar.NextPage();
      return true;
    }
    break;

  case VK_LEFT:
    if (theTabBar.GetCurrentPage() > 0) {
      theTabBar.PreviousPage();
      return true;
    }
  }

  return PaintWindow::on_key_down(key_code);
}

bool
TabDisplay::on_mouse_down(int x, int y)
{
  drag_end();

  POINT Pos;
  Pos.x = x;
  Pos.y = y;

  // If possible -> Give focus to the Control
  bool had_focus = has_focus();
  if (!had_focus)
    set_focus();
  for (unsigned i = 0; i < theTabBar.GetTabCount(); i++) {
    const RECT rc = theTabBar.GetButtonSize(i);
    if (PtInRect(&rc, Pos)
        && i != theTabBar.GetCurrentPage()) {
      dragging = true;
      downindex = i;
      set_capture();
      invalidate();
      return true;
    }
  }
  return PaintWindow::on_mouse_down(x, y);
}

bool
TabDisplay::on_mouse_up(int x, int y)
{
  POINT Pos;
  Pos.x = x;
  Pos.y = y;

  if (dragging) {
    drag_end();
    for (unsigned i = 0; i < theTabBar.GetTabCount(); i++) {
      const RECT rc = theTabBar.GetButtonSize(i);
      if (PtInRect(&rc, Pos)) {
        if ((int)i == downindex) {
          theTabBar.SetCurrentPage(i);
          break;
        }
      }
    }
    if (downindex > -1)
      invalidate();
    downindex = -1;
    return true;
  } else {
    return PaintWindow::on_mouse_up(x, y);
  }
}

void
TabDisplay::drag_end()
{
  if (dragging) {
    dragging = false;
    release_capture();
  }
}
