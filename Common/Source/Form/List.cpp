/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Form/List.hpp"

#include <assert.h>

#include <algorithm>

using std::min;
using std::max;

WndListFrame::WndListFrame(WindowControl *Owner, const TCHAR *Name,
                           int X, int Y, int Width, int Height):
  WndFrame(Owner, Name, X, Y, Width, Height),
  mOnListEnterCallback(NULL),
  CursorCallback(NULL),
  PaintItemCallback(NULL)
{
  SetCanFocus(true);
  PaintSelector(true);

  mListInfo.ItemIndex = 0;
  mListInfo.DrawIndex = 0;
  mListInfo.ItemCount = 0;
  mListInfo.ItemInViewCount = 1;

  mCaption[0] = '\0';
  SetForeColor(GetOwner()->GetForeColor());
  SetBackColor(GetOwner()->GetBackColor());
}

void
WndListFrame::show_or_hide_scroll_bar()
{
  if (mClientCount == 0)
    return;

  const SIZE size = get_size();

  if (mListInfo.ItemCount > mListInfo.ItemInViewCount)
    /* enable the scroll bar */
    scroll_bar.set(size);
  else
    /* all items are visible - hide the scroll bar */
    scroll_bar.reset();
}

bool
WndListFrame::on_resize(unsigned width, unsigned height)
{
  WndFrame::on_resize(width, height);
  show_or_hide_scroll_bar();
  return true;
}

void
WndListFrame::on_paint(Canvas &canvas)
{
  int i;

  if (mClientCount > 0)
    mClients[0]->hide();

  WndFrame::on_paint(canvas);

  if (PaintItemCallback != NULL && mClientCount > 0) {
    // paint using the PaintItemCallback
    RECT rc = mClients[0]->get_position();
    rc.right = scroll_bar.get_left(get_size()) - rc.left;

    for (i = 0; i < mListInfo.ItemInViewCount; i++) {
      if (GetFocused() && i == mListInfo.ItemIndex) {
        Brush brush(GetBackColor().highlight());
        canvas.fill_rectangle(rc, brush);
      }

      PaintItemCallback(canvas, rc, mListInfo.ScrollIndex + i);

      if (i == mListInfo.ItemIndex)
        PaintSelector(canvas, rc);

      ::OffsetRect(&rc, 0, rc.bottom - rc.top);
    }
  }

  DrawScrollBar(canvas);
}

void WndListFrame::DrawScrollBar(Canvas &canvas) {
  if (!scroll_bar.defined())
    return;

  scroll_bar.set_button(mListInfo.ItemCount, mListInfo.ItemInViewCount,
                        mListInfo.ScrollIndex);
  scroll_bar.paint(canvas, GetForeColor());
}


void WndListFrame::SetEnterCallback(void
                                    (*OnListCallback)(WindowControl *Sender,
                                                      ListInfo_t *ListInfo))
{
  mOnListEnterCallback = OnListCallback;
}

void
WndListFrame::SetLength(unsigned n)
{
  if (n == (unsigned)mListInfo.ItemCount)
    return;

  int cursor = GetCursorIndex();

  mListInfo.ItemCount = n;

  if (n == 0)
    cursor = 0;
  else if (cursor >= (int)n)
    cursor = n - 1;

  mListInfo.ItemInViewCount = mClientCount > 0
    ? max(1, (int)(get_size().cy / mClients[0]->get_size().cy))
    : 1;

  if (n <= (unsigned)mListInfo.ItemInViewCount)
    mListInfo.ScrollIndex = 0;
  else if ((unsigned)(mListInfo.ScrollIndex + mListInfo.ItemInViewCount) > n)
    mListInfo.ScrollIndex = n - mListInfo.ItemInViewCount;
  else if (cursor < mListInfo.ScrollIndex)
    mListInfo.ScrollIndex = cursor;

  show_or_hide_scroll_bar();
  invalidate();

  SetCursorIndex(cursor);
}

int WndListFrame::RecalculateIndices(bool bigscroll) {

// scroll to smaller of current scroll or to last page
  mListInfo.ScrollIndex = max(0, min(mListInfo.ScrollIndex,
                                     mListInfo.ItemCount -
                                     mListInfo.ItemInViewCount));

// if we're off end of list, move scroll to last page and select 1st item in last page, return
  if (mListInfo.ItemIndex+mListInfo.ScrollIndex >= mListInfo.ItemCount) {
    mListInfo.ItemIndex = max(0,mListInfo.ItemCount-mListInfo.ScrollIndex-1);
    mListInfo.ScrollIndex = max(0,
			      min(mListInfo.ScrollIndex,
				  mListInfo.ItemCount-mListInfo.ItemIndex-1));
    return 1;
  }

// again, check to see if we're too far off end of list
  mListInfo.ScrollIndex = max(0,
			      min(mListInfo.ScrollIndex,
				  mListInfo.ItemCount-mListInfo.ItemIndex-1));

  if (mListInfo.ScrollIndex + mListInfo.ItemIndex > mListInfo.ItemCount)
    mListInfo.ItemIndex =
      max(mListInfo.ItemCount - mListInfo.ScrollIndex - 1, 0);

  if (mListInfo.ItemIndex >= mListInfo.ItemInViewCount) {
    mListInfo.ScrollIndex++;
    mListInfo.ItemIndex = mListInfo.ItemInViewCount - 1;

    invalidate();
    return 0;
  }
  if (mListInfo.ItemIndex < 0){

    mListInfo.ItemIndex = 0;
    // JMW scroll
    if (mListInfo.ScrollIndex>0) {
      mListInfo.ScrollIndex--;
      invalidate();
      return 0;
    } else {
      // only return if no more scrolling left to do
      return 1;
    }
  }

  invalidate();
  return (0);
}

void
WndListFrame::EnsureVisible(int i)
{
  assert(i >= 0 && i < mListInfo.ItemCount);

  if (mListInfo.ScrollIndex > i)
    mListInfo.ScrollIndex = i;
  else if (mListInfo.ScrollIndex + mListInfo.ItemInViewCount <= i)
    mListInfo.ScrollIndex = i - mListInfo.ItemInViewCount + 1;
  else
    /* no change, no repaint required */
    return;

  invalidate();
}

bool
WndListFrame::SetCursorIndex(int i)
{
  if (i < 0 || i >= mListInfo.ItemCount)
    return false;

  if (i == GetCursorIndex())
    return true;

  EnsureVisible(i);

  mListInfo.ItemIndex = i - mListInfo.ScrollIndex;
  invalidate();

  if (CursorCallback != NULL)
    CursorCallback(GetCursorIndex());
  return true;
}

bool
WndListFrame::on_key_down(unsigned key_code)
{
  // XXX SetSourceRectangle(mRc);

  scroll_bar.drag_end(this);

  switch (key_code) {
#ifdef GNAV
    // JMW added this to make data entry easier
  case VK_F4:
#endif
  case VK_RETURN:
    if (mOnListEnterCallback == NULL)
      break;

    mOnListEnterCallback(this, &mListInfo);
    invalidate();
    return true;

    //#ifndef GNAV

  case VK_LEFT:
    if (mListInfo.ScrollIndex <= 0 ||
        mListInfo.ItemCount <= mListInfo.ItemInViewCount)
      break;

    mListInfo.ScrollIndex -= mListInfo.ItemInViewCount;
    RecalculateIndices(true);

    if (CursorCallback != NULL)
      CursorCallback(GetCursorIndex());
    return true;

  case VK_RIGHT:
    if (mListInfo.ItemIndex + mListInfo.ScrollIndex >= mListInfo.ItemCount ||
        mListInfo.ItemCount <= mListInfo.ItemInViewCount)
      break;

    mListInfo.ScrollIndex += mListInfo.ItemInViewCount;
    RecalculateIndices(true);

    if (CursorCallback != NULL)
      CursorCallback(GetCursorIndex());
    return true;

    //#endif
  case VK_DOWN:
    if (GetCursorIndex() + 1 >= mListInfo.ItemCount)
      break;

    SetCursorIndex(GetCursorIndex() + 1);
    return true;

  case VK_UP:
    if (GetCursorIndex() <= 0)
      break;

    SetCursorIndex(GetCursorIndex() - 1);
    return true;
  }

  return WndFrame::on_key_down(key_code);
}

bool
WndListFrame::on_mouse_up(int x, int y)
{
  scroll_bar.drag_end(this);
    return false;
}

void WndListFrame::SetItemIndex(int iValue){


  mListInfo.ItemIndex=0;  // usually leaves selected item as first in screen
  mListInfo.ScrollIndex=iValue;

  int iTail = mListInfo.ItemCount - iValue; // if within 1 page of end
  if (iTail < mListInfo.ItemInViewCount) {
    int iDiff = mListInfo.ItemInViewCount - iTail;
    int iShrinkBy = min(iValue, iDiff); // don't reduce by

    mListInfo.ItemIndex += iShrinkBy;
    mListInfo.ScrollIndex -= iShrinkBy;
  }

  RecalculateIndices(false);

  if (CursorCallback != NULL)
    CursorCallback(GetCursorIndex());
}

void
WndListFrame::SelectItemFromScreen(int xPos, int yPos)
{
  (void)xPos;
/*  int w = GetWidth()- 4*SELECTORWIDTH;
  int h = GetHeight()- SELECTORWIDTH;

  if ((xPos>= w) && (mListInfo.ItemCount > mListInfo.ItemInViewCount)
      && (mListInfo.ItemCount>0)) {
    // TODO code: scroll!

    mListInfo.ScrollIndex = mListInfo.ItemCount*yPos/h;
    RecalculateIndices(true);

    return;
  }
*/
  int index = yPos / mClients[0]->get_size().cy; // yPos is offset within ListEntry item!

  if (index >= 0 && index + mListInfo.ItemIndex < mListInfo.ItemCount) {
    if (index == mListInfo.ItemIndex) {
      if (mOnListEnterCallback) {
        mOnListEnterCallback(this, &mListInfo);
      }

      invalidate();
    } else {
      SetCursorIndex(mListInfo.ScrollIndex + index);
    }
  }
}


bool
WndListFrame::on_mouse_move(int x, int y, unsigned keys)
{
  static bool bMoving = false;

  if (!bMoving)
  {
    bMoving=true;

    if (scroll_bar.is_dragging()) {
      int iScrollIndex = scroll_bar.drag_move(mListInfo.ItemCount,
                                              mListInfo.ItemInViewCount,
                                              y);

      if(iScrollIndex !=mListInfo.ScrollIndex)
      {
        int iScrollAmount = iScrollIndex - mListInfo.ScrollIndex;
        mListInfo.ScrollIndex = mListInfo.ScrollIndex + iScrollAmount;
        invalidate();
      }
    }

    bMoving=false;
  } // Tickcount
  return false;
}

bool
WndListFrame::on_mouse_down(int x, int y)
{
  scroll_bar.drag_end(this);

  POINT Pos;
  Pos.x = x;
  Pos.y = y;

  if (!GetFocused())
    set_focus();

  if (scroll_bar.in_button(Pos)) // see if click is on scrollbar handle
  {
    // start mouse drag
    scroll_bar.drag_begin(this, Pos.y);
  }
  else if (scroll_bar.in(Pos)) // clicked in scroll bar up/down/pgup/pgdn
  {
    if (scroll_bar.in_up_arrow(Pos.y))
      mListInfo.ScrollIndex = max(0, mListInfo.ScrollIndex- 1);
    else if (scroll_bar.in_down_arrow(Pos.y))
      mListInfo.ScrollIndex = max(0,min(mListInfo.ItemCount- mListInfo.ItemInViewCount, mListInfo.ScrollIndex+ 1));
    else if (scroll_bar.above_button(Pos.y)) // page up
      mListInfo.ScrollIndex = max(0, mListInfo.ScrollIndex- mListInfo.ItemInViewCount);
    else if (scroll_bar.below_button(Pos.y)) // page up
      if (mListInfo.ItemCount > mListInfo.ScrollIndex+ mListInfo.ItemInViewCount)
          mListInfo.ScrollIndex = min ( mListInfo.ItemCount- mListInfo.ItemInViewCount, mListInfo.ScrollIndex +mListInfo.ItemInViewCount);

    invalidate();
  }
  else
  if (mClientCount > 0)
  {
    SelectItemFromScreen(x, y);
  }

  return false;
}

bool
WndListFrame::on_mouse_wheel(int delta)
{
  if (delta > 0) {
    // scroll up
    if (mListInfo.ScrollIndex > 0) {
      --mListInfo.ScrollIndex;
      invalidate();
    }
  } else if (delta < 0) {
    // scroll down
    if (mListInfo.ScrollIndex +
        mListInfo.ItemInViewCount < mListInfo.ItemCount) {
      ++mListInfo.ScrollIndex;
      invalidate();
    }
  }

  return true;
}
