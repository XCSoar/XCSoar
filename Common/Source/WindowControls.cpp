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

WindowControl *ActiveControl = NULL;
WindowControl *LastFocusControl = NULL;

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
			     bool Visible){

  mHelpText = NULL;

  mHasFocus = false;
  mCanFocus = false;

  mReadOnly = false;

  mClientCount = 0;

  mOnHelpCallback = NULL;

  // todo

  mX = X;
  mY = Y;
  mWidth = Width;
  mHeight = Height;
  mOwner = Owner;
  // setup Master Window (the owner of all)
  mTopOwner = Owner;
  while (Owner != NULL && mTopOwner->GetOwner() != NULL)
    mTopOwner = mTopOwner->GetOwner();

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

  set(Parent, mX, mY, mWidth, mHeight,
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
}

void WindowControl::Destroy(void){
  int i;
  for (i=mClientCount-1; i>=0; i--){
    mClients[i]->Destroy();
    delete mClients[i];
  }

  if (LastFocusControl == this)
    LastFocusControl = NULL;

  if (ActiveControl == this)
    ActiveControl = NULL;

  mhBrushBk.reset();
  mhPenBorder.reset();
  mhPenSelector.reset();

  // ShowWindow(GetHandle(), SW_SHOW);
  reset();

  InstCount--;
  if (InstCount==0){
    hBrushDefaultBk.reset();
    hPenDefaultBorder.reset();
    hPenDefaultSelector.reset();
  }

}

void WindowControl::UpdatePosSize(void){
  move(mX, mY, mWidth, mHeight);
}

void WindowControl::SetTop(int Value){
  if (mY != Value){
    mY = Value;
    UpdatePosSize();
  }
}

void WindowControl::SetLeft(int Value){
  if (mX != Value){
    mX = Value;
    UpdatePosSize();
  }
}

void WindowControl::SetHeight(int Value){
  if (mHeight != Value){
    mHeight = Value;
    UpdatePosSize();
  }
}

void WindowControl::SetWidth(int Value){
  if (mWidth != Value){
    mWidth = Value;
    UpdatePosSize();
  }
}

WindowControl *WindowControl::GetCanFocus(void){
  if (mVisible && mCanFocus && !mReadOnly)
    return(this);

  if (!mVisible)
    return(NULL);

  for (int idx=0; idx<mClientCount; idx++){
    WindowControl *w;
    if ((w = mClients[idx]->GetCanFocus()) != NULL){
      return(w);
    }
  }
  return(NULL);
}

void WindowControl::AddClient(WindowControl *Client){
  mClients[mClientCount] = Client;
  mClientCount++;

  Client->SetOwner(this);
  Client->SetFont(GetFont());

  if (Client->mY == -1){
    if (mClientCount > 1){
      Client->mY =
	mClients[mClientCount-2]->mY
	+ mClients[mClientCount-2]->mHeight;
      Client->move(Client->mX, Client->mY);
    }
  }

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
    return(this);
  for (int i=0; i<mClientCount; i++){
    WindowControl *W = mClients[i]->FindByName(Name);
    if (W != NULL)
      return(W);
  }
  return(NULL);
}


WindowControl *WindowControl::SetOwner(WindowControl *Value){
  WindowControl *res = mOwner;
  if (mOwner != Value){
    mOwner = Value;
  }
  return(res);
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

  if (Value){
    if (mCanFocus){
      ActiveControl = this;
      LastFocusControl = this;
    }
  } else {
    ActiveControl = NULL;
    /*
    if (FromTo == NULL){
      SetFocus(GetParent());
    }
    */
  }

  return(res);

}

bool WindowControl::SetCanFocus(bool Value){
  bool res = mCanFocus;
  mCanFocus = Value;
  return(res);
}

bool WindowControl::GetFocused(void){
  return(mHasFocus);
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
  return(mVisible);
}

int WindowControl::GetBorderKind(void){
  return(mBorderKind);
}

int WindowControl::SetBorderKind(int Value){
  int res = mBorderKind;
  if (mBorderKind != Value){
    mBorderKind = Value;
    invalidate();
  }
  return(res);
}

const Font *WindowControl::SetFont(const Font &Value){
  const Font *res = mhFont;
  if (mhFont != &Value){
    // todo
    mhFont = &Value;
  }
  return(res);
}

bool WindowControl::SetReadOnly(bool Value){
  bool res = mReadOnly;
  if (mReadOnly != Value){
    mReadOnly = Value;
    on_paint(GetCanvas());
  }
  return(res);
}

Color WindowControl::SetForeColor(Color Value)
{
  Color res = mColorFore;
  if (mColorFore != Value){
    mColorFore = Value;
    if (mVisible)
      on_paint(GetCanvas());
  }
  return(res);
}

Color WindowControl::SetBackColor(Color Value)
{
  Color res = mColorBack;
  if (mColorBack != Value){
    mColorBack = Value;
    mhBrushBk.set(mColorBack);
    if (mVisible)
      on_paint(GetCanvas());
  }
  return(res);
}


void
WindowControl::PaintSelector(Canvas &canvas)
{

  if (!mDontPaintSelector && mCanFocus && mHasFocus){
    canvas.select(hPenDefaultSelector);

    canvas.two_lines(mWidth - SELECTORWIDTH - 1, 0,
                     mWidth - 1, 0,
                     mWidth - 1, SELECTORWIDTH + 1);

    canvas.two_lines(mWidth - 1, mHeight - SELECTORWIDTH - 2,
                     mWidth - 1, mHeight - 1,
                     mWidth - SELECTORWIDTH - 1, mHeight - 1);

    canvas.two_lines(SELECTORWIDTH + 1, mHeight - 1,
                     0, mHeight - 1,
                     0, mHeight - SELECTORWIDTH - 2);

    canvas.two_lines(0, SELECTORWIDTH + 1,
                     0, 0,
                     SELECTORWIDTH + 1, 0);
  }

}

void WindowControl::Redraw(void){
  invalidate();
}

int WindowControl::OnHelp() {
#ifdef ALTAIRSYNC
    return(0); // undefined. return 1 if defined
#else
    if (mHelpText) {
      dlgHelpShowModal(*get_root_owner(), mCaption, mHelpText);
      return(1);
    } else {
      if (mOnHelpCallback) {
	(mOnHelpCallback)(this);
	return(1);
      } else {
	return(0);
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

  return ContainerWindow::on_key_down(key_code);
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
  RECT rc;

  rc.left = 0;
  rc.top = 0;
  rc.right = 0 + mWidth+2;
  rc.bottom = 0 + mHeight+2;

  canvas.fill_rectangle(rc, GetBackBrush());

  // JMW added highlighting, useful for lists
  if (!mDontPaintSelector && mCanFocus && mHasFocus){
    Color ff = GetBackColor().highlight();
    Brush brush(ff);
    rc.left += 0;
    rc.right -= 2;
    rc.top += 0;
    rc.bottom -= 2;
    canvas.fill_rectangle(rc, brush);

#ifdef WINDOWSPC
  // JMW make it look nice on wine
    canvas.set_background_color(ff);
#endif
  }

  if (mBorderKind != 0){
    canvas.select(GetBorderPen());

    if (mBorderKind & BORDERTOP){
      canvas.line(0, 0, mWidth, 0);
    }
    if (mBorderKind & BORDERRIGHT){
      canvas.line(mWidth - 1, 0, mWidth - 1, mHeight);
    }
    if (mBorderKind & BORDERBOTTOM){
      canvas.line(mWidth - 1, mHeight - 1, -1, mHeight - 1);
    }
    if (mBorderKind & BORDERLEFT){
      canvas.line(0, mHeight - 1, 0, -1);
    }
  }

  PaintSelector(canvas);

  ContainerWindow::on_paint(canvas);
}

WindowControl *WindowControl::FocusNext(WindowControl *Sender){
  int idx;
  WindowControl *W;

  if (Sender != NULL){
    for (idx=0; idx<mClientCount; idx++)
      if (mClients[idx] == Sender) break;

    idx++;
  } else idx = 0;

  for (; idx<mClientCount; idx++){
    if ((W = mClients[idx]->GetCanFocus()) != NULL){
      W->set_focus();
      return(W);
    }
  }

  if (GetOwner() != NULL){
    return(GetOwner()->FocusNext(this));
  }

  return(NULL);

}

WindowControl *WindowControl::FocusPrev(WindowControl *Sender){
  int idx;
  WindowControl *W;

  if (Sender != NULL){
    for (idx=0; idx<mClientCount; idx++)
      if (mClients[idx] == Sender) break;

    idx--;
  } else idx = mClientCount-1;

  for (; idx>=0; idx--)
    if ((W=mClients[idx]->GetCanFocus()) != NULL){
      W->set_focus();
      return(W);
    }

  if (GetOwner() != NULL){
    return(GetOwner()->FocusPrev(this));
  }

  return(NULL);
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

#ifndef ENABLE_SDL

LRESULT
WindowControl::on_message(HWND hwnd, UINT uMsg,
                          WPARAM wParam, LPARAM lParam)
{
  switch (uMsg){
  case WM_ERASEBKGND:
  case WM_PAINT:
  case WM_WINDOWPOSCHANGED:
  case WM_CREATE:
  case WM_DESTROY:
  case WM_ACTIVATE:
  case WM_CLOSE:
    /* exclude these from OnUnhandledMessage() */
    return ContainerWindow::on_message(hwnd, uMsg, wParam, lParam);
  }

  if (mTopOwner != NULL){
    if (!mTopOwner->OnUnhandledMessage(hwnd, uMsg, wParam, lParam))
     return(0);
  } else {
    if (OnUnhandledMessage(hwnd, uMsg, wParam, lParam))
     return(0);
  }

  return ContainerWindow::on_message(hwnd, uMsg, wParam, lParam);
}

#endif /* !ENABLE_SDL */

void InitWindowControlModule(void){

  static bool InitDone = false;

  if (InitDone)
    return;

  ActiveControl = NULL;

  InitDone = true;

}

PeriodClock WndForm::timeAnyOpenClose;

#ifndef ENABLE_SDL
ACCEL  WndForm::mAccel[] = {
  {0, VK_ESCAPE,  VK_ESCAPE},
  {0, VK_RETURN,  VK_RETURN},
};
#endif /* !ENABLE_SDL */

WndForm::WndForm(ContainerWindow *Parent,
                 const TCHAR *Name, const TCHAR *Caption,
                 int X, int Y, int Width, int Height):
  WindowControl(NULL, Parent, Name, X, Y, Width, Height, false) {

  mClientWindow = NULL;
  mOnKeyDownNotify = NULL;
  mOnKeyUpNotify = NULL;
  mOnLButtonUpNotify = NULL;
  mOnTimerNotify = NULL;
  bLButtonDown= false;

#ifndef ENABLE_SDL
  mhAccelTable = CreateAcceleratorTable(mAccel, sizeof(mAccel)/sizeof(mAccel[0]));
#endif /* !ENABLE_SDL */

  mColorTitle = clAqua;

  mhTitleFont = GetFont();

  mClientWindow = new WindowControl(this, this,
                                    TEXT(""), 20, 20, Width, Height);
  mClientWindow->SetBackColor(GetBackColor());
  mClientWindow->SetCanFocus(false);

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
  Destroy();
}



void WndForm::Destroy(void){

  // animation

  if (mClientWindow)
    mClientWindow->SetVisible(false);

  kill_timer(cbTimerID);

#ifdef WIN32
  DestroyAcceleratorTable(mhAccelTable);
#endif /* WIN32 */

  WindowControl::Destroy();  // delete all childs

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
WndForm::on_command(unsigned id, unsigned code)
{
   // VENTA- DEBUG HARDWARE KEY PRESSED
#ifdef VENTA_DEBUG_KEY
	TCHAR ventabuffer[80];
        wsprintf(ventabuffer, TEXT("ONCKEY id=%d code=%d"), id, code);
	DoStatusMessage(ventabuffer);
#endif
   if (id == VK_ESCAPE){
     mModalResult = mrCancel;
     return true;
   }

   return WindowControl::on_command(id, code);
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

  return(res);

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
#define OPENCLOSESUPPRESSTIME 500
#ifndef ENABLE_SDL
  MSG msg;
  HWND oldFocusHwnd;
#endif /* !ENABLE_SDL */

  PeriodClock enter_clock;
  enter_clock.update();

  RECT mRc = get_position();
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
        && msg.hwnd != GetHandle() && !IsChild(GetHandle(), msg.hwnd)  // not current window or child
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

    if (!TranslateAccelerator(GetHandle(), mhAccelTable, &msg)){

      if (msg.message == WM_KEYUP){
	/*
	if (KeyTimer(false,msg.wParam & 0xffff)) {
	  // activate tool tips
	  1;
	} else {
	  // behave as if it was a key down event
	  if (mOnKeyDownNotify != NULL)
	    if (!(mOnKeyDownNotify)(this, msg.wParam, msg.lParam))
	      continue;
	}
	*/
      }

      if (msg.message == WM_KEYDOWN){
	//	KeyTimer(true,msg.wParam & 0xffff);

/*
        if (ActiveControl != NULL){
          switch(msg.wParam & 0xffff){
            case VK_UP:
              if (ActiveControl->GetOwner() != NULL)
                ActiveControl->GetOwner()->FocusPrev(ActiveControl);
            continue;
            case VK_DOWN:
              if (ActiveControl->GetOwner() != NULL)
                ActiveControl->GetOwner()->FocusNext(ActiveControl);
            continue;
          }
        }
*/
        if (mOnKeyDownNotify != NULL)
          if (!(mOnKeyDownNotify)(this, msg.wParam, msg.lParam))
            continue;

      }
      if (msg.message == WM_KEYUP){
        if (mOnKeyUpNotify != NULL)
          if (!(mOnKeyUpNotify)(this, msg.wParam, msg.lParam))
            continue;
      }
      if (msg.message == WM_LBUTTONUP){
        if (mOnLButtonUpNotify != NULL)
          if (!(mOnLButtonUpNotify)(this, msg.wParam, msg.lParam))
            continue;

      }

      TranslateMessage(&msg);
      if (msg.message != WM_LBUTTONUP ||
          // prevents child click from being repeat-handled by parent
          // if buttons overlap
          WndForm::timeAnyOpenClose.elapsed() > OPENCLOSESUPPRESSTIME)
      {
        if (DispatchMessage(&msg)){

          /*
          // navigation messages are moved to unhandled messages, duto nav events handling changes in event loop
          if (msg.message == WM_KEYDOWN){
            if (ActiveControl != NULL){
              switch(msg.wParam & 0xffff){
                case VK_UP:
                  if (ActiveControl->GetOwner() != NULL)
                    ActiveControl->GetOwner()->FocusPrev(ActiveControl);
                continue;
                case VK_DOWN:
                  if (ActiveControl->GetOwner() != NULL)
                    ActiveControl->GetOwner()->FocusNext(ActiveControl);
                continue;
              }
            }
          } */

        } else {

          /*
          if (msg.message == WM_KEYDOWN){
            if (ActiveControl != NULL){
              switch(msg.wParam & 0xffff){
                case VK_UP:
                  if (ActiveControl->GetOwner() != NULL)
                    ActiveControl->GetOwner()->FocusPrev(ActiveControl);
                continue;
                case VK_DOWN:
                  if (ActiveControl->GetOwner() != NULL)
                    ActiveControl->GetOwner()->FocusNext(ActiveControl);
                continue;
              }
            }
          }
          */
        } // DispatchMessage
      } // timeMsg
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

  return(mModalResult);
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
  return(WindowControl::SetForeColor(Value));
}

Color WndForm::SetBackColor(Color Value)
{
  if (mClientWindow)
  mClientWindow->SetBackColor(Value);
  return(WindowControl::SetBackColor(Value));
}

const Font *WndForm::SetFont(const Font &Value){
  if (mClientWindow)
    mClientWindow->SetFont(Value);
  return(WindowControl::SetFont(Value));
}


void WndForm::SetKeyDownNotify(int (*KeyDownNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam)){
  mOnKeyDownNotify = KeyDownNotify;
}

void WndForm::SetKeyUpNotify(int (*KeyUpNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam)){
  mOnKeyUpNotify = KeyUpNotify;
}

void WndForm::SetLButtonUpNotify( int (*LButtonUpNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam)){
  mOnLButtonUpNotify = LButtonUpNotify;
}

void WndForm::SetTimerNotify(int (*OnTimerNotify)(WindowControl * Sender)) {
  mOnTimerNotify = OnTimerNotify;
}

void
WndForm::SetUserMsgNotify(bool (*OnUserMsgNotify)(WindowControl *Sender, unsigned id))
{
  mOnUserMsgNotify = OnUserMsgNotify;
}

// normal form stuff (nonmodal)

int WndForm::OnUnhandledMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

  MSG msg;
  msg.hwnd = hwnd;
  msg.message = uMsg;
  msg.wParam = wParam;
  msg.lParam = lParam;
  msg.time = 0;
  msg.pt.x = 0;
  msg.pt.y = 0;

  /*if (msg.message == WM_ACTIVATE){
    msg.wParam = WA_ACTIVE;
  }*/

  if (msg.message == WM_KEYUP){
  }
  if (msg.message == WM_KEYDOWN){
    if (mOnKeyDownNotify != NULL)
      if (!(mOnKeyDownNotify)(this, msg.wParam, msg.lParam))
        return(0);

  }
  if (msg.message == WM_KEYUP){
    if (mOnKeyUpNotify != NULL)
      if (!(mOnKeyUpNotify)(this, msg.wParam, msg.lParam))
        return(0);
  }
  if (msg.message == WM_LBUTTONUP){
    bLButtonDown=false;
    if (mOnLButtonUpNotify != NULL)
      if (!(mOnLButtonUpNotify)(this, msg.wParam, msg.lParam))
        return(0);

  }

  if (uMsg == WM_KEYDOWN){
    if (ActiveControl != NULL){
      switch(wParam & 0xffff){
        case VK_UP:
          if (ActiveControl->GetOwner() != NULL)
            ActiveControl->GetOwner()->FocusPrev(ActiveControl);
        return(0);
        case VK_DOWN:
          if (ActiveControl->GetOwner() != NULL)
            ActiveControl->GetOwner()->FocusNext(ActiveControl);
        return(0);
      }
    }
  }
  else if (uMsg == WM_LBUTTONDOWN){
    bLButtonDown=true;

    /*

    SetActiveWindow(hwnd);
    SetFocus(hwnd);

    if (!IsChild(GetHandle(), GetTopWindow(GetHandle()))){
      Show();
    }

    */
  }

  return(1);

}

void WndForm::Show(void){

  WindowControl::Show();

  bring_to_top();

//  SetFocus(GetTopWindow(GetHandle()));

//  SetActiveWindow(GetHandle());

}

//-----------------------------------------------------------
// WndButton
//-----------------------------------------------------------

WndButton::WndButton(WindowControl *Parent, const TCHAR *Name, const TCHAR *Caption, int X, int Y, int Width, int Height, void(*Function)(WindowControl * Sender)):
      WindowControl(Parent, NULL /*Parent->GetHandle()*/, Name, X, Y, Width, Height){

  mOnClickNotify = Function;
  mDown = false;
  mDefault = false;
  mCanFocus = true;

  SetForeColor(GetOwner()->GetForeColor());
  SetBackColor(GetOwner()->GetBackColor());

  _tcscpy(mCaption, Caption);

  mLastDrawTextHeight = -1;

}

void WndButton::Destroy(void){

  WindowControl::Destroy();

}


bool
WndButton::on_mouse_up(int x, int y)
{
  POINT Pos;

  mDown = false;
  on_paint(get_canvas());
  release_capture();

  Pos.x = x;
  Pos.y = y;

  //POINTSTOPOINT(Pos, MAKEPOINTS(lParam));

  const RECT client_rect = get_client_rect();
  if (PtInRect(&client_rect, Pos)){
    if (mOnClickNotify != NULL) {
      RECT mRc = get_position();
      SetSourceRectangle(mRc);
      (mOnClickNotify)(this);
    }
  }

  return true;
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
        on_paint(get_canvas());
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
      if (!XCSoarInterface::Debounce()) return(1); // prevent false trigger
      if (mDown){
        mDown = false;
        on_paint(get_canvas());
        if (mOnClickNotify != NULL) {
          RECT mRc = get_position();
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

    rc.top += ((GetHeight()-4-mLastDrawTextHeight)/2);

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

  if (key_code == VK_UP || key_code == VK_DOWN){
#ifdef ENABLE_SDL
    // XXX
#else /* !ENABLE_SDL */
    WindowControl *owner = parent->GetOwner();
    if (owner != NULL)
      // XXX what's the correct lParam value here?
      PostMessage(owner->GetClientAreaWindow(),
                  WM_KEYDOWN, key_code, 0);
#endif /* !ENABLE_SDL */
    // pass the message to the parent window;
    return true;
  }

  if (parent->OnEditKeyDown(key_code))
    return true;

  return false;
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

int     WndProperty::InstCount=0;

WndProperty::WndProperty(WindowControl *Parent,
			 TCHAR *Name,
			 TCHAR *Caption,
			 int X, int Y,
			 int Width, int Height,
			 int CaptionWidth,
			 int (*DataChangeNotify)(WindowControl * Sender,
						 int Mode, int Value),
			 int MultiLine):
  WindowControl(Parent,
		NULL /*Parent->GetHandle()*/,
		Name, X, Y, Width, Height),
  edit(this) {

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
}

void WndProperty::Destroy(void){

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

  edit.reset();

  WindowControl::Destroy();

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
  return(res);
}

void WndProperty::UpdateButtonData(int Value){

  if (Value == 0) // if combo is enabled
    mBitmapSize = 0;
  else
    mBitmapSize = DLGSCALE(32)/2;

  if (mCaptionWidth != 0){
    mEditSize.x = GetWidth()- mCaptionWidth - (DEFAULTBORDERPENWIDTH+1) - mBitmapSize;
    mEditSize.y = GetHeight()-2*(DEFAULTBORDERPENWIDTH+1);
    mEditPos.x = mCaptionWidth;
    mEditPos.y = (DEFAULTBORDERPENWIDTH+1);
  } else {
    mEditSize.x = GetWidth()- 2*((DEFAULTBORDERPENWIDTH+1)+mBitmapSize);
    mEditSize.y = (GetHeight()/2);
    mEditPos.x = mBitmapSize + (DEFAULTBORDERPENWIDTH+2);
    mEditPos.y = (GetHeight()/2)-2*(DEFAULTBORDERPENWIDTH+1);
  }

  mHitRectDown.left = mEditPos.x-mBitmapSize;
  mHitRectDown.top = mEditPos.y + (mEditSize.y)/2 - (mBitmapSize/2);
  mHitRectDown.right = mHitRectDown.left + mBitmapSize;
  mHitRectDown.bottom = mHitRectDown.top + mBitmapSize;

  mHitRectUp.left = GetWidth()-(mBitmapSize+2);
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
  return(res);
}

bool WndProperty::SetReadOnly(bool Value){

  bool res = GetReadOnly();

  if (GetReadOnly() != Value){
    WindowControl::SetReadOnly(Value);

    edit.set_read_only(Value);
  }

  return(res);
}

bool
WndProperty::on_setfocus()
{
  edit.set_focus();
  edit.set_selection();
  return true;
}

bool
WndProperty::on_killfocus()
{
  return true;
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
WndProperty::OnEditKeyDown(unsigned key_code)
{
  switch (key_code){
    case VK_RIGHT:
      IncValue();
      return true;
    case VK_LEFT:
      DecValue();
      return true;
  }

  return false;
}

bool
WndProperty::on_key_down(unsigned key_code)
{
  switch (key_code){
    case VK_RIGHT:
      IncValue();
      return true;
    case VK_LEFT:
      DecValue();
      return true;
  }

  return WindowControl::on_key_down(key_code);
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
        set_focus();
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
  return(0);
}

int WndProperty::IncValue(void){
  if (mDataField != NULL){
    mDataField->Inc();
    edit.set_text(mDataField->GetAsString());
  }
  return(0);
}

int WndProperty::DecValue(void){
  if (mDataField != NULL){
    mDataField->Dec();
    edit.set_text(mDataField->GetAsString());
  }
  return(0);
}


void
WndProperty::on_paint(Canvas &canvas)
{

  RECT r;
  SIZE tsize;
  POINT org;

  WindowControl::on_paint(canvas);

  r.left = 0;
  r.top = 0;
  r.right = GetWidth();
  r.bottom = GetHeight();

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
    org.y = (GetHeight() - tsize.cy)/2;
  }

  if (org.x < 1)
    org.x = 1;

  // JMW TODO: use stretch functions for bigger displays, since these icons are too small for them.

  canvas.text_opaque(org.x, org.y, mCaption);

    if (mDialogStyle) // can't but dlgComboPicker here b/c it calls paint when combopicker closes too
    {     // so it calls dlgCombopicker on the click/focus handlers for the wndproperty & label
    }
    else
    {

      if (GetFocused() && !GetReadOnly()){
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
      this->SetCanFocus(true);
    }
    else
    {
      this->SetButtonSize(16);
    }

    RefreshDisplay();

  }

  return(res);

}


void
WndOwnerDrawFrame::on_paint(Canvas &canvas)
{
  WndFrame::on_paint(canvas);

  canvas.select(*GetFont());

  if (mOnPaintCallback != NULL)
    (mOnPaintCallback)(this, canvas);
}

void WndOwnerDrawFrame::Destroy(void){

  WndFrame::Destroy();

}


void WndFrame::Destroy(void){

  WindowControl::Destroy();

}


bool
WndFrame::on_key_down(unsigned key_code)
{
  if (mIsListItem && GetOwner()!=NULL){
    RECT mRc = get_position();
    SetSourceRectangle(mRc);
    return(((WndListFrame*)GetOwner())->OnItemKeyDown(this, key_code, 0));
  }

  return WindowControl::on_key_down(key_code);
}

void
WndFrame::on_paint(Canvas &canvas)
{
  if (mIsListItem && GetOwner()!=NULL) {
    ((WndListFrame*)GetOwner())->PrepareItemDraw();
  }

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
  return(res);
}

unsigned
WndFrame::GetTextHeight()
{
  RECT rc = get_client_rect();
  ::InflateRect(&rc, -2, -2); // todo border width

  Canvas &canvas = GetCanvas();
  canvas.select(*GetFont());
  canvas.formatted_text(&rc, mCaption, mCaptionStyle | DT_CALCRECT);

  return rc.bottom - rc.top;
}

WndListFrame::WndListFrame(WindowControl *Owner, const TCHAR *Name,
                           int X, int Y, int Width, int Height,
                           void (*OnListCallback)(WindowControl * Sender,
                                                  ListInfo_t *ListInfo)):
  WndFrame(Owner, Name, X, Y, Width, Height)
{

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
  mMouseDown = false;
  ScrollbarWidth=-1;
  ScrollbarTop=-1;

  rcScrollBarButton.top=0; // make sure this rect is initialized so we don't "loose" random lbuttondown events if scrollbar not drawn
  rcScrollBarButton.bottom=0;
  rcScrollBarButton.left=0;
  rcScrollBarButton.right=0;

  rcScrollBar.left=0;  // don't need to initialize this rect, but it's good practice
  rcScrollBar.right=0;
  rcScrollBar.top=0;
  rcScrollBar.bottom=0;

}


void WndListFrame::Destroy(void){

  WndFrame::Destroy();

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
    Viewport viewport(canvas, mClients[0]->GetWidth(),
                      mClients[0]->GetHeight());
    Canvas &canvas2 = viewport;

    viewport.move(mClients[0]->GetLeft(), 0);

    for (i=0; i<mListInfo.ItemInViewCount; i++){
      canvas2.select(*mClients[0]->GetFont());

      if (mOnListCallback != NULL){
        mListInfo.DrawIndex = mListInfo.TopIndex + i;
        if (mListInfo.DrawIndex == mListInfo.ItemIndex) {
          viewport.move(0, mClients[0]->GetHeight());
          continue;
        }
        mOnListCallback(this, &mListInfo);
      }

      mClients[0]->PaintSelector(true);
      mClients[0]->on_paint(canvas2);
      mClients[0]->PaintSelector(false);

      viewport.commit();
      viewport.move(0, mClients[0]->GetHeight());
    }

    viewport.restore();

    mListInfo.DrawIndex = mListInfo.ItemIndex;

    DrawScrollBar(canvas);
  }
}

void WndListFrame::Redraw(void){
  WindowControl::Redraw();  // redraw all but not the current
  mClients[0]->Redraw();    // redraw the current
}


void WndListFrame::DrawScrollBar(Canvas &canvas) {
  static Bitmap hScrollBarBitmapTop;
  static Bitmap hScrollBarBitmapMid;
  static Bitmap hScrollBarBitmapBot;
  static Bitmap hScrollBarBitmapFill;
  RECT rc;

  if ( ScrollbarWidth == -1) {  // resize height for each dialog so top button is below 1st item (to avoid initial highlighted overlap)
    if (has_pointer()) {
      // shrink width factor.  Range .1 to 1 where 1 is very "fat"
      double SHRINKSBFACTOR = is_pna() ? 1.0 : 0.75;

      ScrollbarWidth = (int) (SCROLLBARWIDTH_INITIAL * InfoBoxLayout::dscale * SHRINKSBFACTOR);
      if (mClientCount > 0)
        ScrollbarTop = mClients[0]->GetHeight() + 2;
      else
        ScrollbarTop = (int)(18.0 * InfoBoxLayout::dscale + 2);
    } else {
      // thin for ALTAIR b/c no touch screen
      ScrollbarWidth = (int) (SELECTORWIDTH * 2);
      ScrollbarTop = 1;
    }
  }


  int w = GetWidth()- (ScrollbarWidth);
  int h = GetHeight() - ScrollbarTop;

  if (!hScrollBarBitmapTop.defined())
    hScrollBarBitmapTop.load(IDB_SCROLLBARTOP);
  if (!hScrollBarBitmapMid.defined())
    hScrollBarBitmapMid.load(IDB_SCROLLBARMID);
  if (!hScrollBarBitmapBot.defined())
    hScrollBarBitmapBot.load(IDB_SCROLLBARBOT);
  if (!hScrollBarBitmapFill.defined())
    hScrollBarBitmapFill.load(IDB_SCROLLBARFILL);

  Brush brush(Color(0xff, 0xff, 0xff));
  Pen pen(DEFAULTBORDERPENWIDTH, GetForeColor());
  canvas.select(pen);

  // ENTIRE SCROLLBAR AREA
  rc.left = w;
  rc.top = ScrollbarTop;
  rc.right = w + (ScrollbarWidth) - 1;
  rc.bottom = h + ScrollbarTop;

  // save scrollbar size for mouse events
  rcScrollBar.left=rc.left;
  rcScrollBar.right=rc.right;
  rcScrollBar.top=rc.top;
  rcScrollBar.bottom=rc.bottom;

  if (mListInfo.BottomIndex == mListInfo.ItemCount) { // don't need scroll bar if one page only
    return;
  }

  // draw rectangle around entire scrollbar area
  canvas.two_lines(rc.left, rc.top, rc.left, rc.bottom, rc.right, rc.bottom);
  canvas.two_lines(rc.right, rc.bottom, rc.right, rc.top, rc.left, rc.top);

  // Just Scroll Bar Slider button
  rc.left = w;
  rc.top = GetScrollBarTopFromScrollIndex()-1;
  rc.right = w + (ScrollbarWidth) - 1; // -2 if use 3x pen.  -1 if 2x pen
  rc.bottom = rc.top + GetScrollBarHeight()+2;  // +2 for 3x pen, +1 for 2x pen

  if (rc.bottom >= GetHeight() - ScrollbarWidth){
    int d;
    d= (GetHeight() - ScrollbarWidth - rc.bottom) - 1;
    rc.bottom += d;
    rc.top += d;
  }

  bool bTransparentUpDown = true;

  BitmapCanvas bitmap_canvas(canvas);

  // TOP Dn Button 32x32
  // BOT Up Button 32x32
  if (ScrollbarWidth == SCROLLBARWIDTH_INITIAL)
    {
      bitmap_canvas.select(hScrollBarBitmapTop);
      canvas.copy(w, ScrollbarTop,
                  SCROLLBARWIDTH_INITIAL, SCROLLBARWIDTH_INITIAL,
                  bitmap_canvas, 0, 0,
                  bTransparentUpDown);

      bitmap_canvas.select(hScrollBarBitmapBot);
      canvas.copy(w, h - SCROLLBARWIDTH_INITIAL + ScrollbarTop,
                  SCROLLBARWIDTH_INITIAL, SCROLLBARWIDTH_INITIAL,
                  bitmap_canvas, 0, 0,
                  bTransparentUpDown);
    }
  else
    {
      bitmap_canvas.select(hScrollBarBitmapTop);
      canvas.stretch(w, ScrollbarTop,
                     ScrollbarWidth, ScrollbarWidth,
                     bitmap_canvas,
                     0, 0, SCROLLBARWIDTH_INITIAL, SCROLLBARWIDTH_INITIAL,
                     bTransparentUpDown);

      // BOT Up Button 32x32
      bitmap_canvas.select(hScrollBarBitmapBot);
      canvas.stretch(w, h - ScrollbarWidth + ScrollbarTop,
                     ScrollbarWidth, ScrollbarWidth,
                     bitmap_canvas,
                     0, 0, SCROLLBARWIDTH_INITIAL, SCROLLBARWIDTH_INITIAL,
                     bTransparentUpDown);
    }

  // Middle Slider Button 30x28
  if (mListInfo.ItemCount > mListInfo.ItemInViewCount){

    // handle on slider
    bitmap_canvas.select(hScrollBarBitmapMid);
    if (ScrollbarWidth == SCROLLBARWIDTH_INITIAL)
      {
        canvas.copy_and(w + 1, rc.top + GetScrollBarHeight() / 2 - 14, 30, 28,
                        bitmap_canvas, 0, 0);
        // always SRCAND b/c on top of scrollbutton texture
      }
    else
      {
        static int SCButtonW = -1;
        static int SCButtonH = -1;
        static int SCButtonY = -1;
        if (SCButtonW == -1) {
          SCButtonW = (int) (30.0 * (float)ScrollbarWidth / (float)SCROLLBARWIDTH_INITIAL);
          SCButtonH = (int) (28.0 * (float)ScrollbarWidth / (float)SCROLLBARWIDTH_INITIAL);
          SCButtonY = (int) (14.0 * (float)ScrollbarWidth / (float)SCROLLBARWIDTH_INITIAL);
        }

        canvas.stretch_and(w + 1, rc.top + GetScrollBarHeight() / 2 - SCButtonY,
                           SCButtonW, SCButtonH,
                           bitmap_canvas, 0, 0, 30, 28);
        // always SRCAND b/c on top of scrollbutton texture
      }

    // box around slider rect
    Pen pen3(DEFAULTBORDERPENWIDTH * 2, GetForeColor());
    int iBorderOffset = 1;  // set to 1 if BORDERWIDTH >2, else 0
    canvas.select(pen3);
    canvas.two_lines(rc.left + iBorderOffset, rc.top,
                     rc.left + iBorderOffset, rc.bottom,
                     rc.right, rc.bottom); // just left line of scrollbar
    canvas.two_lines(rc.right, rc.bottom,
                     rc.right, rc.top,
                     rc.left + iBorderOffset, rc.top); // just left line of scrollbar
  } // more items than fit on screen

  rcScrollBarButton.left=rc.left;
  rcScrollBarButton.right=rc.right;
  rcScrollBarButton.top=rc.top;
  rcScrollBarButton.bottom=rc.bottom;
}


void WndListFrame::SetEnterCallback(void
                                    (*OnListCallback)(WindowControl *Sender,
                                                      ListInfo_t *ListInfo))
{
  mOnListEnterCallback = OnListCallback;
}


void WndListFrame::RedrawScrolled(bool all) {

  int newTop;

  /*       -> inefficient and flickering draws the list twice
  if (all) {
    int i;
    for (i=0; i<= mListInfo.ItemInViewCount; i++) {
      mListInfo.DrawIndex = mListInfo.TopIndex+i;
      mOnListCallback(this, &mListInfo);
      mClients[0]->SetTop(mClients[0]->GetHeight() * (i));
      mClients[0]->Redraw();
    }
  }
  */

  mListInfo.DrawIndex = mListInfo.ItemIndex;
  mOnListCallback(this, &mListInfo);
  newTop = mClients[0]->GetHeight() * (mListInfo.ItemIndex - mListInfo.TopIndex);
  if (newTop == mClients[0]->GetTop()){
    Redraw();                     // non moving the helper window force redraw
  } else {
    mClients[0]->SetTop(newTop);  // moving the helper window invalidate the list window
    mClients[0]->Redraw();

    // to be optimized: after SetTop Paint redraw all list items

  }

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
    return(1);
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
      // JMW scroll
      RedrawScrolled(true);
      return(0);
    } else {
      mListInfo.ItemIndex = mListInfo.BottomIndex-1;
      return(1);
    }
  }
  if (mListInfo.ItemIndex < 0){

    mListInfo.ItemIndex = 0;
    // JMW scroll
    if (mListInfo.ScrollIndex>0) {
      mListInfo.ScrollIndex--;
      RedrawScrolled(true);
      return(0);
    } else {
      // only return if no more scrolling left to do
      return(1);
    }
  }
  RedrawScrolled(bigscroll);
  return (0);
}


int WndListFrame::OnItemKeyDown(WindowControl *Sender, WPARAM wParam, LPARAM lParam){
	(void)Sender;
	(void)lParam;
  switch (wParam){
#ifdef GNAV
    // JMW added this to make data entry easier
    case VK_F4:
#endif
  case VK_RETURN:
    if (mOnListEnterCallback) {
      mOnListEnterCallback(this, &mListInfo);
      RedrawScrolled(false);
      return(0);
    } else
      return(1);
    //#ifndef GNAV
  case VK_LEFT:
    if ((mListInfo.ScrollIndex>0)
	&&(mListInfo.ItemCount>mListInfo.ItemInPageCount)) {
      mListInfo.ScrollIndex -= mListInfo.ItemInPageCount;
    }
    return RecalculateIndices(true);
  case VK_RIGHT:
    if ((mListInfo.ItemIndex+mListInfo.ScrollIndex<
	 mListInfo.ItemCount)
	&&(mListInfo.ItemCount>mListInfo.ItemInPageCount)) {
      mListInfo.ScrollIndex += mListInfo.ItemInPageCount;
    }
    return RecalculateIndices(true);
    //#endif
  case VK_DOWN:


    mListInfo.ItemIndex++;
    return RecalculateIndices(false);
  case VK_UP:
    mListInfo.ItemIndex--;
    return RecalculateIndices(false);
  }
  mMouseDown=false;
  return(1);

}

void WndListFrame::ResetList(void){

  mListInfo.ScrollIndex = 0;
  mListInfo.ItemIndex = 0;
  mListInfo.DrawIndex = 0;
  mListInfo.ItemInPageCount = ((GetHeight()+mClients[0]->GetHeight()-1)
			       /mClients[0]->GetHeight())-1;
  mListInfo.TopIndex = 0;
  mListInfo.BottomIndex = 0;
//  mListInfo.SelectedIndex = 0;
  mListInfo.ItemCount = 0;
  mListInfo.ItemInViewCount = (GetHeight()+mClients[0]->GetHeight()-1)
    /mClients[0]->GetHeight()-1;

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

  mClients[0]->SetTop(0);     // move item window to the top
  mClients[0]->Redraw();
}

int WndListFrame::PrepareItemDraw(void){
  if (mOnListCallback)
    mOnListCallback(this, &mListInfo);
  return(1);
}

bool
WndListFrame::on_mouse_up(int x, int y)
{
    mMouseDown=false;
    return false;
}

static bool isselect = false;

// JMW needed to support mouse/touchscreen
bool
WndFrame::on_mouse_down(int xPos, int yPos)
{
  if (mIsListItem && GetOwner()!=NULL) {

    if (!GetFocused()) {
      set_focus();
      //return(1);
    }
    //else {  // always doing this allows selected item in list to remain selected.
      invalidate();
    //}

    WndListFrame* wlf = ((WndListFrame*)GetOwner());
    RECT mRc = get_position();
    wlf->SelectItemFromScreen(xPos, yPos, &mRc);
  }
  isselect = false;
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

void WndListFrame::SelectItemFromScreen(int xPos, int yPos,
                                        RECT *rect) {
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
  int index;
  *rect = get_position();
  index = yPos/mClients[0]->GetHeight(); // yPos is offset within ListEntry item!

  if ((index>=0)&&(index<mListInfo.BottomIndex)) {
    if (!isselect) {
      if (mOnListEnterCallback) {
        mOnListEnterCallback(this, &mListInfo);
      }
      RedrawScrolled(false);
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

    POINT Pos;
    Pos.x = x;
    Pos.y = y;

    if (mMouseDown && PtInRect(&rcScrollBar, Pos))
    {
      int iScrollBarTop = max(1, (int)Pos.y - mMouseScrollBarYOffset);

      int iScrollIndex = GetScrollIndexFromScrollBarTop(iScrollBarTop);

      if(iScrollIndex !=mListInfo.ScrollIndex)
      {
        int iScrollAmount = iScrollIndex - mListInfo.ScrollIndex;
        mListInfo.ScrollIndex = mListInfo.ScrollIndex + iScrollAmount;
        Redraw();
      }
    }
    else //not in scrollbar
    {
      mMouseDown = false; // force re-click of scroll bar
    }
    bMoving=false;
  } // Tickcount
  return false;
}

bool
WndListFrame::on_mouse_down(int x, int y)
{
  POINT Pos;
  Pos.x = x;
  Pos.y = y;
  mMouseDown=false;

  if (PtInRect(&rcScrollBarButton, Pos))  // see if click is on scrollbar handle
  {
    // start mouse drag
    mMouseScrollBarYOffset = max(0, (int)(Pos.y - rcScrollBarButton.top));
    mMouseDown=true;

  }
  else if (PtInRect(&rcScrollBar, Pos)) // clicked in scroll bar up/down/pgup/pgdn
  {
    if (Pos.y - rcScrollBar.top < (ScrollbarWidth)) // up arrow
      mListInfo.ScrollIndex = max(0, mListInfo.ScrollIndex- 1);

    else if (rcScrollBar.bottom -Pos.y < (ScrollbarWidth)  ) //down arrow
      mListInfo.ScrollIndex = max(0,min(mListInfo.ItemCount- mListInfo.ItemInViewCount, mListInfo.ScrollIndex+ 1));

    else if (Pos.y < rcScrollBarButton.top) // page up
      mListInfo.ScrollIndex = max(0, mListInfo.ScrollIndex- mListInfo.ItemInViewCount);

    else // page down
      if (mListInfo.ItemCount > mListInfo.ScrollIndex+ mListInfo.ItemInViewCount)
          mListInfo.ScrollIndex = min ( mListInfo.ItemCount- mListInfo.ItemInViewCount, mListInfo.ScrollIndex +mListInfo.ItemInViewCount);

    Redraw();

  }
  else
  if (mClientCount > 0)
  {
    isselect = true;
    ((WndFrame *)mClients[0])->on_mouse_down(x, y);
  }

  return false;
}

inline int WndListFrame::GetScrollBarHeight (void)
{
  int h = GetHeight() - ScrollbarTop;
  if(mListInfo.ItemCount ==0)
    return h-2*ScrollbarWidth;
  else
    return max(ScrollbarWidth,((h-2*ScrollbarWidth)*mListInfo.ItemInViewCount)/mListInfo.ItemCount);
}

inline int WndListFrame::GetScrollIndexFromScrollBarTop(int iScrollBarTop)
{
  int h = GetHeight() - ScrollbarTop;
  if (h-2*(ScrollbarWidth) - GetScrollBarHeight() == 0)
    return 0;
  else

    return max(0,
              min(mListInfo.ItemCount - mListInfo.ItemInPageCount,
              max(0,
                  ( 0 +
                    (mListInfo.ItemCount-mListInfo.ItemInViewCount)
                    * (iScrollBarTop - (ScrollbarWidth)-ScrollbarTop)
                  )
                    / ( h-2*(ScrollbarWidth) - GetScrollBarHeight() ) /*-ScrollbarTop(*/
              )
           ));
}

inline int WndListFrame::GetScrollBarTopFromScrollIndex()
{
  int iRetVal=0;
  int h = GetHeight() - ScrollbarTop;
  if (mListInfo.ItemCount - mListInfo.ItemInViewCount ==0) {
    iRetVal= h + (ScrollbarWidth);
  }
  else {
    iRetVal =
      ( (ScrollbarWidth)+ScrollbarTop +
        (mListInfo.ScrollIndex) *(h-2*(ScrollbarWidth)-GetScrollBarHeight() ) /*-ScrollbarTop*/
      /(mListInfo.ItemCount - mListInfo.ItemInViewCount)
      );
  }
  return iRetVal;
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
