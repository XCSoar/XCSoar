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
#include "Screen/Viewport.hpp"

#include <algorithm>

using std::min;
using std::max;

WndListFrame::WndListFrame(WindowControl *Owner, const TCHAR *Name,
                           int X, int Y, int Width, int Height,
                           void (*OnListCallback)(WindowControl *Sender,
                                                  ListInfo_t *ListInfo)):
  WndFrame(Owner, Name, X, Y, Width, Height),
  PaintItemCallback(NULL)
{
  SetCanFocus(true);
  PaintSelector(true);

  mListInfo.ItemIndex = 0;
  mListInfo.DrawIndex = 0;
  mListInfo.ItemInPageCount = 0;
  mListInfo.TopIndex = 0;
  mListInfo.BottomIndex = 0;
//  mListInfo.SelectedIndex = 0;
  mListInfo.ItemCount = 0;
  mListInfo.ItemInViewCount = 0;

  mCaption[0] = '\0';
  mOnListCallback = OnListCallback;
  mOnListEnterCallback = NULL;
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

  /* now resize the item renderer, according to the width of the
     scroll bar */

  const RECT rc = mClients[0]->get_position();
  mClients[0]->resize(scroll_bar.get_left(size) - rc.left * 2,
                      rc.bottom - rc.top);
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

  if (mClientCount > 0){
    ((WndFrame *)mClients[0])->SetIsListItem(true);
//    ShowWindow(mClients[0]->GetHandle(), SW_HIDE);
/*
    if (mOnListCallback != NULL){
      mListInfo.DrawIndex = mListInfo.ItemIndex;
      mOnListCallback(this, &mListInfo);
      mClients[0]->SetTop(mClients[0]->GetHeight() * (mListInfo.ItemIndex-mListInfo.TopIndex));
    }
*/
  }

  WndFrame::on_paint(canvas);

  if (PaintItemCallback != NULL && mClientCount > 0) {
    // paint using the PaintItemCallback
    RECT rc = mClients[0]->get_position();

    for (i = 0; i < mListInfo.ItemInViewCount; i++) {
      if (GetFocused() && mListInfo.TopIndex + i == mListInfo.ItemIndex) {
        Brush brush(GetBackColor().highlight());
        canvas.fill_rectangle(rc, brush);
      }

      PaintItemCallback(canvas, rc, mListInfo.TopIndex + i);

      if (mListInfo.TopIndex + i == mListInfo.ItemIndex)
        PaintSelector(canvas, rc);

      ::OffsetRect(&rc, 0, rc.bottom - rc.top);
    }
  } else if (mClientCount > 0){
    // paint using the hidden client window
    const RECT rc = mClients[0]->get_position();

    Viewport viewport(canvas, rc.right - rc.left, rc.bottom - rc.top);
    Canvas &canvas2 = viewport;

    viewport.move(rc.left, rc.top);

    for (i=0; i<mListInfo.ItemInViewCount; i++){
      canvas2.select(*mClients[0]->GetFont());

      if (mOnListCallback != NULL){
        mListInfo.DrawIndex = mListInfo.TopIndex + i;
        mOnListCallback(this, &mListInfo);
      }

      mClients[0]->PaintSelector(mListInfo.DrawIndex != mListInfo.ItemIndex);
      mClients[0]->on_paint(canvas2);

      viewport.commit();
      viewport.move(0, rc.bottom - rc.top);
    }

    viewport.restore();

    DrawScrollBar(canvas);
  }
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

int WndListFrame::RecalculateIndices(bool bigscroll) {

// scroll to smaller of current scroll or to last page
  mListInfo.ScrollIndex = max(0,min(mListInfo.ScrollIndex,
				    mListInfo.ItemCount-mListInfo.ItemInPageCount));

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

  if (mListInfo.ItemIndex >= mListInfo.BottomIndex){
    if ((mListInfo.ItemCount>mListInfo.ItemInPageCount)
	&& (mListInfo.ItemIndex+mListInfo.ScrollIndex < mListInfo.ItemCount)) {
      mListInfo.ScrollIndex++;
      mListInfo.ItemIndex = mListInfo.BottomIndex-1;

      invalidate();
      return 0;
    } else {
      mListInfo.ItemIndex = mListInfo.BottomIndex-1;
      return 1;
    }
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
        mListInfo.ItemCount <= mListInfo.ItemInPageCount)
      break;

    mListInfo.ScrollIndex -= mListInfo.ItemInPageCount;
    RecalculateIndices(true);
    return true;

  case VK_RIGHT:
    if (mListInfo.ItemIndex + mListInfo.ScrollIndex >= mListInfo.ItemCount ||
        mListInfo.ItemCount <= mListInfo.ItemInPageCount)
      break;

    mListInfo.ScrollIndex += mListInfo.ItemInPageCount;
    RecalculateIndices(true);
    return true;

    //#endif
  case VK_DOWN:
    if (mListInfo.ItemIndex + mListInfo.ScrollIndex >= mListInfo.ItemCount)
      break;

    mListInfo.ItemIndex++;
    RecalculateIndices(false);
    return true;

  case VK_UP:
    if (mListInfo.ItemIndex + mListInfo.ScrollIndex <= 0)
      break;

    mListInfo.ItemIndex--;
    RecalculateIndices(false);
    return true;
  }

  return WndFrame::on_key_down(key_code);
}

void WndListFrame::ResetList(void){
  unsigned height = get_size().cy;
  unsigned client_height = mClients[0]->get_size().cy;

  mListInfo.ScrollIndex = 0;
  mListInfo.ItemIndex = 0;
  mListInfo.DrawIndex = 0;
  mListInfo.ItemInPageCount = (height + client_height - 1)
    / client_height - 1;
  mListInfo.TopIndex = 0;
  mListInfo.BottomIndex = 0;
//  mListInfo.SelectedIndex = 0;
  mListInfo.ItemCount = 0;
  mListInfo.ItemInViewCount = (height + client_height - 1)
    / client_height - 1;

  if (mOnListCallback != NULL){
    mListInfo.DrawIndex = -1;                               // -1 -> initialize data
    mOnListCallback(this, &mListInfo);
    mListInfo.DrawIndex = 0;                                // setup data for first item,
    mOnListCallback(this, &mListInfo);
  }

  if (mListInfo.BottomIndex  == 0){                         // calc bounds
    mListInfo.BottomIndex  = mListInfo.ItemCount;
    if (mListInfo.BottomIndex > mListInfo.ItemInViewCount){
      mListInfo.BottomIndex = mListInfo.ItemInViewCount;
    }
  }

  show_or_hide_scroll_bar();
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
  if ( iTail < mListInfo.ItemInPageCount)
  {
    int iDiff = mListInfo.ItemInPageCount - iTail;
    int iShrinkBy = min(iValue, iDiff); // don't reduce by

    mListInfo.ItemIndex += iShrinkBy;
    mListInfo.ScrollIndex -= iShrinkBy;
  }

  RecalculateIndices(false);
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

  if ((index>=0)&&(index<mListInfo.BottomIndex)) {
    if (index == mListInfo.ItemIndex) {
      if (mOnListEnterCallback) {
        mOnListEnterCallback(this, &mListInfo);
      }

      invalidate();
    } else {
      mListInfo.ItemIndex = index;
      RecalculateIndices(false);
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
