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
#include "Screen/Icon.hpp"
#include "Screen/Canvas.hpp"
#include "Asset.hpp"

#include <assert.h>


TabBarControl::TabBarControl(ContainerWindow &_parent,
                int x, int y, unsigned _width, unsigned _height,
                const WindowStyle style, bool _flipOrientation,
                bool _clientOverlapTabs):
                TabbedControl(_parent, 0, 0, _parent.get_width(), _parent.get_height(), style),
                theTabDisplay(NULL),
                TabLineHeight((Layout::landscape ^ _flipOrientation) ?
                    (Layout::Scale(TabLineHeightInitUnscaled) * 0.75) :
                    Layout::Scale(TabLineHeightInitUnscaled)),
                flipOrientation(_flipOrientation),
                clientOverlapTabs(_clientOverlapTabs),
                setting_up(true)
{
  theTabDisplay = new TabDisplay(*this, x, y, _width, _height, flipOrientation);
}

TabBarControl::~TabBarControl()
{
  delete theTabDisplay;
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

const Bitmap *
TabBarControl::GetButtonIcon(unsigned i)
{
  assert(i < buttons.size());

  if (i >= buttons.size())
    return NULL;

  return buttons[i]->bmp;
}

unsigned
TabBarControl::AddClient(Window *w, const TCHAR* Caption,
    bool IsButtonOnly, const Bitmap *bmp,
    PreHideNotifyCallback_t PreHideFunction,
    PreShowNotifyCallback_t PreShowFunction,
    PostShowNotifyCallback_t PostShowFunction,
    ReClickNotifyCallback_t ReClickFunction)
{
  if (GetCurrentPage() != buttons.size())
    w->hide();

  TabbedControl::AddClient(w);
  const PixelRect rc = get_client_rect();
  if (clientOverlapTabs) {
    w->move(rc.left , rc.top, rc.right - rc.left , rc.bottom - rc.top);
  } else {
    if (Layout::landscape ^ flipOrientation)
      w->move(rc.left + theTabDisplay->GetTabHeight(), rc.top,
              rc.right - rc.left - theTabDisplay->GetTabHeight(),
              rc.bottom - rc.top);
    else
      w->move(rc.left, rc.top + theTabDisplay->GetTabHeight(),
              rc.right - rc.left,
              rc.bottom - rc.top - theTabDisplay->GetTabHeight());
  }

  OneTabButton *b = new OneTabButton(Caption, IsButtonOnly, bmp,
      PreHideFunction, PreShowFunction, PostShowFunction, ReClickFunction);

  buttons.append(b);

  return buttons.size() - 1;
}

void
TabBarControl::SetCurrentPage(unsigned i, EventType _EventType,
                              bool ReClick)
{
  bool Continue = true;
  assert(i < buttons.size());

  if (ReClick) {
    if (buttons[GetCurrentPage()]->ReClickFunction)
      buttons[GetCurrentPage()]->ReClickFunction();

  } else {
    if (!setting_up && buttons[GetCurrentPage()]->PreHideFunction) {
      if (!buttons[GetCurrentPage()]->PreHideFunction())
          Continue = false;
    }

    if (Continue) {
      if (buttons[i]->PreShowFunction) {
        Continue = buttons[i]->PreShowFunction(_EventType);
      }
    }

    if (Continue) {
      TabbedControl::SetCurrentPage(i);
      if (buttons[i]->PostShowFunction) {
        buttons[i]->PostShowFunction();
      }
    }
  }
  theTabDisplay->trigger_invalidate();

  setting_up = false;
}

void
TabBarControl::NextPage(EventType _EventType)
{
  if (buttons.size() < 2)
    return;

  assert(GetCurrentPage() < buttons.size());

  if (!has_pointer() && (GetCurrentPage()+1 >= buttons.size()))
    // prevent wraparound
    return;

  SetCurrentPage((GetCurrentPage() + 1) % buttons.size(), _EventType, false);
}

void
TabBarControl::PreviousPage(EventType _EventType)
{
  if (buttons.size() < 2)
    return;

  assert(GetCurrentPage() < buttons.size());

  if (!has_pointer() && (GetCurrentPage() == 0))
    // prevent wraparound
    return;

  SetCurrentPage((GetCurrentPage() + buttons.size() - 1) % buttons.size(),
                  _EventType, false);
}

const PixelRect
&TabBarControl::GetButtonSize(unsigned i)
{
  const static PixelRect rcFallback = {0, 0, 0, 0};

  if (i >= buttons.size())
    return rcFallback; // should never be used

  if (buttons[i]->butSize.left < buttons[i]->butSize.right)
    return buttons[i]->butSize;

  const unsigned margin = 1;

  /*
  bool partialTab = false;
  if ( ((Layout::landscape ^ flipOrientation) && theTabDisplay->GetTabHeight() < get_height()) ||
      ((!Layout::landscape ^ flipOrientation) && theTabDisplay->GetTabWidth() < get_width()) )
    partialTab = true;
  */

  const unsigned finalmargin = 1; //partialTab ? TabLineHeight - 1 * margin : margin;
  // Todo make the final margin display on either beginning or end of tab bar
  // depending on position of tab bar

  PixelRect rc;

  if (Layout::landscape ^ flipOrientation) {
    const unsigned but_width =
       (theTabDisplay->GetTabHeight() - finalmargin) / buttons.size() - margin;

    rc.left = 0;
    rc.right = theTabDisplay->GetTabWidth() - TabLineHeight;

    rc.top = finalmargin + (margin + but_width) * i;
    rc.bottom = rc.top + but_width;

  } else {
    const unsigned portraitRows = (buttons.size() > 4) ? 2 : 1;

    const unsigned portraitColumnsRow0 = ((portraitRows == 1)
       ? buttons.size() : buttons.size() / 2);
    const unsigned portraitColumnsRow1 = ((portraitRows == 1)
       ? 0 : buttons.size() - buttons.size() / 2);

    const unsigned row = (i > (portraitColumnsRow0 - 1)) ? 1 : 0;

    const unsigned rowheight = (theTabDisplay->GetTabHeight() - TabLineHeight)
        / portraitRows - margin;

    const unsigned but_width =
          (theTabDisplay->GetTabWidth() - finalmargin) /
          ((row == 0) ? portraitColumnsRow0 : portraitColumnsRow1) - margin;

    rc.top = row * (rowheight + margin);
    rc.bottom = rc.top + rowheight;

    rc.left = finalmargin + (margin + but_width) * (row ? (i - portraitColumnsRow0) : i);
    rc.right = rc.left + but_width;
  }

  buttons[i]->butSize = rc;
  return buttons[i]->butSize;
}

unsigned
TabBarControl::GetTabHeight()
{
  return theTabDisplay->GetTabHeight();
}

unsigned
TabBarControl::GetTabWidth()
{
  return theTabDisplay->GetTabWidth();
}

// TabDisplay Functions
TabDisplay::TabDisplay(TabBarControl& _theTabBar,
    unsigned left, unsigned top, unsigned width, unsigned height, bool _flipOrientation) :
  PaintWindow(),
  theTabBar(_theTabBar),
  dragging(false),
  downindex(-1),
  dragoffbutton(false),
  flipOrientation(_flipOrientation)
{
  WindowStyle mystyle;
  mystyle.tab_stop();
  set(theTabBar, left, top, width, height, mystyle);
}

void
TabDisplay::on_paint(Canvas &canvas)
{
  canvas.clear(COLOR_BLACK);
  canvas.select(Fonts::MapBold);
  const unsigned CaptionStyle = DT_EXPANDTABS | DT_CENTER | DT_NOCLIP
      | DT_WORDBREAK;

  for (unsigned i = 0; i < theTabBar.GetTabCount(); i++) {
    bool inverse = false;
    if (((int)i == downindex) && (dragoffbutton == false)) {
      canvas.set_text_color(COLOR_BLACK);
      canvas.set_background_color(COLOR_YELLOW);

    } else if (i == theTabBar.GetCurrentPage()) {
        canvas.set_text_color(COLOR_WHITE);
        if (has_focus() && !has_pointer()) {
          canvas.set_background_color(COLOR_GRAY.highlight());
        } else {
          canvas.set_background_color(COLOR_BLACK);
        }
        inverse = true;

    } else {
      canvas.set_text_color(COLOR_BLACK);
      canvas.set_background_color(COLOR_WHITE);
    }
    const PixelRect &rc = theTabBar.GetButtonSize(i);

    PixelRect rcTextFinal = rc;
    const unsigned buttonheight = rc.bottom - rc.top;
    const int textwidth = canvas.text_width(theTabBar.GetButtonCaption(i));
    const int textheight = canvas.text_height(theTabBar.GetButtonCaption(i));
    unsigned textheightoffset = 0;

    if (textwidth > (rc.right - rc.left)) // assume 2 lines
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
    if (theTabBar.GetButtonIcon(i) != NULL) {

      const Bitmap *bmp = theTabBar.GetButtonIcon(i);
      const int offsetx = (rc.right - rc.left - bmp->get_size().cx / 2) / 2;
      const int offsety = (rc.bottom - rc.top - bmp->get_size().cy) / 2;

      if (inverse) // black background
        canvas.copy_not(rc.left + offsetx,
                    rc.top + offsety,
                    bmp->get_size().cx / 2,
                    bmp->get_size().cy,
                    *bmp,
                    bmp->get_size().cx / 2, 0);

      else
        canvas.copy(rc.left + offsetx,
                    rc.top + offsety,
                    bmp->get_size().cx / 2,
                    bmp->get_size().cy,
                    *bmp,
                    bmp->get_size().cx / 2, 0);

    } else {
      canvas.formatted_text(&rcTextFinal, theTabBar.GetButtonCaption(i),
          CaptionStyle);
    }
  }
  if (has_focus()) {
    PixelRect rcFocus;
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

  case VK_APP1:
  case VK_APP2:
  case VK_APP3:
  case VK_APP4:
    return true;

  case VK_RETURN:
    return true;

  case VK_LEFT:
    return (theTabBar.GetCurrentPage() > 0);

  case VK_RIGHT:
    return theTabBar.GetCurrentPage() < theTabBar.GetTabCount() - 1;

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
    if (theTabBar.GetTabCount() > 0)
      theTabBar.SetCurrentPage(0, TabBarControl::MouseOrButton,
          theTabBar.GetCurrentPage() == 0);
    return true;

  case VK_APP2:
    if (theTabBar.GetTabCount() > 1)
      theTabBar.SetCurrentPage(1, TabBarControl::MouseOrButton,
          theTabBar.GetCurrentPage() == 1);
    return true;

  case VK_APP3:
    if (theTabBar.GetTabCount() > 2)
      theTabBar.SetCurrentPage(2, TabBarControl::MouseOrButton,
          theTabBar.GetCurrentPage() == 2);
    return true;

  case VK_APP4:
    if (theTabBar.GetTabCount() > 3)
      theTabBar.SetCurrentPage(3, TabBarControl::MouseOrButton,
          theTabBar.GetCurrentPage() == 3);
    return true;

  case VK_RETURN:
    theTabBar.SetCurrentPage(theTabBar.GetCurrentPage(),
        TabBarControl::MouseOrButton, true);
    return true;

  case VK_DOWN:
    break;

  case VK_RIGHT:
    theTabBar.NextPage(TabBarControl::NextPreviousKey);
    return true;

  case VK_UP:
    break;

  case VK_LEFT:
    theTabBar.PreviousPage(TabBarControl::NextPreviousKey);
    return true;
  }
  return PaintWindow::on_key_down(key_code);
}

bool
TabDisplay::on_mouse_down(int x, int y)
{
  drag_end();

  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  // If possible -> Give focus to the Control
  bool had_focus = has_focus();
  if (!had_focus)
    set_focus();
  for (unsigned i = 0; i < theTabBar.GetTabCount(); i++) {
    const PixelRect rc = theTabBar.GetButtonSize(i);
    if (PtInRect(&rc, Pos)) {
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
  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  if (dragging) {
    drag_end();
    for (unsigned i = 0; i < theTabBar.GetTabCount(); i++) {
      const PixelRect rc = theTabBar.GetButtonSize(i);
      if (PtInRect(&rc, Pos)) {
        if ((int)i == downindex) {
          theTabBar.SetCurrentPage(i, TabBarControl::MouseOrButton,
              theTabBar.GetCurrentPage() == i);
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

bool
TabDisplay::on_mouse_move(int x, int y, unsigned keys)
{
  if (downindex == -1)
    return false;

  const PixelRect rc = theTabBar.GetButtonSize(downindex);
  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  const bool tmp = !PtInRect(&rc, Pos);
  if (dragoffbutton != tmp) {
    dragoffbutton = tmp;
    invalidate(rc);
  }
  return true;
}

void
TabDisplay::drag_end()
{
  if (dragging) {
    dragging = false;
    dragoffbutton = false;
    release_capture();
  }
}
