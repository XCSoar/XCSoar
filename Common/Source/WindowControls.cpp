/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

#include "stdafx.h"
#include "tchar.h"
#include <stdio.h>
#include "WindowControls.h"
#include "MapWindow.h"

#define DEFAULTBORDERPENWIDTH 1
#define SELECTORWIDTH         4

void DataField::Inc(void){
  (mOnDataAccess)(this, daInc);
}

void DataField::Dec(void){
  (mOnDataAccess)(this, daDec);
}

void DataField::GetData(void){
  (mOnDataAccess)(this, daGet);
}

void DataField::SetData(void){
  (mOnDataAccess)(this, daPut);
}

  void __Dummy(DataField *Sender, DataField::DataAccessKind_t Mode){
    (void) Sender;
    (void) Mode;
  }

DataField::DataField(TCHAR *EditFormat, TCHAR *DisplayFormat, void(*OnDataAccess)(DataField *Sender, DataAccessKind_t Mode)){
  mUsageCounter=0;
  mOnDataAccess = OnDataAccess;
  _tcscpy(mEditFormat, EditFormat);
  _tcscpy(mDisplayFormat, DisplayFormat);

  if (mOnDataAccess == NULL){
    mOnDataAccess = __Dummy;
  }

}

void DataField::SetDisplayFormat(TCHAR *Value){
  _tcscpy(mDisplayFormat, Value);
}


bool DataFieldBoolean::GetAsBoolean(void){
  return(mValue);
}

int DataFieldBoolean::GetAsInteger(void){
  if (mValue)
    return(1);
  else
    return(0);
}

double DataFieldBoolean::GetAsFloat(void){
  if (mValue)
    return(1.0);
  else
    return(0.0);
}

TCHAR *DataFieldBoolean::GetAsString(void){
  if (mValue)
    return(mTextTrue);
  else
    return(mTextFalse);
}


void DataFieldBoolean::Set(bool Value){
  mValue = Value;
}

bool DataFieldBoolean::SetAsBoolean(bool Value){
  bool res = mValue;
  if (mValue != Value){
    mValue = Value;
    (mOnDataAccess)(this, daChange);
  }
  return(res);
}

int DataFieldBoolean::SetAsInteger(int Value){
  int res = GetAsInteger();
  if (GetAsInteger() != Value){
    SetAsBoolean(!(Value==0));
  }
  return(res);
}

double DataFieldBoolean::SetAsFloat(double Value){
  double res = GetAsFloat();
  if (GetAsFloat() != Value){
    SetAsBoolean(!(Value==0.0));
  }
  return(res);
}

TCHAR *DataFieldBoolean::SetAsString(TCHAR *Value){
  TCHAR *res = GetAsString();
  if (_tcscmp(res, Value) != 0){
    SetAsBoolean(_tcscmp(Value, mTextTrue) == 0);
  }
  return(res);
}

void DataFieldBoolean::Inc(void){
  SetAsBoolean(!GetAsBoolean());
}

void DataFieldBoolean::Dec(void){
  SetAsBoolean(!GetAsBoolean());
}

//----------------------------------------------------------
// DataField Integer
//----------------------------------------------------------


bool DataFieldInteger::GetAsBoolean(void){
  return(mValue != 0);
}

int DataFieldInteger::GetAsInteger(void){
  return(mValue);
}

double DataFieldInteger::GetAsFloat(void){
  return(mValue);
}

TCHAR *DataFieldInteger::GetAsString(void){
  _stprintf(mOutBuf, mEditFormat, mValue);
  return(mOutBuf);
}

TCHAR *DataFieldInteger::GetAsDisplayString(void){
  _stprintf(mOutBuf, mDisplayFormat, mValue);
  return(mOutBuf);
}


void DataFieldInteger::Set(int Value){
  mValue = Value;
}

bool DataFieldInteger::SetAsBoolean(bool Value){
  bool res = GetAsBoolean();
  if (Value)
    SetAsInteger(1);
  else
    SetAsInteger(0);
  return(res);
}

int DataFieldInteger::SetAsInteger(int Value){
  int res = mValue;
  if (Value < mMin)
    Value = mMin;
  if (Value > mMax)
    Value = mMax;
  if (mValue != Value){
    mValue = Value;
    (mOnDataAccess)(this, daChange);
  }
  return(res);
}

double DataFieldInteger::SetAsFloat(double Value){
  double res = GetAsFloat();
  SetAsInteger((int)(Value+0.5));
  return(res);
}

TCHAR *DataFieldInteger::SetAsString(TCHAR *Value){
  TCHAR *res = GetAsString();
  SetAsInteger(_ttoi(Value));
  return(res);
}

void DataFieldInteger::Inc(void){
  SetAsInteger(mValue + mStep*SpeedUp());
}

void DataFieldInteger::Dec(void){
  SetAsInteger(mValue - mStep*SpeedUp());
}

int DataFieldInteger::SpeedUp(void){
  int res=1;
  if ((long)(GetTickCount()-mTmLastStep) < 200){
    mSpeedup++;

    if (mSpeedup > 5){
      res = 10;

      mTmLastStep = GetTickCount()+350;
      return(res);

    }
  } else
    mSpeedup = 0;

  mTmLastStep = GetTickCount();

  return(res);
}


//----------------------------------------------------------
// DataField Float
//----------------------------------------------------------


bool DataFieldFloat::GetAsBoolean(void){
  return(mValue != 0.0);
}

int DataFieldFloat::GetAsInteger(void){
  return(int)(mValue+0.5);
}

double DataFieldFloat::GetAsFloat(void){
  return(mValue);
}

TCHAR *DataFieldFloat::GetAsString(void){
  _stprintf(mOutBuf, mEditFormat, mValue);
  return(mOutBuf);
}

TCHAR *DataFieldFloat::GetAsDisplayString(void){
  _stprintf(mOutBuf, mDisplayFormat, mValue);
  return(mOutBuf);
}

void DataFieldFloat::Set(double Value){
  mValue = Value;
}

double DataFieldFloat::SetMin(double Value){
  double res = mMin;
  mMin = Value;
  return(res);
};
double DataFieldFloat::SetMax(double Value){
  double res = mMax;
  mMax = Value;
  return(res);
};

bool DataFieldFloat::SetAsBoolean(bool Value){
  bool res = GetAsBoolean();
  if (res != Value){
    if (Value)
      SetAsFloat(1.0);
    else
      SetAsFloat(0.0);
  }
  return(res);
}

int DataFieldFloat::SetAsInteger(int Value){
  int res = GetAsInteger();
  SetAsFloat(Value);
  return(res);
}

double DataFieldFloat::SetAsFloat(double Value){
  double res = mValue;
  if (Value < mMin)
    Value = mMin;
  if (Value > mMax)
    Value = mMax;
  if (res != Value){
    mValue = Value;
    (mOnDataAccess)(this, daChange);
  }
  return(res);
}

TCHAR *DataFieldFloat::SetAsString(TCHAR *Value){
  TCHAR *res = GetAsString();
  SetAsFloat(_tcstod(Value, NULL));
  return(res);
}

void DataFieldFloat::Inc(void){
  SetAsFloat(mValue + mStep*SpeedUp());
}

void DataFieldFloat::Dec(void){
  SetAsFloat(mValue - mStep*SpeedUp());
}

double DataFieldFloat::SpeedUp(void){
  double res=1;
  if ((long)(GetTickCount()-mTmLastStep) < 200){
    mSpeedup++;

    if (mSpeedup > 5){
      res = 10;

      mTmLastStep = GetTickCount()+350;
      return(res);

    }
  } else
    mSpeedup = 0;

  mTmLastStep = GetTickCount();

  return(res);
}


//----------------------------------------------------------
// DataField String
//----------------------------------------------------------


TCHAR *DataFieldString::SetAsString(TCHAR *Value){
  _tcscpy(mValue, Value);
  return(mValue);
}

void DataFieldString::Set(TCHAR *Value){
  _tcscpy(mValue, Value);
}

TCHAR *DataFieldString::GetAsString(void){
  return(mValue);
}

TCHAR *DataFieldString::GetAsDisplayString(void){
  return(mValue);
}


//----------------------------------------------------------
// WindowControl Classes
//----------------------------------------------------------


extern HFONT  MapWindowFont;
extern HINSTANCE hInst;
WindowControl *ActiveControl = NULL;
WindowControl *LastFocusControl = NULL;


void InitWindowControlModule(void);
LRESULT CALLBACK WindowControlWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static COLORREF bkColor = clWhite;
static COLORREF fgColor = clBlack;
int WindowControl::InstCount=0;
HBRUSH WindowControl::hBrushDefaultBk=NULL;
HPEN WindowControl::hPenDefaultBorder=NULL;
HPEN WindowControl::hPenDefaultSelector=NULL;

WindowControl::WindowControl(WindowControl *Owner, HWND Parent, TCHAR *Name, int X, int Y, int Width, int Height, bool Visible){

  mHasFocus = false;
  mCanFocus = false;

  mReadOnly = false;

  mClientCount = 0;

  // todo

  DWORD Style = 0;

  mX = X;
  mY = Y;
  mWidth = Width;
  mHeight = Height;
  mParent = Parent;
  mOwner = Owner;
  // todo
  mhFont = MapWindowFont;
  mVisible = Visible;
  mCaption[0] = '\0';
  mDontPaintSelector = false;

  if ((mParent == NULL) && (mOwner != NULL))
    mParent = mOwner->GetClientAeraHandle();

  if (Name != NULL)
    _tcscpy(mName, Name);  // todo size check
  else
    mName[0] = '\0';

  InitWindowControlModule();

  mColorBack = bkColor;
  mColorFore = fgColor;

  if (InstCount == 0){
    hBrushDefaultBk = (HBRUSH)CreateSolidBrush(mColorBack);
    hPenDefaultBorder = (HPEN)CreatePen(PS_SOLID, DEFAULTBORDERPENWIDTH, mColorFore);
    hPenDefaultSelector = (HPEN)CreatePen(PS_SOLID, DEFAULTBORDERPENWIDTH+2, mColorFore);
  }
  InstCount++;

  Style = WS_CHILD | ES_MULTILINE | ES_CENTER
    | ES_READONLY | WS_CLIPCHILDREN
    | WS_CLIPSIBLINGS;

  if (mParent == NULL)
    Style |= WS_POPUP;

  mHWnd = CreateWindow(TEXT("STATIC"), TEXT("\0"),
		     Style,
		     mX, mY,
		     mWidth, mHeight,
		     mParent, NULL, hInst, NULL);

  SetWindowPos(mHWnd, HWND_TOP,
		     mX, mY,
		     mWidth, mHeight,
	       SWP_HIDEWINDOW);

  if (mOwner != NULL)
    mOwner->AddClient(this);

  mBoundRect.top = 0;
  mBoundRect.left = 0;
  mBoundRect.right = GetWidth();
  mBoundRect.bottom = GetHeight();

  mSavWndProcedure = GetWindowLong(mHWnd, GWL_WNDPROC);
  SetWindowLong(mHWnd, GWL_USERDATA, (long)this);
  SetWindowLong(mHWnd, GWL_WNDPROC, (LONG) WindowControlWndProc);

  mHdc = GetDC(mHWnd);
  mHdcTemp = CreateCompatibleDC(mHdc);

  /* JMW debugging
  mBmpMem = CreateCompatibleBitmap(mHdc, mWidth, mHeight);
  SelectObject(mHdcTemp, mBmpMem);
  */

  mhBrushBk = hBrushDefaultBk;
  mhPenBorder = hPenDefaultBorder;
  mhPenSelector = hPenDefaultSelector;
  mBorderSize = 1;

  mBorderKind = 0; //BORDERRIGHT | BORDERBOTTOM;

  SetBkMode(mHdc, TRANSPARENT);

  if (mVisible)
    ShowWindow(GetHandle(), SW_SHOW);

}

WindowControl::~WindowControl(void){

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

  if (mhBrushBk != hBrushDefaultBk){
    DeleteObject(mhBrushBk);
  }
  if (mhPenBorder != hPenDefaultBorder){
    DeleteObject(mhPenBorder);
  }
  if (mhPenSelector != hPenDefaultSelector){
    DeleteObject(mhPenSelector);
  }

  ReleaseDC(mHWnd, mHdc);
  DeleteDC(mHdcTemp);
  /* JMW debugging
  DeleteObject(mBmpMem);
  */
  SetWindowLong(mHWnd, GWL_WNDPROC, (LONG) mSavWndProcedure);
  SetWindowLong(mHWnd, GWL_USERDATA, (long)0);

  // SetWindowLong(mHWnd, GWL_WNDPROC, (LONG) WindowControlWndProc);
  // ShowWindow(GetHandle(), SW_SHOW);
  DestroyWindow(mHWnd);

  InstCount--;
  if (InstCount==0){
    DeleteObject(hBrushDefaultBk);
    DeleteObject(hPenDefaultBorder);
    DeleteObject(hPenDefaultSelector);
  }

}

void WindowControl::UpdatePosSize(void){

  mBoundRect.top = 0;
  mBoundRect.left = 0;
  mBoundRect.right = GetWidth();
  mBoundRect.bottom = GetHeight();

  SetWindowPos(GetHandle(),0,
     mX, mY,
     mWidth, mHeight,
     SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
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
    if (mClients[idx]->GetCanFocus()){
      return(mClients[idx]);
    }
  }
  return(NULL);
};

void WindowControl::AddClient(WindowControl *Client){
  mClients[mClientCount] = Client;
  mClientCount++;

  Client->SetOwner(this);
  // dont work propertly
//  Client->SetParentHandle(GetHandle());
  Client->SetFont(GetFont());

  if (Client->mY == -1){
    if (mClientCount > 1){
      Client->mY = mClients[mClientCount-2]->mY + mClients[mClientCount-2]->mHeight;
      SetWindowPos(Client->GetHandle(), 0,
		     Client->mX, Client->mY,
		     0, 0,
	       SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
  }

}

WindowControl *WindowControl::FindByName(TCHAR *Name){
  if (_tcscmp(mName, Name)==0)
    return(this);
  for (int i=0; i<mClientCount; i++){
    WindowControl *W = mClients[i]->FindByName(Name);
    if (W != NULL)
      return(W);
  }
  return(NULL);
}


void WindowControl::SetParentHandle(HWND hwnd){
  mParent = hwnd;
  SetParent(GetHandle(), hwnd);
}


WindowControl *WindowControl::SetOwner(WindowControl *Value){
  WindowControl *res = mOwner;
  if (mOwner != Value){
    mOwner = Value;
  }
  return(res);
}

void WindowControl::SetCaption(TCHAR *Value){

  if (Value == NULL && mCaption[0] != '\0'){
    mCaption[0] ='\0';
    InvalidateRect(GetHandle(), GetBoundRect(), false);
    UpdateWindow(GetHandle());

    return;

  }

  if (_tcscmp(mCaption, Value) != 0){

    _tcscpy(mCaption, Value);

    InvalidateRect(GetHandle(), GetBoundRect(), false);
    UpdateWindow(GetHandle());


  }

}

bool WindowControl::SetFocused(bool Value, HWND FromTo){

  bool res = mHasFocus;

  if (mHasFocus != Value){
    mHasFocus = Value;

    if (mCanFocus){
      RECT rc;
      rc.left = 0;
      rc.top = 0;
      rc.right = GetWidth();
      rc.bottom = GetHeight();
      InvalidateRect(GetHandle(), &rc, false);
      // todo, only paint the selector edges
      UpdateWindow(GetHandle());
      // Paint(GetDeviceContext());
    }

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

bool WindowControl::SetVisible(bool Value){
  bool res = mVisible;
  if (mVisible != Value){

    mVisible = Value;

    /*
    for (int i=0; i<mClientCount; i++){
      mClients[i]->SetVisible(mVisible);
    }
    */

    if (mVisible){
      InvalidateRect(GetHandle(), GetBoundRect(), false);
      UpdateWindow(GetHandle());
      ShowWindow(GetHandle(), SW_SHOW);
    } else {
      ShowWindow(GetHandle(), SW_HIDE);
    }

  }
  return(res);
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
    InvalidateRect(GetHandle(), GetBoundRect(), false);
    UpdateWindow(GetHandle());
  }
  return(res);
}

HFONT WindowControl::SetFont(HFONT Value){
  HFONT res = mhFont;
  if (mhFont != Value){
    // todo
    mhFont = Value;
  }
  return(res);
}

bool WindowControl::SetReadOnly(bool Value){
  bool res = mReadOnly;
  if (mReadOnly != Value){
    mReadOnly = Value;
    Paint(GetDeviceContext());
  }
  return(res);
}

COLORREF WindowControl::SetForeColor(COLORREF Value){
  COLORREF res = mColorFore;
  if (mColorFore != Value){
    mColorFore = Value;
    if (mVisible)
      Paint(GetDeviceContext());
  }
  return(res);
}

COLORREF WindowControl::SetBackColor(COLORREF Value){
  COLORREF res = mColorBack;
  if (mColorBack != Value){
    mColorBack = Value;
    if (mhBrushBk != hBrushDefaultBk){
      DeleteObject(mhBrushBk);
    }
    mhBrushBk = (HBRUSH)CreateSolidBrush(mColorBack);
    if (mVisible)
      Paint(GetDeviceContext());
  }
  return(res);
}

void WindowControl::PaintSelector(HDC hDC){

  if (!mDontPaintSelector && mCanFocus && mHasFocus){
    HPEN oldPen = (HPEN)SelectObject(hDC, hPenDefaultSelector);

    MoveToEx(hDC, mWidth-SELECTORWIDTH-1, 0, NULL);
    LineTo(hDC, mWidth-1, 0);
    LineTo(hDC, mWidth-1, SELECTORWIDTH+1);

    MoveToEx(hDC, mWidth-1, mHeight-SELECTORWIDTH-2, NULL);
    LineTo(hDC, mWidth-1, mHeight-1);
    LineTo(hDC, mWidth-SELECTORWIDTH-1, mHeight-1);

    MoveToEx(hDC, SELECTORWIDTH+1, mHeight-1, NULL);
    LineTo(hDC, 0, mHeight-1);
    LineTo(hDC, 0, mHeight-SELECTORWIDTH-2);

    MoveToEx(hDC, 0, SELECTORWIDTH+1, NULL);
    LineTo(hDC, 0, 0);
    LineTo(hDC, SELECTORWIDTH+1, 0);

    SelectObject(hDC,oldPen);
  }

}

void WindowControl::Redraw(void){
  if (GetVisible()){
    InvalidateRect(GetHandle(), GetBoundRect(), false);
    UpdateWindow(GetHandle());
  }
}


void WindowControl::Paint(HDC hDC){

  RECT rc;

  rc.left = 0;
  rc.top = 0;
  rc.right = 0 + mWidth+2;
  rc.bottom = 0 + mHeight+2;

  if (!mVisible) return;

  FillRect(hDC, &rc, mhBrushBk);

  if (mBorderKind != 0){

    HPEN oldPen = (HPEN)SelectObject(hDC, mhPenBorder);

    if (mBorderKind & BORDERTOP){
      MoveToEx(hDC, 0, 0, NULL);
      LineTo(hDC, mWidth, 0);
    }
    if (mBorderKind & BORDERRIGHT){
      MoveToEx(hDC, mWidth-1, 0, NULL);
      LineTo(hDC, mWidth-1, mHeight);
    }
    if (mBorderKind & BORDERBOTTOM){
      MoveToEx(hDC, mWidth-1, mHeight-1, NULL);
      LineTo(hDC, -1, mHeight-1);
    }
    if (mBorderKind & BORDERLEFT){
      MoveToEx(hDC, 0, mHeight-1, NULL);
      LineTo(hDC, 0, -1);
    }
    SelectObject(hDC,oldPen);
  }

  PaintSelector(hDC);

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
      SetFocus(W->GetHandle());
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
      SetFocus(W->GetHandle());
      return(W);
    }

  if (GetOwner() != NULL){
    return(GetOwner()->FocusPrev(this));
  }

  return(NULL);
}

LRESULT CALLBACK WindowControlWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	WindowControl *w = (WindowControl *) GetWindowLong(hwnd, GWL_USERDATA);

	if (w)
		return (w->WndProc(hwnd, uMsg, wParam, lParam));
	else
		return (DefWindowProc(hwnd, uMsg, wParam, lParam));
}

int WindowControl::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

  PAINTSTRUCT ps;            // structure for paint info
  HDC hDC;                   // handle to graphics device context,

  switch (uMsg){

    case WM_PAINT:
      hDC = BeginPaint(GetHandle(), &ps);
      Paint(hDC);

      DeleteDC(hDC);
      EndPaint(GetHandle(), &ps);
    return(0);

    case WM_WINDOWPOSCHANGED:
      //ib = (WindowControl *)GetWindowLong(hwnd, GWL_USERDATA);
      //ib->Paint(ib->GetDeviceContext());
    return 0;

    case WM_CREATE:
    break;

    case WM_DESTROY:
      MapWindow::RequestFastRefresh=true;
    break;

    case WM_COMMAND:
      if (OnCommand(wParam, lParam)) return(0);
    break;

    case WM_LBUTTONDBLCLK:
      if (!OnLButtonDoubleClick(wParam, lParam)) return(0);
    break;

    case WM_LBUTTONDOWN:
      if (!OnLButtonDown(wParam, lParam)) return(0);
    break;

    case WM_LBUTTONUP:
      if (!OnLButtonUp(wParam, lParam)) return(0);
    break;

    case WM_KEYDOWN:
//      if (!OnKeyDown(wParam, lParam)) return(0);
      return(OnKeyDown(wParam, lParam));
//    break;

    case WM_KEYUP:
//      if (!OnKeyUp(wParam, lParam)) return(0);
      return(OnKeyUp(wParam, lParam));
//    break;

    case WM_SETFOCUS:
      SetFocused(true, (HWND) wParam);
    return(0);

    case WM_KILLFOCUS:
      SetFocused(false, (HWND) wParam);
    return(0);

    case WM_ACTIVATE:
      /*
      if (wParam == WA_ACTIVE){
        if (LastFocusControl != NULL)
          SetFocus(LastFocusControl->GetHandle());
      }
      return(0);
      */
    break;

    case WM_QUIT:
    case WM_CLOSE:
      Close();
    return(0);

  }

  return (DefWindowProc (hwnd, uMsg, wParam, lParam));
}


void InitWindowControlModule(void){

  static bool InitDone = false;

  if (InitDone)
    return;

  ActiveControl = NULL;

  InitDone = true;

}


ACCEL  WndForm::mAccel[] = {
  {0, VK_ESCAPE,  VK_ESCAPE},
  {0, VK_RETURN,  VK_RETURN},
};

WndForm::WndForm(HWND Parent, TCHAR *Name, TCHAR *Caption, int X, int Y, int Width, int Height):
      WindowControl(NULL, Parent, Name, X, Y, Width, Height, false){

  mClientWindow = NULL;
  mOnKeyDownNotify = NULL;
  mOnKeyUpNotify = NULL;
  mOnLButtonUpNotify = NULL;

  mhAccelTable = CreateAcceleratorTable(mAccel, sizeof(mAccel)/sizeof(mAccel[0]));

  mColorTitle = clAqua;

  mhTitleFont = GetFont();

  mhBrushTitle = (HBRUSH)CreateSolidBrush(mColorTitle);

  mClientWindow = new WindowControl(this, GetHandle(), TEXT(""), 20, 20, Width, Height);
  mClientWindow->SetBackColor(GetBackColor());
  mClientWindow->SetCanFocus(false);

  mClientRect.top=0;
  mClientRect.left=0;
  mClientRect.bottom=Width;
  mClientRect.right=Height;

  mModalResult = 0;
  if (Caption != NULL)
    _tcscpy(mCaption, Caption);

};

WndForm::~WndForm(void){
  Destroy();
}


void WndForm::Destroy(void){

  if (mClientWindow)
    mClientWindow->SetVisible(false);

  DestroyAcceleratorTable(mhAccelTable);
  DeleteObject(mhBrushTitle);

  WindowControl::Destroy();  // delets all childs

}


HWND WndForm::GetClientAeraHandle(void){

  if (mClientWindow != NULL)

    return(mClientWindow->GetHandle());

  else

    return(GetHandle());

};


void WndForm::AddClient(WindowControl *Client){             // add client window
  if (mClientWindow != NULL){
    mClientWindow->AddClient(Client);                       // add it to the clientarea window
  } else
    WindowControl::AddClient(Client);
}


int WndForm::OnCommand(WPARAM wParam, LPARAM lParam){

   if ((wParam & 0xffff) == VK_ESCAPE){
     mModalResult = mrCancle;
     return(0);
   }

   /*
   if ((wParam & 0xffff) == VK_RETURN){
     mModalResult = mrOK;
     return(0);
   }
   */

   return(1);

};

HFONT WndForm::SetTitleFont(HFONT Value){
  HFONT res = mhTitleFont;

  if (mhTitleFont != Value){
    // todo
    mhTitleFont = Value;



  }

  return(res);

}


int WndForm::ShowModal(void){

  MSG msg;
  HWND oldFocusHwnd;

  SetVisible(true);
  BringWindowToTop(GetHandle());
  SetActiveWindow(GetHandle());

  mModalResult = 0;

  oldFocusHwnd = SetFocus(GetHandle());

  FocusNext(NULL);

  while ((mModalResult == 0) && GetMessage(&msg, NULL, 0, 0)){

//hack!

    if ((msg.message == WM_KEYDOWN) && ((msg.wParam & 0xffff) == VK_ESCAPE))
      mModalResult = mrCancle;

    if ((msg.message == WM_KEYDOWN
        || msg.message == WM_KEYUP
        || msg.message == WM_KEYDOWN
        || msg.message == WM_LBUTTONDOWN
        || msg.message == WM_LBUTTONUP
        || msg.message == WM_LBUTTONDBLCLK
        ) && (msg.hwnd != GetHandle() && !IsChild(GetHandle(), msg.hwnd))) continue;   // make it modal

    if (!TranslateAccelerator(GetHandle(), mhAccelTable, &msg)){

      if (msg.message == WM_KEYDOWN){
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
      if (DispatchMessage(&msg)){

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
      }

    }
  }

  SetFocus(oldFocusHwnd);

  return(mModalResult);

}

void WndForm::Paint(HDC hDC){

  RECT rcClient;
  SIZE tsize;
  HPEN oldPen;
  HBRUSH oldBrush;

  if (!GetVisible()) return;

  CopyRect(&rcClient, GetBoundRect());

  oldPen = (HPEN)SelectObject(hDC, GetBorderPen());
  oldBrush = (HBRUSH) SelectObject(hDC, GetBackBrush());

  DrawEdge(hDC, &rcClient, EDGE_RAISED, BF_ADJUST | BF_FLAT | BF_RECT);

  SetTextColor(hDC, GetForeColor());
  SetBkColor(hDC, mColorTitle);
  SetBkMode(hDC, TRANSPARENT);

  SelectObject(hDC, mhTitleFont);
  GetTextExtentPoint(hDC, mCaption, _tcslen(mCaption), &tsize);

  CopyRect(&mTitleRect, &rcClient);
  mTitleRect.bottom = mTitleRect.top + tsize.cy;

  rcClient.top += tsize.cy;

  if (mClientWindow && !EqualRect(&mClientRect, &rcClient)){

    SetWindowPos(mClientWindow->GetHandle(), HWND_TOP,
      rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top,
      0);

    CopyRect(&mClientRect, &rcClient);

  }

  ExtTextOut(hDC, mTitleRect.left+1, mTitleRect.top-2,
  ETO_OPAQUE, &mTitleRect, mCaption, _tcslen(mCaption), NULL);

//  FillRect(hDC, &rc, GetBackBrush());

  SelectObject(hDC, oldBrush);
  SelectObject(hDC, oldPen);

}

void WndForm::SetCaption(TCHAR *Value){

  if (Value == NULL && mCaption[0] != '\0'){
    mCaption[0] ='\0';
    InvalidateRect(GetHandle(), GetBoundRect(), false);
    UpdateWindow(GetHandle());

    return;

  }

  if (_tcscmp(mCaption, Value) != 0){
    _tcscpy(mCaption, Value);
    InvalidateRect(GetHandle(), &mTitleRect, false);
    UpdateWindow(GetHandle());


  }

}

COLORREF WndForm::SetForeColor(COLORREF Value){
  if (mClientWindow)
    mClientWindow->SetForeColor(Value);
  return(WindowControl::SetForeColor(Value));
}

COLORREF WndForm::SetBackColor(COLORREF Value){
  if (mClientWindow)
  mClientWindow->SetBackColor(Value);
  return(WindowControl::SetBackColor(Value));
}

HFONT WndForm::SetFont(HFONT Value){
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


//-----------------------------------------------------------
// WndButton
//-----------------------------------------------------------

WndButton::WndButton(WindowControl *Parent, TCHAR *Name, TCHAR *Caption, int X, int Y, int Width, int Height, void(*Function)(WindowControl * Sender)):
      WindowControl(Parent, NULL /*Parent->GetHandle()*/, Name, X, Y, Width, Height){

  mOnClickNotify = Function;
  mDown = false;
  mDefault = false;
  mCanFocus = true;

  SetForeColor(GetOwner()->GetForeColor());
  SetBackColor(GetOwner()->GetBackColor());

  _tcscpy(mCaption, Caption);

};

void WndButton::Destroy(void){

  WindowControl::Destroy();

}


int WndButton::OnLButtonUp(WPARAM wParam, LPARAM lParam){
  POINT Pos;

  mDown = false;
  Paint(GetDeviceContext());
  ReleaseCapture();

  Pos.x = lParam & 0x0000ffff;
  Pos.y = (lParam >> 16)& 0x0000ffff;

  //POINTSTOPOINT(Pos, MAKEPOINTS(lParam));

  if (PtInRect(GetBoundRect(), Pos)){
    if (mOnClickNotify != NULL)
      (mOnClickNotify)(this);
  }

  return(1);
};

int WndButton::OnKeyDown(WPARAM wParam, LPARAM lParam){
  switch (wParam){
    case VK_RETURN:
    case VK_SPACE:
      if (!mDown){
        mDown = true;
        Paint(GetDeviceContext());
      }
    return(0);
  }
  return(1);
}

int WndButton::OnKeyUp(WPARAM wParam, LPARAM lParam){
  switch (wParam){
    case VK_RETURN:
    case VK_SPACE:
      if (mDown){
        mDown = false;
        Paint(GetDeviceContext());
        if (mOnClickNotify != NULL)
          (mOnClickNotify)(this);
      }
    return(0);
  }
  return(1);
}

int WndButton::OnLButtonDown(WPARAM wParam, LPARAM lParam){
  mDown = true;
  if (!GetFocused())
    SetFocus(GetHandle());
  else {
    InvalidateRect(GetHandle(), GetBoundRect(), false);
    UpdateWindow(GetHandle());
  }
  SetCapture(GetHandle());
  return(1);
};

int WndButton::OnLButtonDoubleClick(WPARAM wParam, LPARAM lParam){
  mDown = true;
  InvalidateRect(GetHandle(), GetBoundRect(), false);
  UpdateWindow(GetHandle());
  SetCapture(GetHandle());
  return(1);
};


void WndButton::Paint(HDC hDC){

  RECT r;
  SIZE tsize;
  POINT org;

  if (!GetVisible()) return;

  WindowControl::Paint(hDC);

  r.left = 2;
  r.top = 2;
  r.right = GetWidth()-2;
  r.bottom = GetHeight()-2;


  SetTextColor(hDC, GetForeColor());
  SetBkMode(hDC, TRANSPARENT);
  SelectObject(hDC, GetFont());
  GetTextExtentPoint(hDC, mCaption, _tcslen(mCaption), &tsize);

  org.x = (GetWidth() - tsize.cx)/2;
  org.y = (GetHeight() - tsize.cy)/2;

  if (mDown){
    DrawFrameControl(hDC, &r, DFC_BUTTON, DFCS_BUTTONPUSH | DFCS_PUSHED);
    org.x+=2;
    org.y+=2;
  }else{
    DrawFrameControl(hDC, &r, DFC_BUTTON, DFCS_BUTTONPUSH);
  }

  ExtTextOut(hDC, org.x, org.y,
    ETO_OPAQUE, NULL, mCaption, _tcslen(mCaption), NULL);

}



HBITMAP WndProperty::hBmpLeft32=NULL;
HBITMAP WndProperty::hBmpLeft16=NULL;
HBITMAP WndProperty::hBmpRight32=NULL;
HBITMAP WndProperty::hBmpRight16=NULL;

int     WndProperty::InstCount=0;


LRESULT CALLBACK WndPropertyEditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


WndProperty::WndProperty(WindowControl *Parent, TCHAR *Name, TCHAR *Caption, int X, int Y, int Width, int Height, int CaptionWidth, int (*DataChangeNotify)(WindowControl * Sender, int Mode, int Value), int MultiLine):
      WindowControl(Parent, NULL /*Parent->GetHandle()*/, Name, X, Y, Width, Height){

  mOnClickUpNotify = NULL;
  mOnClickDownNotify = NULL;
  mOnDataChangeNotify = DataChangeNotify;
  _tcscpy(mCaption, Caption);
  mhEdit = NULL;
  mDataField = NULL;

  mhValueFont = GetFont();
  mCaptionWidth = CaptionWidth;

  if (mCaptionWidth != 0){
    if (GetHeight() < 32)
      mBitmapSize = 16;
    else
      mBitmapSize = 32;
  } else {
    if ((GetHeight()/2) < 32)
      mBitmapSize = 16;
    else
      mBitmapSize = 32;
  }
  UpdateButtonData(mBitmapSize);

  if (MultiLine) {
    mhEdit = CreateWindow(TEXT("EDIT"), TEXT("\0"),
			  WS_BORDER | WS_VISIBLE | WS_CHILD
			  | ES_LEFT | ES_AUTOHSCROLL
			  | WS_CLIPCHILDREN
			  | WS_CLIPSIBLINGS
			  | ES_MULTILINE, // JMW added MULTILINE
			  mEditPos.x, mEditPos.y,
			  mEditSize.x, mEditSize.y,
			  GetHandle(), NULL, hInst, NULL);
  } else {
    mhEdit = CreateWindow(TEXT("EDIT"), TEXT("\0"),
			  WS_BORDER | WS_VISIBLE | WS_CHILD
			  | ES_LEFT | ES_AUTOHSCROLL
			  | WS_CLIPCHILDREN
			  | WS_CLIPSIBLINGS,
			  mEditPos.x, mEditPos.y,
			  mEditSize.x, mEditSize.y,
			  GetHandle(), NULL, hInst, NULL);
  }

  SetWindowLong(mhEdit, GWL_USERDATA, (long)this);
  mEditWindowProcedure = (WNDPROC)SetWindowLong(mhEdit, GWL_WNDPROC, (LONG) WndPropertyEditWndProc);

  SendMessage(mhEdit, WM_SETFONT,
		     (WPARAM)mhValueFont, MAKELPARAM(TRUE,0));


  mCanFocus = true;

  SetForeColor(GetOwner()->GetForeColor());
  SetBackColor(GetOwner()->GetBackColor());

  if (InstCount == 0){
    hBmpLeft32 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_DLGBUTTONLEFT32));
    hBmpRight32 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_DLGBUTTONRIGHT32));
    hBmpLeft16 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_DLGBUTTONLEFT16));
    hBmpRight16 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_DLGBUTTONRIGHT16));
  }
  InstCount++;

  mDownDown = false;
  mUpDown = false;

};


WndProperty::~WndProperty(void){
}

void WndProperty::Destroy(void){

  InstCount--;
  if (InstCount == 0){
    DeleteObject(hBmpLeft32);
    DeleteObject(hBmpRight32);
    DeleteObject(hBmpLeft16);
    DeleteObject(hBmpRight16);
  }

  if (mDataField != NULL){
    if (!mDataField->Unuse())
      delete(mDataField);
      mDataField = NULL;
  }

  SetWindowLong(mhEdit, GWL_WNDPROC, (LONG) mEditWindowProcedure);
  SetWindowLong(mhEdit, GWL_USERDATA, (long)0);

  DestroyWindow(mhEdit);

  WindowControl::Destroy();

}



void WndProperty::SetText(TCHAR *Value){
  SetWindowText(mhEdit, Value);
}


LRESULT CALLBACK WndPropertyEditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	WndProperty *w = (WndProperty *) GetWindowLong(hwnd, GWL_USERDATA);
	if (w)
		return (w->WndProcEditControl(hwnd, uMsg, wParam, lParam));
	else
		return (DefWindowProc(hwnd, uMsg, wParam, lParam));
}

HFONT WndProperty::SetFont(HFONT Value){
  HFONT res = GetFont();

  WindowControl::SetFont(Value);

  // todo, support value font

  if (res != Value){
    mhValueFont = Value;
    SendMessage(mhEdit, WM_SETFONT,
		     (WPARAM)mhValueFont, MAKELPARAM(TRUE,0));
  }
  return(res);
}

void WndProperty::UpdateButtonData(int Value){

  if (Value < 32)
    mBitmapSize = 16;
  else
    mBitmapSize = 32;

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

    SetWindowPos(mhEdit, 0, 0, 0,
      mEditSize.x, mEditSize.y,
      SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER
    );

    if (GetVisible()){
      InvalidateRect(GetHandle(), GetBoundRect(), false);
      UpdateWindow(GetHandle());
    }
  }
  return(res);
};


int WndProperty::WndProcEditControl(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

  switch (uMsg){

    case WM_KEYDOWN:
      if (!OnEditKeyDown(wParam, lParam))
        return(0);
    break;

    case WM_KEYUP:
    break;

    case WM_SETFOCUS:
      if (GetReadOnly()){
        SetFocus((HWND)wParam);
        return(0);
      } else {
        if ((HWND)wParam != GetHandle()){
          SetFocused(true, (HWND) wParam);
        }
      }
    break;

    case WM_KILLFOCUS:
      if ((HWND)wParam != GetHandle()){
        SetFocused(false, (HWND) wParam);
      }
    break;
  }

  return(CallWindowProc(mEditWindowProcedure, hwnd, uMsg, wParam, lParam));

}

bool WndProperty::SetReadOnly(bool Value){

  bool res = GetReadOnly();

  if (GetReadOnly() != Value){
    WindowControl::SetReadOnly(Value);

    SendMessage(mhEdit, EM_SETREADONLY, (WPARAM)(BOOL)Value, 0L);

  }

  return(res);
}

bool WndProperty::SetFocused(bool Value, HWND FromTo){

  TCHAR sTmp[STRINGVALUESIZE];

  if (Value && GetReadOnly()){  // keep focus on last control
    if (FromTo != mhEdit)
      SetFocus(FromTo);
    return(false);
  }

  if (!Value && (FromTo == mhEdit))
    Value = true;

  if (Value != GetFocused()){
    if (Value){
      if (mDataField != NULL){
        mDataField->GetData();
        SetWindowText(mhEdit, mDataField->GetAsString());
      }
    } else {
      if (mDataField != NULL){
        GetWindowText(mhEdit, sTmp, (sizeof(sTmp)/sizeof(TCHAR))-1);
        mDataField->SetAsString(sTmp);
        mDataField->SetData();
        SetWindowText(mhEdit, mDataField->GetAsDisplayString());
      }
    }
  }

  if (FromTo != mhEdit)
    WindowControl::SetFocused(Value, FromTo);
  if (Value){
    SetFocus(mhEdit);
    PostMessage(mhEdit, EM_SETSEL, 0, -1);
  }
  return(0);
}

int WndProperty::OnEditKeyDown(WPARAM wParam, LPARAM lParam){

  switch (wParam){
    case VK_RIGHT:
      IncValue();
    return(0);
    case VK_LEFT:
      DecValue();
    return(0);
  }

  return(1);
}

int WndProperty::OnKeyDown(WPARAM wParam, LPARAM lParam){

  switch (wParam){
    case VK_RIGHT:
      IncValue();
    return(0);
    case VK_LEFT:
      DecValue();
    return(0);
  }

  return(1);
};

int WndProperty::OnLButtonDown(WPARAM wParam, LPARAM lParam){

  POINT Pos;

  if (!GetFocused()){
    SetFocus(GetHandle());
    return(0);
  }

  Pos.x = lParam & 0x0000ffff;
  Pos.y = (lParam >> 16)& 0x0000ffff;
  //POINTSTOPOINT(Pos, MAKEPOINTS(lParam));

  if ((mDownDown = PtInRect(&mHitRectDown, Pos)) != 0){
    DecValue();
    InvalidateRect(GetHandle(), &mHitRectDown, false);
    UpdateWindow(GetHandle());
  }

  if ((mUpDown = PtInRect(&mHitRectUp, Pos)) != 0){
    IncValue();
    InvalidateRect(GetHandle(), &mHitRectUp, false);
    UpdateWindow(GetHandle());
  }

  SetCapture(GetHandle());

  return(0);
};

int WndProperty::OnLButtonDoubleClick(WPARAM wParam, LPARAM lParam){

  return(OnLButtonDown(wParam, lParam));

}

int WndProperty::OnLButtonUp(WPARAM wParam, LPARAM lParam){
  if (mDownDown){
    mDownDown = false;
    InvalidateRect(GetHandle(), &mHitRectDown, false);
    UpdateWindow(GetHandle());
  }
  if (mUpDown){
    mUpDown = false;
    InvalidateRect(GetHandle(), &mHitRectUp, false);
    UpdateWindow(GetHandle());
  }

  ReleaseCapture();

  return(0);
}


int WndProperty::IncValue(void){
  if (mDataField != NULL){
    mDataField->Inc();
    SetWindowText(mhEdit, mDataField->GetAsString());
  }
  return(0);
}

int WndProperty::DecValue(void){
  if (mDataField != NULL){
    mDataField->Dec();
    SetWindowText(mhEdit, mDataField->GetAsString());
  }
  return(0);
}


void WndProperty::Paint(HDC hDC){

  RECT r;
  SIZE tsize;
  POINT org;
  HBITMAP oldBmp;


  if (!GetVisible()) return;

  WindowControl::Paint(hDC);

  r.left = 0;
  r.top = 0;
  r.right = GetWidth();
  r.bottom = GetHeight();


  SetTextColor(hDC, GetForeColor());
  SetBkMode(hDC, TRANSPARENT);
  SelectObject(hDC, GetFont());
  GetTextExtentPoint(hDC, mCaption, _tcslen(mCaption), &tsize);

  if (mCaptionWidth==0){
    org.x = mEditPos.x;
    org.y = mEditPos.y - tsize.cy;
  } else {
    org.x = mCaptionWidth - mBitmapSize - (tsize.cx + 1);
    org.y = (GetHeight() - tsize.cy)/2;
  }

  if (org.x < 1)
    org.x = 1;

  ExtTextOut(hDC, org.x, org.y,
    ETO_OPAQUE, NULL, mCaption, _tcslen(mCaption), NULL);

  if (GetFocused() && !GetReadOnly()){

    if (mBitmapSize == 16)
      oldBmp = (HBITMAP)SelectObject(GetTempDeviceContext(), hBmpLeft16);
    else
      oldBmp = (HBITMAP)SelectObject(GetTempDeviceContext(), hBmpLeft32);

    if (mDownDown)
      BitBlt(hDC, mHitRectDown.left, mHitRectDown.top, mBitmapSize, mBitmapSize,
        GetTempDeviceContext(), mBitmapSize, 0, SRCCOPY);
    else
      BitBlt(hDC, mHitRectDown.left, mHitRectDown.top, mBitmapSize, mBitmapSize,
        GetTempDeviceContext(), 0, 0, SRCCOPY);

    if (mBitmapSize == 16)
      SelectObject(GetTempDeviceContext(), hBmpRight16);
    else
      SelectObject(GetTempDeviceContext(), hBmpRight32);

    if (mUpDown)
      BitBlt(hDC, mHitRectUp.left, mHitRectUp.top, mBitmapSize, mBitmapSize,
        GetTempDeviceContext(), mBitmapSize, 0, SRCCOPY);
    else
      BitBlt(hDC, mHitRectUp.left, mHitRectUp.top, mBitmapSize, mBitmapSize,
        GetTempDeviceContext(), 0, 0, SRCCOPY);

    SelectObject(GetTempDeviceContext(), oldBmp);

  }

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

    if (GetFocused())

      SetWindowText(mhEdit, mDataField->GetAsString());

    else

      SetWindowText(mhEdit, mDataField->GetAsDisplayString());

  }

  return(res);

}


void WndOwnerDrawFrame::Paint(HDC hDC){

  if (!GetVisible()) return;

  WndFrame::Paint(hDC);

  if (mOnPaintCallback != NULL)
    (mOnPaintCallback)(this, hDC);

}

void WndOwnerDrawFrame::Destroy(void){

  WndFrame::Destroy();

}


void WndFrame::Destroy(void){

  WindowControl::Destroy();

}


int WndFrame::OnKeyDown(WPARAM wParam, LPARAM lParam){
  if (mIsListItem && GetOwner()!=NULL){
    return(((WndListFrame*)GetOwner())->OnItemKeyDown(this, wParam, lParam));
  }
  return(1);
}

void WndFrame::Paint(HDC hDC){

  if (!GetVisible()) return;

  if (mIsListItem && GetOwner()!=NULL)
    ((WndListFrame*)GetOwner())->PrepareItemDraw();

  WindowControl::Paint(hDC);

  if (mCaption != 0){

    RECT rc;

    SetTextColor(hDC, GetForeColor());
    SetBkColor(hDC, GetBackColor());
    SetBkMode(hDC, TRANSPARENT);

    SelectObject(hDC, GetFont());

    CopyRect(&rc, GetBoundRect());
    InflateRect(&rc, -2, -2); // todo border width

//    h = rc.bottom - rc.top;

    DrawText(hDC, mCaption, _tcslen(mCaption), &rc,
      mCaptionStyle // | DT_CALCRECT
    );

    mLastDrawTextHeight = rc.bottom - rc.top;

  }

}

void WndFrame::SetCaption(TCHAR *Value){

  if (Value == NULL && mCaption[0] != '\0'){
    mCaption[0] ='\0';
    InvalidateRect(GetHandle(), GetBoundRect(), false);
    UpdateWindow(GetHandle());

    return;

  }

  if (_tcscmp(mCaption, Value) != 0){
    _tcscpy(mCaption, Value);  // todo size check
    InvalidateRect(GetHandle(), GetBoundRect(), false);
    UpdateWindow(GetHandle());

  }
}

UINT WndFrame::SetCaptionStyle(UINT Value){
  UINT res = mCaptionStyle;
  if (res != Value){
    mCaptionStyle = Value;

    InvalidateRect(GetHandle(), GetBoundRect(), false);
    UpdateWindow(GetHandle());

  }
  return(res);
}


WndListFrame::WndListFrame(WindowControl *Owner, TCHAR *Name, int X, int Y, int Width, int Height, void (*OnListCallback)(WindowControl * Sender, ListInfo_t *ListInfo)):
   WndFrame(Owner, Name, X, Y, Width, Height)
{

  mListInfo.ItemIndex = 0;
  mListInfo.DrawIndex = 0;
  mListInfo.ItemInPageCount = 0;
  mListInfo.TopIndex = 0;
  mListInfo.BottomIndex = 0;
  mListInfo.SelectedIndex = 0;
  mListInfo.ItemCount = 0;
  mListInfo.ItemInViewCount = 0;

  mCaption[0] = '\0';
  mOnListCallback = OnListCallback;
  SetForeColor(GetOwner()->GetForeColor());
  SetBackColor(GetOwner()->GetBackColor());

};

void WndListFrame::Destroy(void){

  WndFrame::Destroy();

}


void WndListFrame::Paint(HDC hDC){
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

  WndFrame::Paint(hDC);

  if (mClientCount > 0){

    HDC HdcTemp = CreateCompatibleDC(hDC);
    HBITMAP BmpMem = CreateCompatibleBitmap(hDC,
               mClients[0]->GetWidth(),
               mClients[0]->GetHeight());

    SelectObject(HdcTemp, BmpMem);

    for (i=0; i<mListInfo.ItemInViewCount; i++){

      if (mOnListCallback != NULL){
        mListInfo.DrawIndex = mListInfo.TopIndex + i;
        mOnListCallback(this, &mListInfo);
      }

      mClients[0]->PaintSelector(true);
      mClients[0]->Paint(HdcTemp);
      mClients[0]->PaintSelector(false);

      BitBlt(hDC,
          mClients[0]->GetLeft(), i*mClients[0]->GetHeight(),
          mClients[0]->GetWidth(), mClients[0]->GetHeight(),
          HdcTemp,
          0,0,
          SRCCOPY
        );

    }

    mListInfo.DrawIndex = mListInfo.ItemIndex;

    DeleteObject(BmpMem);
    DeleteDC(HdcTemp);

  }
}

int WndListFrame::OnItemKeyDown(WindowControl *Swnder, WPARAM wParam, LPARAM lParam){
  switch (wParam){
    case VK_DOWN:
      mListInfo.ItemIndex++;
      if (mListInfo.ItemIndex >= mListInfo.BottomIndex){
        mListInfo.ItemIndex = mListInfo.BottomIndex;
        return(1);
      }

      mListInfo.DrawIndex = mListInfo.ItemIndex;
      mOnListCallback(this, &mListInfo);
      mClients[0]->SetTop(mClients[0]->GetHeight() * (mListInfo.ItemIndex-mListInfo.TopIndex));
      mClients[0]->Redraw();

    return(0);
    case VK_UP:
      mListInfo.ItemIndex--;
      if (mListInfo.ItemIndex < 0){
        mListInfo.ItemIndex = 0;
        return(1);
      }

      mListInfo.DrawIndex = mListInfo.ItemIndex;
      mOnListCallback(this, &mListInfo);
      mClients[0]->SetTop(mClients[0]->GetHeight() * (mListInfo.ItemIndex-mListInfo.TopIndex));
      mClients[0]->Redraw();

    return(0);
  }
  return(1);

}

void WndListFrame::ResetList(void){

  mListInfo.ItemIndex = 0;
  mListInfo.DrawIndex = 0;
  mListInfo.ItemInPageCount = ((GetWidth()+mClients[0]->GetHeight()-1)/mClients[0]->GetHeight())-1;
  mListInfo.TopIndex = 0;
  mListInfo.BottomIndex = 0;
  mListInfo.SelectedIndex = 0;
  mListInfo.ItemCount = 0;
  mListInfo.ItemInViewCount = (GetWidth()+mClients[0]->GetHeight()-1)/mClients[0]->GetHeight();

  if (mOnListCallback != NULL){
    mListInfo.DrawIndex = -1;                               // -1 -> initialize data
    mOnListCallback(this, &mListInfo);
    mListInfo.DrawIndex = 0;                                // setup data for first ite,
    mOnListCallback(this, &mListInfo);
  }

  if (mListInfo.BottomIndex  == 0){                         // calc bounds
    mListInfo.BottomIndex  = mListInfo.ItemCount;
    if (mListInfo.BottomIndex > mListInfo.ItemInViewCount){
      mListInfo.BottomIndex = mListInfo.ItemInViewCount;
    }
  }

  mClients[0]->SetTop(0);                                   // move item windoe to the top
  mClients[0]->Redraw();

}

int WndListFrame::PrepareItemDraw(void){
  mOnListCallback(this, &mListInfo);
  return(1);
}

