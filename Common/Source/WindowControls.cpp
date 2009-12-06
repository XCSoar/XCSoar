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

#include "WindowControls.h"
#include "Interface.hpp"
#include "Dialogs/dlgTools.h"
#ifndef ALTAIRSYNC
#include "Protection.hpp"
#include "InfoBoxLayout.h"
#include "MainWindow.hpp"
#endif
#include "Math/FastMath.h"
#include "Compatibility/string.h"
#include "PeriodClock.hpp"
#include "Screen/Animation.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Viewport.hpp"
#include "Screen/Bitmap.hpp"
#include "DataField/Base.hpp"
#include "resource.h"
#include "Asset.hpp"

#if !defined(ALTAIRSYNC) && !defined(GNAV) && !defined(WINDOWSPC) && \
  !defined(__GNUC__)
#include <projects.h>
#endif

#include <assert.h>
#include <stdio.h>

#ifndef _MSC_VER
#include <algorithm>
using std::min;
using std::max;
#endif

#ifdef ALTAIRSYNC
#define ISCALE 1
void SetSourceRectangle(RECT fromRect) {};
RECT WINAPI DrawWireRects(LPRECT lprcTo, UINT nMilliSecSpeed) {
  return *lprcTo;
}

#else
#define ISCALE InfoBoxLayout::scale
#endif

#define DEFAULTBORDERPENWIDTH 1*ISCALE
#define SELECTORWIDTH         4*ISCALE

// utility functions

void DrawLine(Canvas &canvas, int x1, int y1, int x2, int y2) {
  canvas.line(x1, y1, x2, y2);
}

// returns true if it is a long press,
// otherwise returns false
static bool KeyTimer(bool isdown, DWORD thekey) {
  static PeriodClock fps_time_down;
  static DWORD savedKey=0;

  if (thekey == savedKey && fps_time_down.check_update(2000)) {
    savedKey = 0;
    return true;
  }

  if (!isdown) {
    // key is released
  } else {
    // key is lowered
    if (thekey != savedKey) {
      fps_time_down.update();
      savedKey = thekey;
    }
  }
  return false;
}


//----------------------------------------------------------
// WindowControl Classes
//----------------------------------------------------------

void InitWindowControlModule(void);

static Color bkColor = clWhite;
static Color fgColor = clBlack;
int WindowControl::InstCount=0;
Brush WindowControl::hBrushDefaultBk;
Pen WindowControl::hPenDefaultBorder;
Pen WindowControl::hPenDefaultSelector;

WindowControl::WindowControl(WindowControl *Owner,
                             ContainerWindow *Parent,
                             const TCHAR *Name,
                             int X, int Y,
                             int Width, int Height,
                             bool Visible)
{
  mHelpText = NULL;

  mHasFocus = false;
  mCanFocus = false;

  mReadOnly = false;

  mClientCount = 0;

  mOnHelpCallback = NULL;

  // todo

  mOwner = Owner;

  // todo
  mhFont = &MapWindowFont;
  mVisible = Visible;
  mCaption[0] = '\0';
  mDontPaintSelector = false;

  if ((Parent == NULL) && (mOwner != NULL))
    Parent = (ContainerWindow *)&mOwner->GetClientAreaWindow();

  if (Name != NULL)
    _tcscpy(mName, Name);  // todo size check
  else
    mName[0] = '\0';

  InitWindowControlModule();

  mColorBack = bkColor;
  mColorFore = fgColor;

  if (InstCount == 0){
    hBrushDefaultBk.set(mColorBack);
    hPenDefaultBorder.set(DEFAULTBORDERPENWIDTH, mColorFore);
    hPenDefaultSelector.set(DEFAULTBORDERPENWIDTH + 2, mColorFore);
  }
  InstCount++;

  set(Parent, X, Y, Width, Height,
      false, false, false, false, false);

  if (mOwner != NULL)
    mOwner->AddClient(this);

  install_wndproc();

  mBorderSize = 1;

  mBorderKind = 0; //BORDERRIGHT | BORDERBOTTOM;

  get_canvas().background_transparent();

  if (mVisible)
    show();
}

WindowControl::~WindowControl(void){
  if (mHelpText) {
    free(mHelpText);
    mHelpText = NULL;
  }

  int i;
  for (i=mClientCount-1; i>=0; i--){
    delete mClients[i];
  }

  reset();

  InstCount--;
  if (InstCount==0){
    hBrushDefaultBk.reset();
    hPenDefaultBorder.reset();
    hPenDefaultSelector.reset();
  }

}

Window *
WindowControl::GetCanFocus()
{
  if (!mVisible)
    return NULL;

  if (mCanFocus && !mReadOnly)
    return this;

  for (int idx=0; idx<mClientCount; idx++){
    Window *w;
    if ((w = mClients[idx]->GetCanFocus()) != NULL){
      return w;
    }
  }
  return NULL;
}

void WindowControl::AddClient(WindowControl *Client){
  mClients[mClientCount] = Client;
  mClientCount++;

  Client->SetOwner(this);
  Client->SetFont(GetFont());

  if (Client->get_position().top == -1 && mClientCount > 1)
    Client->move(Client->get_position().left,
                 mClients[mClientCount - 2]->get_position().bottom);

  /*
  // TODO code: also allow autosizing of height/width to maximum of parent

  if (Client->mHeight == -1){
    // maximum height
    Client->mHeight = mHeight - Client->mY;
    SetWindowPos(Client->GetHandle(), 0,
                 Client->mX, Client->mY,
                 Client->mWidth, Client->mHeight,
                 SWP_NOSIZE | SWP_NOZORDER
                 | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
  }
  */
}

void WindowControl::FilterAdvanced(bool advanced){
  if (_tcsstr(mCaption, TEXT("*")) != NULL) {
    if (advanced) {
      SetVisible(true);
    } else {
      SetVisible(false);
    }
  }
  for (int i=0; i<mClientCount; i++){
    mClients[i]->FilterAdvanced(advanced);
  }
}

WindowControl *WindowControl::FindByName(const TCHAR *Name) {
  if (_tcscmp(mName, Name)==0)
    return this;
  for (int i=0; i<mClientCount; i++){
    WindowControl *W = mClients[i]->FindByName(Name);
    if (W != NULL)
      return W;
  }
  return NULL;
}


WindowControl *WindowControl::SetOwner(WindowControl *Value){
  WindowControl *res = mOwner;
  if (mOwner != Value){
    mOwner = Value;
  }
  return res;
}


void WindowControl::SetHelpText(const TCHAR *Value) {
  if (mHelpText) {
    free(mHelpText);
    mHelpText = NULL;
  }
  if (Value == NULL) {
    return;
  }
  int len = _tcslen(Value);
  if (len==0) {
    return;
  }

  mHelpText= (TCHAR*)malloc((len+1)*sizeof(TCHAR));
  if (mHelpText != NULL) {
    _tcscpy(mHelpText, Value);
  }
}


void WindowControl::SetCaption(const TCHAR *Value){
  if (Value == NULL)
    Value = TEXT("");

  if (_tcscmp(mCaption, Value) != 0){
    _tcscpy(mCaption, Value);
    invalidate();
  }

}

bool WindowControl::SetFocused(bool Value){
  bool res = mHasFocus;

  if (mHasFocus != Value){
    mHasFocus = Value;

    if (mCanFocus)
      // todo, only paint the selector edges
      invalidate();
  }

  return res;

}

bool WindowControl::SetCanFocus(bool Value){
  bool res = mCanFocus;
  mCanFocus = Value;
  return res;
}

bool WindowControl::GetFocused(void){
  return mHasFocus;
}

void
WindowControl::SetVisible(bool Value)
{
  if (mVisible != Value){

    mVisible = Value;

    /*
    for (int i=0; i<mClientCount; i++){
      mClients[i]->SetVisible(mVisible);
    }
    */

    if (mVisible){
      show();
    } else {
      hide();
    }

  }
}

bool WindowControl::GetVisible(void){
  return mVisible;
}

int WindowControl::GetBorderKind(void){
  return mBorderKind;
}

int WindowControl::SetBorderKind(int Value){
  int res = mBorderKind;
  if (mBorderKind != Value){
    mBorderKind = Value;
    invalidate();
  }
  return res;
}

const Font *WindowControl::SetFont(const Font &Value){
  const Font *res = mhFont;
  if (mhFont != &Value){
    // todo
    mhFont = &Value;
  }
  return res;
}

bool WindowControl::SetReadOnly(bool Value){
  bool res = mReadOnly;
  if (mReadOnly != Value){
    mReadOnly = Value;
    invalidate();
  }
  return res;
}

Color WindowControl::SetForeColor(Color Value)
{
  Color res = mColorFore;
  if (mColorFore != Value){
    mColorFore = Value;
    invalidate();
  }
  return res;
}

Color WindowControl::SetBackColor(Color Value)
{
  Color res = mColorBack;
  if (mColorBack != Value){
    mColorBack = Value;
    mhBrushBk.set(mColorBack);
    invalidate();
  }
  return res;
}


void
WindowControl::PaintSelector(Canvas &canvas)
{

  if (!mDontPaintSelector && mCanFocus && mHasFocus){
    const RECT rc = get_client_rect();

    canvas.select(hPenDefaultSelector);

    canvas.two_lines(rc.right - SELECTORWIDTH - 1, rc.top,
                     rc.right - 1, rc.top,
                     rc.right - 1, SELECTORWIDTH + 1);

    canvas.two_lines(rc.right - 1, rc.bottom - SELECTORWIDTH - 2,
                     rc.right - 1, rc.bottom - 1,
                     rc.right - SELECTORWIDTH - 1, rc.bottom - 1);

    canvas.two_lines(SELECTORWIDTH + 1, rc.bottom - 1,
                     rc.left, rc.bottom - 1,
                     rc.left, rc.bottom - SELECTORWIDTH - 2);

    canvas.two_lines(rc.left, SELECTORWIDTH + 1,
                     rc.left, rc.top,
                     SELECTORWIDTH + 1, rc.top);
  }

}

int WindowControl::OnHelp() {
#ifdef ALTAIRSYNC
    return 0; // undefined. return 1 if defined
#else
    if (mHelpText) {
      dlgHelpShowModal(*get_root_owner(), mCaption, mHelpText);
      return 1;
    } else {
      if (mOnHelpCallback) {
        (mOnHelpCallback)(this);
        return 1;
      } else {
        return 0;
      }
    }
#endif
}

bool
WindowControl::on_close(void)
{
  Close();
  return true;
}

bool
WindowControl::on_key_down(unsigned key_code)
{
  // JMW: HELP
  KeyTimer(true, key_code);

  return ContainerWindow::on_key_down(key_code) ||
    on_unhandled_key(key_code);
}

bool
WindowControl::on_key_up(unsigned key_code)
{
  // JMW: detect long enter release
  // VENTA4: PNAs don't have Enter, so it should be better to find an alternate solution
  // activate tool tips if hit return for long time
  if (KeyTimer(false, key_code) && key_code == VK_RETURN && OnHelp())
    return true;

  return ContainerWindow::on_key_up(key_code);
}

void
WindowControl::on_paint(Canvas &canvas)
{
  const RECT rc = get_client_rect();

  // JMW added highlighting, useful for lists
  if (!mDontPaintSelector && mCanFocus && mHasFocus){
    Color ff = GetBackColor().highlight();
    Brush brush(ff);
    canvas.fill_rectangle(rc, brush);

#ifdef WINDOWSPC
  // JMW make it look nice on wine
    canvas.set_background_color(ff);
#endif
  } else
    canvas.fill_rectangle(rc, GetBackBrush());

  if (mBorderKind != 0){
    const RECT rc = get_client_rect();

    canvas.select(GetBorderPen());

    if (mBorderKind & BORDERTOP){
      canvas.line(rc.left, rc.top, rc.right, rc.top);
    }
    if (mBorderKind & BORDERRIGHT){
      canvas.line(rc.right - 1, rc.top, rc.right - 1, rc.bottom);
    }
    if (mBorderKind & BORDERBOTTOM){
      canvas.line(rc.right - 1, rc.bottom - 1, rc.left - 1, rc.bottom - 1);
    }
    if (mBorderKind & BORDERLEFT){
      canvas.line(rc.left, rc.bottom - 1, rc.left, rc.top - 1);
    }
  }

  PaintSelector(canvas);

  ContainerWindow::on_paint(canvas);
}

Window *
WindowControl::FocusNext(WindowControl *Sender)
{
  int idx;
  Window *W;

  if (Sender != NULL){
    for (idx=0; idx<mClientCount; idx++)
      if (mClients[idx] == Sender) break;

    idx++;
  } else idx = 0;

  for (; idx<mClientCount; idx++){
    if ((W = mClients[idx]->GetCanFocus()) != NULL){
      W->set_focus();
      return W;
    }
  }

  if (GetOwner() != NULL){
    return GetOwner()->FocusNext(this);
  }

  return NULL;

}

Window *
WindowControl::FocusPrev(WindowControl *Sender)
{
  int idx;
  Window *W;

  if (Sender != NULL){
    for (idx=0; idx<mClientCount; idx++)
      if (mClients[idx] == Sender) break;

    idx--;
  } else idx = mClientCount-1;

  for (; idx>=0; idx--)
    if ((W=mClients[idx]->GetCanFocus()) != NULL){
      W->set_focus();
      return W;
    }

  if (GetOwner() != NULL){
    return GetOwner()->FocusPrev(this);
  }

  return NULL;
}

bool
WindowControl::on_setfocus()
{
  ContainerWindow::on_setfocus();
  SetFocused(true);
  return true;
}

bool
WindowControl::on_killfocus()
{
  ContainerWindow::on_killfocus();
  SetFocused(false);
  return true;
}

bool
WindowControl::on_unhandled_key(unsigned key_code)
{
  if (mOwner != NULL && mOwner->on_unhandled_key(key_code))
    return true;

  if (mOwner != NULL && mHasFocus) {
    switch (key_code) {
    case VK_UP:
      mOwner->FocusPrev(this);
      return true;

    case VK_DOWN:
      mOwner->FocusNext(this);
      return true;
    }
  }

  return false;
}

void InitWindowControlModule(void){

  static bool InitDone = false;

  if (InitDone)
    return;

  InitDone = true;

}

PeriodClock WndForm::timeAnyOpenClose;

WndForm::WndForm(ContainerWindow *Parent,
                 const TCHAR *Name, const TCHAR *Caption,
                 int X, int Y, int Width, int Height):
  WindowControl(NULL, Parent, Name, X, Y, Width, Height, false) {

  mClientWindow = NULL;
  mOnKeyDownNotify = NULL;
  mOnTimerNotify = NULL;

  mColorTitle = clAqua;

  mhTitleFont = GetFont();

  mClientWindow = new WindowControl(this, this,
                                    TEXT(""), 20, 20, Width, Height);
  mClientWindow->SetBackColor(GetBackColor());

  mClientRect.top=0;
  mClientRect.left=0;
  mClientRect.bottom=Width;
  mClientRect.right=Height;

  cbTimerID = set_timer(1001, 500);

  mModalResult = 0;
  if (Caption != NULL)
    _tcscpy(mCaption, Caption);

}

WndForm::~WndForm(void){
  // animation

  if (mClientWindow)
    mClientWindow->SetVisible(false);

  kill_timer(cbTimerID);
}


ContainerWindow &
WndForm::GetClientAreaWindow(void)
{

  if (mClientWindow != NULL)
    return *mClientWindow;
  else
    return *this;
}


void WndForm::AddClient(WindowControl *Client){      // add client window
  if (mClientWindow != NULL){
    mClientWindow->AddClient(Client); // add it to the clientarea window
  } else
    WindowControl::AddClient(Client);
}

bool
WndForm::on_timer(timer_t id)
{
  if (id == cbTimerID) {
    if (mOnTimerNotify)
      mOnTimerNotify(this);
    return true;
  } else
    return WindowControl::on_timer(id);
}

bool
WndForm::on_user(unsigned id)
{
  if (mOnUserMsgNotify != NULL && mOnUserMsgNotify(this, id))
    return true;

  return WindowControl::on_user(id);
}

const Font *
WndForm::SetTitleFont(const Font &font)
{
  const Font *res = mhTitleFont;

  if (mhTitleFont != &font){
    // todo
    mhTitleFont = &font;



  }

  return res;

}

int WndForm::ShowModal(void){
  return ShowModal(false);
}

#ifndef ENABLE_SDL

static bool
is_user_input(UINT message)
{
  return message == WM_KEYDOWN || message == WM_KEYUP ||
    message == WM_LBUTTONDOWN || message == WM_LBUTTONUP ||
    message == WM_LBUTTONDBLCLK;
}

static bool
is_allowed_map_message(UINT message)
{
  return message == WM_LBUTTONDOWN || message == WM_LBUTTONUP ||
    message == WM_MOUSEMOVE;
}

static bool
is_allowed_map(HWND hWnd, UINT message, bool enable_map)
{
  return !is_altair() && enable_map && MapWindow::identify(hWnd) &&
    is_allowed_map_message(message);
}

#endif /* !ENABLE_SDL */

int WndForm::ShowModal(bool bEnableMap) {
  assert_none_locked();

#define OPENCLOSESUPPRESSTIME 500
#ifndef ENABLE_SDL
  MSG msg;
  HWND oldFocusHwnd;
#endif /* !ENABLE_SDL */

  PeriodClock enter_clock;
  enter_clock.update();

  RECT mRc = get_screen_position();
  DrawWireRects(XCSoarInterface::EnableAnimation,&mRc, 5);

  SetVisible(true);

  bring_to_top();

  mModalResult = 0;

#ifndef ENABLE_SDL
  oldFocusHwnd = ::GetFocus();
#endif /* !ENABLE_SDL */
  set_focus();

  FocusNext(NULL);

#ifndef ENABLE_SDL
  bool hastimed = false;
#endif /* !ENABLE_SDL */
  WndForm::timeAnyOpenClose.update(); // when current dlg opens or child closes

#ifdef ENABLE_SDL

  update();

  SDL_Event event;
  while (SDL_WaitEvent(&event)) {
    if (event.type == SDL_QUIT)
      break;

    if (event.type >= SDL_USEREVENT && event.type <= SDL_NUMEVENTS-1 &&
        event.user.data1 != NULL) {
      Window *window = (Window *)event.user.data1;
      window->on_user(event.type - SDL_USEREVENT);
    } else
      parent->on_event(event);
  }

  return 0;

#else /* !ENABLE_SDL */
  while ((mModalResult == 0) && GetMessage(&msg, NULL, 0, 0)) {
//hack!

    // JMW update display timeout so we don't get blanking
    /*
    if (msg.message == WM_KEYDOWN) {
      if (!Debounce()) {
        continue;
      }
    }
    */

    if (msg.message == WM_KEYDOWN) {
      XCSoarInterface::InterfaceTimeoutReset();
    }

    if ((msg.message == WM_KEYDOWN) && ((msg.wParam & 0xffff) == VK_ESCAPE))
      mModalResult = mrCancel;

    if (is_user_input(msg.message)
        && !identify_descendant(msg.hwnd) // not current window or child
        && !is_allowed_map(msg.hwnd, msg.message, bEnableMap))
      continue;   // make it modal

#ifndef NOKEYDEBONCE
    // hack to stop exiting immediately
    if (!is_altair() && !hastimed && is_user_input(msg.message)) {
      if (!enter_clock.check(1000))
        /* ignore user input in the first 1000ms */
        continue;
      else
        hastimed = true;
    }
#endif

    if (msg.message == WM_KEYDOWN && mOnKeyDownNotify != NULL &&
        mOnKeyDownNotify(this, msg.wParam))
      continue;

    TranslateMessage(&msg);
    if (msg.message != WM_LBUTTONUP ||
        // prevents child click from being repeat-handled by parent
        // if buttons overlap
        WndForm::timeAnyOpenClose.elapsed() > OPENCLOSESUPPRESSTIME) {
      assert_none_locked();

      DispatchMessage(&msg);

      assert_none_locked();
    }
  } // End Modal Loop
#endif /* !ENABLE_SDL */

  // static.  this is current open/close or child open/close
  WndForm::timeAnyOpenClose.update();

  //  SetSourceRectangle(mRc);
  //  DrawWireRects(&aniRect, 5);

  /*
  // reset to center?
  aniRect.top = (mRc.top+mRc.bottom)/2;;
  aniRect.left = (mRc.left+mRc.right)/2;
  aniRect.right = (mRc.left+mRc.right)/2;
  aniRect.bottom = (mRc.top+mRc.bottom)/2;
  SetSourceRectangle(aniRect);
  */

#ifndef ENABLE_SDL
  SetFocus(oldFocusHwnd);
#endif /* !ENABLE_SDL */

  return mModalResult;
}

void
WndForm::on_paint(Canvas &canvas)
{
  WindowControl::on_paint(canvas);

  RECT rcClient = get_client_rect();

  canvas.select(GetBorderPen());
  canvas.select(GetBackBrush());

  canvas.raised_edge(rcClient);

  canvas.set_text_color(GetForeColor());
  canvas.set_background_color(mColorTitle);
  canvas.background_transparent();

  canvas.select(*mhTitleFont);
  SIZE tsize = canvas.text_size(mCaption);

  // JMW todo add here icons?

  CopyRect(&mTitleRect, &rcClient);
  mTitleRect.bottom = mTitleRect.top + tsize.cy;

  rcClient.top += tsize.cy;

  if (mClientWindow && !EqualRect(&mClientRect, &rcClient)){
    mClientWindow->move(rcClient.left, rcClient.top);
    mClientWindow->insert_after(HWND_TOP, true);

    CopyRect(&mClientRect, &rcClient);

  }

  canvas.text_opaque(mTitleRect.left + 1, mTitleRect.top - 2,
                     &mTitleRect, mCaption);
}

void WndForm::SetCaption(const TCHAR *Value){
  if (Value == NULL)
    Value = TEXT("");

  if (_tcscmp(mCaption, Value) != 0){
    _tcscpy(mCaption, Value);
    invalidate(mTitleRect);
  }

}

Color WndForm::SetForeColor(Color Value)
{
  if (mClientWindow)
    mClientWindow->SetForeColor(Value);
  return WindowControl::SetForeColor(Value);
}

Color WndForm::SetBackColor(Color Value)
{
  if (mClientWindow)
  mClientWindow->SetBackColor(Value);
  return WindowControl::SetBackColor(Value);
}

const Font *WndForm::SetFont(const Font &Value){
  if (mClientWindow)
    mClientWindow->SetFont(Value);
  return WindowControl::SetFont(Value);
}

void
WndForm::SetKeyDownNotify(bool (*KeyDownNotify)(WindowControl *Sender,
                                                unsigned key_code))
{
  mOnKeyDownNotify = KeyDownNotify;
}

void
WndForm::SetTimerNotify(int (*OnTimerNotify)(WindowControl *Sender))
{
  mOnTimerNotify = OnTimerNotify;
}

void
WndForm::SetUserMsgNotify(bool (*OnUserMsgNotify)(WindowControl *Sender, unsigned id))
{
  mOnUserMsgNotify = OnUserMsgNotify;
}

// normal form stuff (nonmodal)

#ifndef ENABLE_SDL

bool
WndForm::on_unhandled_key(unsigned key_code)
{
  if (mOnKeyDownNotify != NULL && mOnKeyDownNotify(this, key_code))
    return 0;

  return WindowControl::on_unhandled_key(key_code);
}

#endif /* !ENABLE_SDL */

void WndForm::Show(void){

  WindowControl::Show();

  bring_to_top();

//  SetFocus(GetTopWindow(GetHandle()));

//  SetActiveWindow(GetHandle());

}

//-----------------------------------------------------------
// WndButton
//-----------------------------------------------------------

WndButton::WndButton(WindowControl *Parent,
                     const TCHAR *Name, const TCHAR *Caption,
                     int X, int Y, int Width, int Height,
                     void (*Function)(WindowControl *Sender))
      :WindowControl(Parent, NULL /*Parent->GetHandle()*/, Name, X, Y, Width, Height)
{
  SetCanFocus(true);

  mOnClickNotify = Function;
  mDown = false;
  mDefault = false;

  SetForeColor(GetOwner()->GetForeColor());
  SetBackColor(GetOwner()->GetBackColor());

  _tcscpy(mCaption, Caption);

  mLastDrawTextHeight = -1;

}

bool
WndButton::on_mouse_up(int x, int y)
{
  if (has_capture()) {
    release_capture();

    if (!mDown)
      return true;

    mDown = false;
    invalidate();

    if (mOnClickNotify != NULL) {
      RECT mRc = get_screen_position();
      SetSourceRectangle(mRc);
      (mOnClickNotify)(this);
    }

    return true;
  } else
    return WindowControl::on_mouse_up(x, y);
}


bool
WndButton::on_key_down(unsigned key_code)
{
#ifdef VENTA_DEBUG_EVENT
  TCHAR ventabuffer[80];
  wsprintf(ventabuffer,TEXT("ONKEYDOWN key_code=%d"), key_code); // VENTA-
  DoStatusMessage(ventabuffer);
#endif
  switch (key_code){
#ifdef GNAV
    // JMW added this to make data entry easier
    case VK_F4:
#endif
    case VK_RETURN:
    case VK_SPACE:
      if (!mDown){
        mDown = true;
        invalidate();
      }
      return true;
  }

  return WindowControl::on_key_down(key_code);
}

bool
WndButton::on_key_up(unsigned key_code)
{
  switch (key_code){
#ifdef GNAV
    // JMW added this to make data entry easier
    case VK_F4:
#endif
    case VK_RETURN:
    case VK_SPACE:
      if (!XCSoarInterface::Debounce())
        return 1; // prevent false trigger
      if (mDown){
        mDown = false;
        invalidate();

        if (mOnClickNotify != NULL) {
          RECT mRc = get_screen_position();
          SetSourceRectangle(mRc);
          (mOnClickNotify)(this);
        }
      }
      return true;
  }

  return WindowControl::on_key_up(key_code);
}

bool
WndButton::on_mouse_down(int x, int y)
{
  (void)x; (void)y;
  mDown = true;
  if (!GetFocused())
    set_focus();
  else
    invalidate();

  set_capture();
  return true;
}

bool
WndButton::on_mouse_move(int x, int y, unsigned keys)
{
  if (has_capture()) {
    bool in = in_client_rect(x, y);
    if (in != mDown) {
      mDown = in;
      invalidate();
    }

    return true;
  } else
    return WindowControl::on_mouse_move(x, y, keys);
}

bool
WndButton::on_mouse_double(int x, int y)
{
  (void)x; (void)y;
  mDown = true;
  invalidate();
  set_capture();
  return true;
}


void
WndButton::on_paint(Canvas &canvas)
{
  WindowControl::on_paint(canvas);

  RECT rc = get_client_rect();
  InflateRect(&rc, -2, -2); // todo border width

  // JMW todo: add icons?

  canvas.draw_button(rc, mDown);

  if (mCaption != NULL && mCaption[0] != '\0'){
    canvas.set_text_color(GetForeColor());
    canvas.set_background_color(GetBackColor());
    canvas.background_transparent();

    canvas.select(*GetFont());

    rc = get_client_rect();
    InflateRect(&rc, -2, -2); // todo border width

    if (mDown)
      OffsetRect(&rc, 2, 2);

    if (mLastDrawTextHeight < 0){
      canvas.formatted_text(&rc, mCaption,
          DT_CALCRECT
        | DT_EXPANDTABS
        | DT_CENTER
        | DT_NOCLIP
        | DT_WORDBREAK // mCaptionStyle // | DT_CALCRECT
      );

      mLastDrawTextHeight = rc.bottom - rc.top;
      // DoTo optimize
      rc = get_client_rect();
      InflateRect(&rc, -2, -2); // todo border width
      if (mDown)
        OffsetRect(&rc, 2, 2);

    }

    rc.top += (canvas.get_height() - 4 - mLastDrawTextHeight) / 2;

    canvas.formatted_text(&rc, mCaption,
        DT_EXPANDTABS
      | DT_CENTER
      | DT_NOCLIP
      | DT_WORDBREAK // mCaptionStyle // | DT_CALCRECT
    );

//    mLastDrawTextHeight = rc.bottom - rc.top;

  }

//  UINT lastAlign = SetTextAlign(hDC, TA_CENTER /*| VTA_CENTER*/);
//  ExtTextOut(hDC, GetWidth()/2, GetHeight()/2,
//    /*ETO_OPAQUE | */ETO_CLIPPED, &r, mCaption, _tcslen(mCaption), NULL);
//  if (lastAlign != GDI_ERROR){
//    SetTextAlign(hDC, lastAlign);
//  }


// 20060518:sgi old version
//  ExtTextOut(hDC, org.x, org.y,
//    /*ETO_OPAQUE | */ETO_CLIPPED, &r, mCaption, _tcslen(mCaption), NULL);
}


bool
WndProperty::Editor::on_mouse_down(int x, int y)
{
  // if it's an Combopicker field, then call the combopicker routine
  if (parent->mDialogStyle) {
    if (parent->on_mouse_down(x, y)) {
      return true;
    }
  } //end combopicker

#ifndef ENABLE_SDL
  if (parent->GetReadOnly())
    /* drop this event, so the default handler doesn't obtain the
       keyboard focus */
    return true;
#endif /* !ENABLE_SDL */

  return false;
}

bool
WndProperty::Editor::on_key_down(unsigned key_code)
{
  if (key_code == VK_RETURN || key_code == VK_F23) { // Compaq uses VKF23
    if (parent->mDialogStyle) {
      if (parent->on_mouse_down(0, 0)) {
        return true;
      }
    } //end combopicker
  }
  // tmep hack, do not process nav keys
  if (KeyTimer(true, key_code)) {
    // activate tool tips if hit return for long time
    if (key_code == VK_RETURN || key_code == VK_F23) { // Compaq uses VKF23
      if (parent->OnHelp())
        return true;
    }
  }

  return EditWindow::on_key_down(key_code) ||
    parent->on_unhandled_key(key_code);
}

bool
WndProperty::Editor::on_key_up(unsigned key_code)
{
  if (KeyTimer(false, key_code)) {
    // activate tool tips if hit return for long time
    if (key_code == VK_RETURN || key_code == VK_F23) { // Compaq uses VKF23
      if (parent->OnHelp())
        return true;
    }
  } else if (key_code == VK_RETURN) {
    if (parent->CallSpecial())
      return true;
  }

  return false;
}

bool
WndProperty::Editor::on_setfocus()
{
  KeyTimer(true, 0);
  EditWindow::on_setfocus();
  parent->on_editor_setfocus();
  set_selection();
  return true;
}

bool
WndProperty::Editor::on_killfocus()
{
  KeyTimer(true, 0);
  parent->on_editor_killfocus();
  EditWindow::on_killfocus();
  return true;
}

Bitmap WndProperty::hBmpLeft32;
Bitmap WndProperty::hBmpRight32;

int WndProperty::InstCount = 0;

WndProperty::WndProperty(WindowControl *Parent,
                         TCHAR *Name,
                         TCHAR *Caption,
                         int X, int Y,
                         int Width, int Height,
                         int CaptionWidth,
                         int (*DataChangeNotify)(WindowControl *Sender,
                                                 int Mode, int Value),
                         int MultiLine)
  :WindowControl(Parent,
                 NULL /*Parent->GetHandle()*/,
                 Name, X, Y, Width, Height),
  edit(this)
{
  SetCanFocus(true);

  mOnClickUpNotify = NULL;
  mOnClickDownNotify = NULL;
  mOnDataChangeNotify = DataChangeNotify;
  _tcscpy(mCaption, Caption);
  mDataField = NULL;
  mDialogStyle=false; // this is set by ::SetDataField()

  mhValueFont = GetFont();
  mCaptionWidth = CaptionWidth;

  if (mCaptionWidth != 0){
    mBitmapSize = DLGSCALE(32)/2;
  } else {
    mBitmapSize = DLGSCALE(32)/2;
  }
  if (mDialogStyle)
    mBitmapSize = 0;

  UpdateButtonData(mBitmapSize);

  edit.set(*this, mEditPos.x, mEditPos.y, mEditSize.x, mEditSize.y,
           MultiLine);
  edit.install_wndproc();

  edit.set_font(*mhValueFont);

  mCanFocus = true;

  SetForeColor(GetOwner()->GetForeColor());
  SetBackColor(GetOwner()->GetBackColor());

  if (InstCount == 0){
    hBmpLeft32.load(IDB_DLGBUTTONLEFT32);
    hBmpRight32.load(IDB_DLGBUTTONRIGHT32);
  }
  InstCount++;

  mDownDown = false;
  mUpDown = false;

}


WndProperty::~WndProperty(void){
  InstCount--;
  if (InstCount == 0){
    hBmpLeft32.reset();
    hBmpRight32.reset();
  }

  if (mDataField != NULL){
    if (!mDataField->Unuse()) {
      delete mDataField;
      mDataField = NULL;
    } else {
      assert(0);
    }
  }
}

Window *
WndProperty::GetCanFocus()
{
  Window *w = WindowControl::GetCanFocus();
  if (w == this)
    return &edit;

  return w;
}

void WndProperty::SetText(const TCHAR *Value){
  edit.set_text(Value);
}

const Font *WndProperty::SetFont(const Font &Value){
  const Font *res = GetFont();

  WindowControl::SetFont(Value);

  // todo, support value font

  if (res != &Value){
    mhValueFont = &Value;
    edit.set_font(Value);
  }
  return res;
}

void WndProperty::UpdateButtonData(int Value){

  if (Value == 0) // if combo is enabled
    mBitmapSize = 0;
  else
    mBitmapSize = DLGSCALE(32)/2;

  const SIZE size = get_size();

  if (mCaptionWidth != 0){
    mEditSize.x = size.cx - mCaptionWidth - (DEFAULTBORDERPENWIDTH + 1) - mBitmapSize;
    mEditSize.y = size.cy - 2 * (DEFAULTBORDERPENWIDTH + 1);
    mEditPos.x = mCaptionWidth;
    mEditPos.y = (DEFAULTBORDERPENWIDTH+1);
  } else {
    mEditSize.x = size.cx - 2 * (DEFAULTBORDERPENWIDTH + 1 + mBitmapSize);
    mEditSize.y = size.cy / 2;
    mEditPos.x = mBitmapSize + (DEFAULTBORDERPENWIDTH+2);
    mEditPos.y = size.cy / 2 - 2 * (DEFAULTBORDERPENWIDTH + 1);
  }

  mHitRectDown.left = mEditPos.x-mBitmapSize;
  mHitRectDown.top = mEditPos.y + (mEditSize.y)/2 - (mBitmapSize/2);
  mHitRectDown.right = mHitRectDown.left + mBitmapSize;
  mHitRectDown.bottom = mHitRectDown.top + mBitmapSize;

  mHitRectUp.left = size.cx - (mBitmapSize + 2);
  mHitRectUp.top = mHitRectDown.top;
  mHitRectUp.right = mHitRectUp.left + mBitmapSize;
  mHitRectUp.bottom = mHitRectUp.top + mBitmapSize;

}

int WndProperty::SetButtonSize(int Value){
  int res = mBitmapSize;

  if (mBitmapSize != Value){

    UpdateButtonData(Value);

    edit.move(mEditPos.x, mEditPos.y, mEditSize.x, mEditSize.y);

    invalidate();
  }
  return res;
}

bool WndProperty::SetReadOnly(bool Value){

  bool res = GetReadOnly();

  if (GetReadOnly() != Value){
    WindowControl::SetReadOnly(Value);

    edit.set_read_only(Value);
  }

  return res;
}

void
WndProperty::on_editor_setfocus()
{
  if (mDataField != NULL) {
    mDataField->GetData();
    edit.set_text(mDataField->GetAsString());
  }

  if (!GetFocused())
    SetFocused(true);
}

void
WndProperty::on_editor_killfocus()
{
  if (mDataField != NULL) {
    TCHAR sTmp[128];
    edit.get_text(sTmp, (sizeof(sTmp)/sizeof(TCHAR))-1);
    mDataField->SetAsString(sTmp);
    mDataField->SetData();
    edit.set_text(mDataField->GetAsDisplayString());
  }

  if (GetFocused())
    SetFocused(false);
}

bool
WndProperty::on_unhandled_key(unsigned key_code)
{
  switch (key_code){
    case VK_RIGHT:
      IncValue();
      return true;
    case VK_LEFT:
      DecValue();
      return true;
  }

  return WindowControl::on_unhandled_key(key_code);
}

bool
WndProperty::on_mouse_down(int x, int y)
{
  POINT Pos;

  if (mDialogStyle)
  {
    if (!GetReadOnly())  // when they click on the label
    {
      dlgComboPicker(*get_root_owner(), this);
    }
    else
    {
      OnHelp(); // this would display xml file help on a read-only wndproperty if it exists
    }
  }
  else
  {

    if (!GetFocused()){
      if (!GetReadOnly())
        edit.set_focus();
      return true;
    }

    Pos.x = x;
    Pos.y = y;
    //POINTSTOPOINT(Pos, MAKEPOINTS(lParam));

    mDownDown = (PtInRect(&mHitRectDown, Pos) != 0);

    if (mDownDown) {
      DecValue();
      invalidate(mHitRectDown);
    }

    mUpDown = (PtInRect(&mHitRectUp, Pos) != 0);

    if (mUpDown) {
      IncValue();
      invalidate(mHitRectUp);
    }
    set_capture();
  }

  return true;
}

bool
WndProperty::on_mouse_double(int x, int y)
{
  return on_mouse_down(x, y);
}

bool
WndProperty::on_mouse_up(int x, int y)
{
  if (mDialogStyle)
  {
  }
  else
  {

    if (mDownDown){
      mDownDown = false;
      invalidate(mHitRectDown);
    }
    if (mUpDown){
      mUpDown = false;
      invalidate(mHitRectUp);
    }

  }
  release_capture();
  return true;
}


int WndProperty::CallSpecial(void){
  if (mDataField != NULL){
    mDataField->Special();
    edit.set_text(mDataField->GetAsString());
  }
  return 0;
}

int WndProperty::IncValue(void){
  if (mDataField != NULL){
    mDataField->Inc();
    edit.set_text(mDataField->GetAsString());
  }
  return 0;
}

int WndProperty::DecValue(void){
  if (mDataField != NULL){
    mDataField->Dec();
    edit.set_text(mDataField->GetAsString());
  }
  return 0;
}


void
WndProperty::on_paint(Canvas &canvas)
{
  SIZE tsize;
  POINT org;

  WindowControl::on_paint(canvas);

  canvas.set_text_color(GetForeColor());

#ifdef WINDOWSPC
  // JMW make it look nice on wine
  if (!GetFocused()) {
    canvas.set_background_color(GetBackColor());
  }
#endif

  canvas.background_transparent();
  canvas.select(*GetFont());

  tsize = canvas.text_size(mCaption);

  if (mCaptionWidth==0){
    org.x = mEditPos.x;
    org.y = mEditPos.y - tsize.cy;
  } else {
    org.x = mCaptionWidth - mBitmapSize - (tsize.cx + 1);
    org.y = (get_size().cy - tsize.cy) / 2;
  }

  if (org.x < 1)
    org.x = 1;

  // JMW TODO: use stretch functions for bigger displays, since these icons are too small for them.

  canvas.text_opaque(org.x, org.y, mCaption);

  // can't but dlgComboPicker here b/c it calls paint when combopicker closes too
  // so it calls dlgCombopicker on the click/focus handlers for the wndproperty & label
  if (!mDialogStyle && GetFocused() && !GetReadOnly()) {
    BitmapCanvas bitmap_canvas(canvas);

    bitmap_canvas.select(hBmpLeft32);
    canvas.stretch(mHitRectDown.left, mHitRectDown.top,
                   mBitmapSize, mBitmapSize,
                   bitmap_canvas,
                   mDownDown ? 32 : 0, 0, 32, 32);

    bitmap_canvas.select(hBmpRight32);
    canvas.stretch(mHitRectUp.left, mHitRectUp.top,
                   mBitmapSize, mBitmapSize,
                   bitmap_canvas,
                   mUpDown ? 32 : 0, 0, 32, 32);
  }
}


void WndProperty::RefreshDisplay() {
  if (!mDataField) return;
  if (GetFocused())
    edit.set_text(mDataField->GetAsString());
  else
    edit.set_text(mDataField->GetAsDisplayString());
}


DataField *WndProperty::SetDataField(DataField *Value){
  DataField *res = mDataField;

  if (mDataField != Value){

    if (mDataField!=NULL){

      if (!mDataField->Unuse()){

        delete(mDataField);

        res = NULL;

      }

    }

    Value->Use();

    mDataField = Value;

    mDataField->GetData();

    mDialogStyle = has_pointer();

    if (mDataField->SupportCombo == false )
      mDialogStyle=false;


    if (mDialogStyle)
    {
      this->SetButtonSize(0);
    }
    else
    {
      this->SetButtonSize(16);
    }

    RefreshDisplay();

  }

  return res;

}


void
WndOwnerDrawFrame::on_paint(Canvas &canvas)
{
  WndFrame::on_paint(canvas);

  canvas.select(*GetFont());

  if (mOnPaintCallback != NULL)
    (mOnPaintCallback)(this, canvas);
}

void
WndFrame::on_paint(Canvas &canvas)
{
  WindowControl::on_paint(canvas);

  if (mCaption != 0){
    canvas.set_text_color(GetForeColor());
    canvas.set_background_color(GetBackColor());
    canvas.background_transparent();

    canvas.select(*GetFont());

    RECT rc = get_client_rect();
    InflateRect(&rc, -2, -2); // todo border width

//    h = rc.bottom - rc.top;

    canvas.formatted_text(&rc, mCaption,
      mCaptionStyle // | DT_CALCRECT
    );
  }
}

void WndFrame::SetCaption(const TCHAR *Value){
  if (Value == NULL)
    Value = TEXT("");

  if (_tcscmp(mCaption, Value) != 0){
    _tcscpy(mCaption, Value);  // todo size check
    invalidate();
  }
}

UINT WndFrame::SetCaptionStyle(UINT Value){
  UINT res = mCaptionStyle;
  if (res != Value){
    mCaptionStyle = Value;
    invalidate();
  }
  return res;
}

unsigned
WndFrame::GetTextHeight()
{
  RECT rc = get_client_rect();
  ::InflateRect(&rc, -2, -2); // todo border width

  Canvas &canvas = get_canvas();
  canvas.select(*GetFont());
  canvas.formatted_text(&rc, mCaption, mCaptionStyle | DT_CALCRECT);

  return rc.bottom - rc.top;
}

WndListFrame::ScrollBar::ScrollBar()
  :dragging(false)
{
  SetRectEmpty(&rc);
  SetRectEmpty(&button);
}

void
WndListFrame::ScrollBar::set(const SIZE size)
{
  unsigned width;

  if (has_pointer()) {
    // shrink width factor.  Range .1 to 1 where 1 is very "fat"
    double SHRINKSBFACTOR = is_pna() ? 1.0 : 0.75;

    width = (unsigned) (SCROLLBARWIDTH_INITIAL * InfoBoxLayout::dscale * SHRINKSBFACTOR);

    // resize height for each dialog so top button is below 1st item (to avoid initial highlighted overlap)
  } else {
    // thin for ALTAIR b/c no touch screen
    width = SELECTORWIDTH * 2;
  }

  rc.left = size.cx - width;
  rc.top = 0;
  rc.right = size.cx;
  rc.bottom = size.cy;

  if (!hScrollBarBitmapTop.defined())
    hScrollBarBitmapTop.load(IDB_SCROLLBARTOP);
  if (!hScrollBarBitmapMid.defined())
    hScrollBarBitmapMid.load(IDB_SCROLLBARMID);
  if (!hScrollBarBitmapBot.defined())
    hScrollBarBitmapBot.load(IDB_SCROLLBARBOT);
  if (!hScrollBarBitmapFill.defined())
    hScrollBarBitmapFill.load(IDB_SCROLLBARFILL);
}

void
WndListFrame::ScrollBar::reset()
{
  SetRectEmpty(&rc);
  SetRectEmpty(&button);
}

void
WndListFrame::ScrollBar::set_button(unsigned size, unsigned view_size,
                                    unsigned origin)
{
  const int netto_height = get_netto_height();

  int height = size > 0 ? netto_height * view_size / size : netto_height;
  if (height < get_width())
    height = get_width();

  int max_origin = size - view_size;
  int top = max_origin > 0
    ? (netto_height - height) * origin / max_origin
    : 0;

  if (top + height > netto_height)
    height = netto_height - top;

  button.left = rc.left;
  button.top = rc.top + get_width() + top - 1;
  button.right = rc.right - 1; // -2 if use 3x pen.  -1 if 2x pen
  button.bottom = button.top + height + 2; // +2 for 3x pen, +1 for 2x pen
}

unsigned
WndListFrame::ScrollBar::to_origin(unsigned size, unsigned view_size,
                                   int y) const
{
  int max_origin = size - view_size;
  if (max_origin <= 0)
    return 0;

  y -= rc.top + get_width();
  if (y < 0)
    return 0;

  unsigned origin = y * max_origin / get_scroll_height();
  return min(origin, (unsigned)max_origin);
}

void
WndListFrame::ScrollBar::paint(Canvas &canvas, Color fore_color) const
{
  Brush brush(Color(0xff, 0xff, 0xff));
  Pen pen(DEFAULTBORDERPENWIDTH, fore_color);
  canvas.select(pen);

  // draw rectangle around entire scrollbar area
  canvas.two_lines(rc.left, rc.top, rc.left, rc.bottom,
                   rc.right, rc.bottom);
  canvas.two_lines(rc.right, rc.bottom, rc.right, rc.top,
                   rc.left, rc.top);

  // Just Scroll Bar Slider button

  bool bTransparentUpDown = true;

  BitmapCanvas bitmap_canvas(canvas);

  // TOP Dn Button 32x32
  // BOT Up Button 32x32
  if (get_width() == SCROLLBARWIDTH_INITIAL) {
    bitmap_canvas.select(hScrollBarBitmapTop);
    canvas.copy(rc.left, rc.top,
                SCROLLBARWIDTH_INITIAL, SCROLLBARWIDTH_INITIAL,
                bitmap_canvas, 0, 0,
                bTransparentUpDown);

    bitmap_canvas.select(hScrollBarBitmapBot);
    canvas.copy(rc.left, rc.bottom - SCROLLBARWIDTH_INITIAL,
                SCROLLBARWIDTH_INITIAL, SCROLLBARWIDTH_INITIAL,
                bitmap_canvas, 0, 0,
                bTransparentUpDown);
  } else {
    bitmap_canvas.select(hScrollBarBitmapTop);
    canvas.stretch(rc.left, rc.top, get_width(), get_width(),
                   bitmap_canvas,
                   0, 0, SCROLLBARWIDTH_INITIAL, SCROLLBARWIDTH_INITIAL,
                   bTransparentUpDown);

    // BOT Up Button 32x32
    bitmap_canvas.select(hScrollBarBitmapBot);
    canvas.stretch(rc.left, rc.bottom - get_width(), get_width(), get_width(),
                   bitmap_canvas,
                   0, 0, SCROLLBARWIDTH_INITIAL, SCROLLBARWIDTH_INITIAL,
                   bTransparentUpDown);
  }

  // Middle Slider Button 30x28

  // handle on slider
  bitmap_canvas.select(hScrollBarBitmapMid);
  // always SRCAND b/c on top of scrollbutton texture
  canvas.stretch_and(button.left + 1, button.top + 1,
                     button.right - button.left - 2,
                     button.bottom - button.top - 2,
                     bitmap_canvas, 0, 0, 30, 28);

  // box around slider rect
  Pen pen3(DEFAULTBORDERPENWIDTH * 2, fore_color);
  int iBorderOffset = 1;  // set to 1 if BORDERWIDTH >2, else 0
  canvas.select(pen3);
  canvas.two_lines(button.left + iBorderOffset, button.top,
                   button.left + iBorderOffset, button.bottom,
                   button.right, button.bottom); // just left line of scrollbar
  canvas.two_lines(button.right, button.bottom,
                   button.right, button.top,
                   button.left + iBorderOffset, button.top); // just left line of scrollbar
}

void
WndListFrame::ScrollBar::drag_begin(Window *w, unsigned y)
{
  assert(!dragging);

  drag_offset = y - button.top;
  dragging = true;
  w->set_capture();
}

void
WndListFrame::ScrollBar::drag_end(Window *w)
{
  if (!dragging)
    return;

  dragging = false;
  w->release_capture();
}

unsigned
WndListFrame::ScrollBar::drag_move(unsigned size, unsigned view_size,
                                   int y) const
{
  assert(dragging);

  return to_origin(size, view_size, y - drag_offset);
}

WndListFrame::WndListFrame(WindowControl *Owner, const TCHAR *Name,
                           int X, int Y, int Width, int Height,
                           void (*OnListCallback)(WindowControl *Sender,
                                                  ListInfo_t *ListInfo)):
  WndFrame(Owner, Name, X, Y, Width, Height)
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

  if (mClientCount > 0){
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

#ifndef ALTAIRSYNC
#include "InputEvents.h"

void WndEventButton_OnClickNotify(WindowControl *Sender) {
  WndEventButton *wb = (WndEventButton*)Sender;
  wb->CallEvent();
}

void WndEventButton::CallEvent() {
  if (inputEvent) {
    inputEvent(parameters);
  }
}

WndEventButton::~WndEventButton() {
  if (parameters) {
    free(parameters);
    parameters=NULL;
  }
}


WndEventButton::WndEventButton(WindowControl *Parent, const TCHAR *Name,
			       const TCHAR *Caption,
			       int X, int Y, int Width, int Height,
			       const TCHAR* ename,
			       const TCHAR* theparameters):
  WndButton(Parent,Name,Caption,X,Y,Width,Height,
	    WndEventButton_OnClickNotify)
{
  inputEvent = InputEvents::findEvent(ename);
  if (theparameters) {
    parameters = _tcsdup(theparameters);
  } else {
    parameters = NULL;
  }

}


//
//
#endif
