/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#if !defined(__WINDOWSCONTROL_H)
#define __WINDOWSCONTROL_H

#include "Screen/BitmapCanvas.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/EditWindow.hpp"

#include <malloc.h>
#include "Units.h"

#include <tchar.h>
#define IsEmptyString(x)        ((x==NULL) || (x[0]=='\0'))

#define BORDERTOP    (1<<bkTop)
#define BORDERRIGHT  (1<<bkRight)
#define BORDERBOTTOM (1<<bkBottom)
#define BORDERLEFT   (1<<bkLeft)

#define clBlack   RGB(0x00,0x00,0x00)
#define clMaroon  RGB(0x00,0x00,0x80)
#define clGreen   RGB(0x00,0x80,0x00)
#define clOlive   RGB(0x00,0x80,0x80)
#define clNavy    RGB(0x80,0x00,0x00)
#define clPurple  RGB(0x80,0x00,0x80)
#define clTeal    RGB(0x80,0x80,0x00)
#define clGray    RGB(0x80,0x80,0x80)
#define clSilver  RGB(0xC0,0xC0,0xC0)
#define clRed     RGB(0xFF,0x00,0xFF)
#define clLime    RGB(0x00,0xFF,0x00)
#define clYellow  RGB(0x00,0xFF,0xFF)
#define clBlue    RGB(0xFF,0x00,0x00)
#define clFuchsia RGB(0xFF,0x00,0xFF)
#define clAqua    RGB(0xFF,0xFF,0x00)
#define clLtGray  RGB(0xC0,0xC0,0xC0)
#define clDkGray  RGB(0x80,0x80,0x80)
#define clWhite   RGB(0xFF,0xFF,0xFF)
#define clNone    0x1FFFFFFF
#define clDefault 0x20000000

class DataField;

typedef enum{
  bkNone,
  bkTop,
  bkRight,
  bkBottom,
  bkLeft
}BorderKind_t;

class WindowControl {
 public:
    typedef void (*OnHelpCallback_t)(WindowControl * Sender);

  private:

    int mX;
    int mY;
    int mWidth;
    int mHeight;

    WindowControl *mOwner;
    WindowControl *mTopOwner;
    BitmapCanvas mHdcTemp;
    HBITMAP mBmpMem;
    int  mBorderKind;
    COLORREF mColorBack;
    COLORREF mColorFore;
    Brush mhBrushBk;
    Pen mhPenBorder;
    Pen mhPenSelector;
    RECT mBoundRect;
    const Font *mhFont;
    TCHAR mName[64];
    TCHAR *mHelpText;

    OnHelpCallback_t mOnHelpCallback;

    int mTag;
    bool mReadOnly;
    bool mHasFocus;

    int  mBorderSize;
    bool mVisible;

    WindowControl *mActiveClient;

    static int InstCount;
    static Brush hBrushDefaultBk;
    static Pen hPenDefaultBorder;
    static Pen hPenDefaultSelector;

  protected:

    ContainerWindow widget;
    bool mCanFocus;
    TCHAR mCaption[254];
    bool mDontPaintSelector;

    WindowControl *mClients[50];
    int mClientCount;

    virtual void PaintSelector(Canvas &canvas);
    virtual WindowControl *SetOwner(WindowControl *Value);
    void UpdatePosSize(void);
    bool HasFocus(void) { return mHasFocus; };

  public:
    TCHAR* GetCaption(void) { return mCaption; };
    int WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    virtual void AddClient(WindowControl *Client);

    virtual void Paint(Canvas &canvas);

    virtual int OnHelp();

    virtual int OnLButtonDoubleClick(WPARAM wParam, LPARAM lParam){
		(void)wParam; (void)lParam;
      return(1);
    };
    virtual int OnLButtonDown(WPARAM wParam, LPARAM lParam){
		(void)wParam; (void)lParam;
      return(1);
    };
    virtual int OnLButtonUp(WPARAM wParam, LPARAM lParam){
		(void)wParam; (void)lParam;
      return(1);
    };
    virtual int OnKeyDown(WPARAM wParam, LPARAM lParam){
		(void)wParam; (void)lParam;
      return(1);
    };
    virtual int OnKeyUp(WPARAM wParam, LPARAM lParam){
		(void)wParam; (void)lParam;
      return(1);
    };
    virtual int OnCommand(WPARAM wParam, LPARAM lParam){
		(void)wParam; (void)lParam;
      return(1);
    };
    virtual int OnMouseMove(WPARAM wParam, LPARAM lParam){
		(void)wParam; (void)lParam;
      return(1);
    };
    virtual int OnUnhandledMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
		(void)hwnd; (void)uMsg; (void)wParam; (void)lParam;
      return(1);
    };
    virtual void Close(void){
      SetVisible(false);
    };
    virtual void Show(void){
      SetVisible(true);
    };

    void SetOnHelpCallback(void(*Function)(WindowControl * Sender)){
      mOnHelpCallback = Function;
    }

    RECT *GetBoundRect(void){return(&mBoundRect);};

    int GetWidth(void){return(mWidth);};
    int GetHeight(void){return(mHeight);};

    virtual bool SetFocused(bool Value, HWND FromTo);
    bool GetFocused(void);
    WindowControl *GetCanFocus(void);
    bool SetCanFocus(bool Value);

    bool GetReadOnly(void){return(mReadOnly);};
    bool SetReadOnly(bool Value);

    bool SetVisible(bool Value);
    bool GetVisible(void);

    int  GetBorderKind(void);
    int  SetBorderKind(int Value);

    const Font *GetFont(void) { return(mhFont); };
    virtual const Font *SetFont(const Font &font);

    const Font *SetFont(const Font *font) {
      return SetFont(*font);
    }

    virtual COLORREF SetForeColor(COLORREF Value);
    COLORREF GetForeColor(void){return(mColorFore);};

    virtual COLORREF SetBackColor(COLORREF Value);
    COLORREF GetBackColor(void){return(mColorBack);};

    Brush &GetBackBrush(void) {
      return mhBrushBk.defined()
        ? mhBrushBk
        : hBrushDefaultBk;
    }
    Pen &GetBorderPen(void) {
      return mhPenBorder.defined()
        ? mhPenBorder
        : hPenDefaultBorder;
    }
    Pen &GetSelectorPen(void) {
      return mhPenSelector.defined()
        ? mhPenSelector
        : hPenDefaultSelector;
    }

    virtual void SetCaption(const TCHAR *Value);
    void SetHelpText(const TCHAR *Value);

    HWND GetHandle(void) { return widget; }
    ContainerWindow &GetWidget(void) { return widget; }
    virtual ContainerWindow &GetClientAreaWidget(void) { return widget; }
    Canvas &GetCanvas(void) { return widget.get_canvas(); }
    BitmapCanvas &GetTempDeviceContext(void) { return mHdcTemp; }
    WindowControl *GetOwner(void){return(mOwner);};

    int GetTag(void){return(mTag);};
    int SetTag(int Value){mTag = Value; return(mTag);};

    void SetTop(int Value);
    void SetLeft(int Value);
    void SetWidth(int Value);
    void SetHeight(int Value);

    int GetTop(void){return(mY);};
    int GetLeft(void){return(mX);};

    WindowControl *FocusNext(WindowControl *Sender);
    WindowControl *FocusPrev(WindowControl *Sender);

    WindowControl(WindowControl *Owner, ContainerWindow *Parent,
                  const TCHAR *Name, int X, int Y, int Width, int Height,
                  bool Visible=true);
    virtual ~WindowControl(void);

    virtual void Destroy(void);

    virtual void Redraw(void);

    void PaintSelector(bool Value){mDontPaintSelector = Value;};

    WindowControl *FindByName(const TCHAR *Name);

    void FilterAdvanced(bool advanced);

};

class WndFrame:public WindowControl{

  public:

    WndFrame(WindowControl *Owner, const TCHAR *Name,
             int X, int Y, int Width, int Height):
      WindowControl(Owner, NULL, Name, X, Y, Width, Height)
    {
      mIsListItem = false;

      SetForeColor(GetOwner()->GetForeColor());
      SetBackColor(GetOwner()->GetBackColor());
      mCaptionStyle = DT_EXPANDTABS
      | DT_LEFT
      | DT_NOCLIP
      | DT_WORDBREAK;
    };

    virtual void Destroy(void);

    virtual int OnMouseMove(WPARAM wParam, LPARAM lParam){
		(void)wParam; (void)lParam;
      return(1);
    };

    void SetCaption(const TCHAR *Value);

    UINT GetCaptionStyle(void){return(mCaptionStyle);};
    UINT SetCaptionStyle(UINT Value);

    unsigned GetTextHeight();

    void SetIsListItem(bool Value){mIsListItem = Value;};


    int OnLButtonDown(WPARAM wParam, LPARAM lParam);
    int OnLButtonUp(WPARAM wParam, LPARAM lParam);

  protected:

    int OnKeyDown(WPARAM wParam, LPARAM lParam);

    bool mIsListItem;

    UINT mCaptionStyle;

    void Paint(Canvas &canvas);

};

class WndListFrame:public WndFrame{

  public:

    typedef struct{
      int TopIndex;
      int BottomIndex;
      int ItemIndex;
      int DrawIndex;
//      int SelectedIndex;
      int ScrollIndex;
      int ItemCount;
      int ItemInViewCount;
      int ItemInPageCount;
    }ListInfo_t;

    typedef void (*OnListCallback_t)(WindowControl * Sender, ListInfo_t *ListInfo);

    WndListFrame(WindowControl *Owner, TCHAR *Name, int X, int Y,
                 int Width, int Height,
                 void (*OnListCallback)(WindowControl * Sender,
                                        ListInfo_t *ListInfo));

    virtual void Destroy(void);

    int OnMouseMove(WPARAM wParam, LPARAM lParam);
    int OnItemKeyDown(WindowControl *Sender, WPARAM wParam, LPARAM lParam);
    int PrepareItemDraw(void);
    void ResetList(void);
    void SetEnterCallback(void (*OnListCallback)(WindowControl * Sender, ListInfo_t *ListInfo));
    void RedrawScrolled(bool all);
    void DrawScrollBar(Canvas &canvas);
    int RecalculateIndices(bool bigscroll);
    void Redraw(void);
    int GetItemIndex(void){return(mListInfo.ItemIndex);}
    void SetItemIndex(int iValue);
    void SelectItemFromScreen(int xPos, int yPos, RECT *rect);
    int GetScrollBarHeight (void);
    int GetScrollIndexFromScrollBarTop(int iScrollBarTop);
    int GetScrollBarTopFromScrollIndex();

  protected:
#define SCROLLBARWIDTH_INITIAL 32
    int ScrollbarTop;
    int ScrollbarWidth;

    int OnLButtonDown(WPARAM wParam, LPARAM lParam);
    int OnLButtonUp(WPARAM wParam, LPARAM lParam);

    OnListCallback_t mOnListCallback;
    OnListCallback_t mOnListEnterCallback;
    ListInfo_t mListInfo;
    void Paint(Canvas &canvas);
	  RECT rcScrollBarButton;
	  RECT rcScrollBar;
    int mMouseScrollBarYOffset; // where in the scrollbar button was mouse down at
    bool mMouseDown;
    int LastMouseMoveTime;
};

class WndOwnerDrawFrame:public WndFrame{

  public:

    typedef void (*OnPaintCallback_t)(WindowControl *Sender, Canvas &canvas);

    WndOwnerDrawFrame(WindowControl *Owner, TCHAR *Name, int X, int Y,
                      int Width, int Height,
                      OnPaintCallback_t OnPaintCallback):
      WndFrame(Owner, Name, X, Y, Width, Height)
    {
      mCaption[0] = '\0';
      mOnPaintCallback = OnPaintCallback;
      SetForeColor(GetOwner()->GetForeColor());
      SetBackColor(GetOwner()->GetBackColor());

    };

    virtual void Destroy(void);

    void SetOnPaintNotify(OnPaintCallback_t OnPaintCallback){
      mOnPaintCallback = OnPaintCallback;
    }

  protected:

    OnPaintCallback_t mOnPaintCallback;
    void Paint(Canvas &canvas);

};

extern WindowControl *ActiveControl;
extern WindowControl *LastFocusControl;

#define mrOK             2
#define mrCancle         3

class WndForm:public WindowControl{

  protected:

    static ACCEL  mAccel[];

    int mModalResult;
    HACCEL mhAccelTable;
    COLORREF mColorTitle;
    HBRUSH mhBrushTitle;
    const Font *mhTitleFont;
    WindowControl *mClientWindow;
    RECT mClientRect;
    RECT mTitleRect;

    int (*mOnTimerNotify)(WindowControl * Sender);
    int (*mOnKeyDownNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam);
    int (*mOnKeyUpNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam);
    int (*mOnLButtonUpNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam);
    int (*mOnUserMsgNotify)(WindowControl * Sender, MSG *msg);


    int OnUnhandledMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void Paint(Canvas &canvas);
    int cbTimerID;

  public:

    WndForm(ContainerWindow *Parent,
            const TCHAR *Name, const TCHAR *Caption,
            int X, int Y, int Width, int Height);
    ~WndForm(void);
    virtual void Destroy(void);

    bool bLButtonDown; //RLD
    ContainerWindow &GetClientAreaWidget(void);
    void AddClient(WindowControl *Client);

    virtual bool SetFocused(bool Value, HWND FromTo);


    int OnLButtonUp(WPARAM wParam, LPARAM lParam){
		(void)wParam; (void)lParam;
      return(0);
    };

    void Close(void){
      WindowControl::Close();
      mModalResult = mrCancle;
    }

    DWORD enterTime;

    void SetToForeground(void);

    int GetModalResult(void){return(mModalResult);};
    int SetModalResult(int Value){mModalResult = Value;return(Value);};

    const Font *SetTitleFont(const Font &font);

    int ShowModal(bool bEnableMap);
    int ShowModal(void);
    void Show(void);

    void SetCaption(const TCHAR *Value);

    virtual int OnCommand(WPARAM wParam, LPARAM lParam);

    COLORREF SetForeColor(COLORREF Value);
    COLORREF SetBackColor(COLORREF Value);
    const Font *SetFont(const Font &Value);

    void SetKeyDownNotify(int (*KeyDownNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam));
    void SetKeyUpNotify(int (*KeyUpNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam));
    void SetLButtonUpNotify(int (*LButtonUpNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam));

    void SetTimerNotify(int (*OnTimerNotify)(WindowControl * Sender));

    void SetUserMsgNotify(int (*OnUserMsgNotify)(WindowControl * Sender, MSG *msg));
private:
    static DWORD timeAnyOpenClose; // when any dlg opens or child closes

};

class WndButton:public WindowControl{

  private:

    void Paint(Canvas &canvas);
    bool mDown;
    bool mDefault;
    int mLastDrawTextHeight;
    void (*mOnClickNotify)(WindowControl * Sender);

  public:

    typedef void (*ClickNotifyCallback_t)(WindowControl * Sender);

    WndButton(WindowControl *Parent, const TCHAR *Name, const TCHAR *Caption, int X, int Y, int Width, int Height, void(*Function)(WindowControl * Sender) = NULL);
    virtual void Destroy(void);

    int OnLButtonUp(WPARAM wParam, LPARAM lParam);
    int OnLButtonDown(WPARAM wParam, LPARAM lParam);
    int OnLButtonDoubleClick(WPARAM wParam, LPARAM lParam);

    int OnKeyDown(WPARAM wParam, LPARAM lParam);
    int OnKeyUp(WPARAM wParam, LPARAM lParam);

    void SetOnClickNotify(void(*Function)(WindowControl * Sender)){
      mOnClickNotify = Function;
    }


};



#define STRINGVALUESIZE         128

class WndProperty:public WindowControl{

  private:

    static HBITMAP hBmpLeft32;
    static HBITMAP hBmpRight32;
    static int InstCount;

    EditWidget edit;
    POINT mEditSize;
    POINT mEditPos;
    const Font *mhCaptionFont;
    const Font *mhValueFont;
    int  mBitmapSize;
    int  mCaptionWidth;
    RECT mHitRectUp;
    RECT mHitRectDown;
    bool mDownDown;
    bool mUpDown;

    void Paint(Canvas &canvas);
    void (*mOnClickUpNotify)(WindowControl * Sender);
    void (*mOnClickDownNotify)(WindowControl * Sender);

    int (*mOnDataChangeNotify)(WindowControl * Sender, int Mode, int Value);

    int CallSpecial(void);
    int IncValue(void);
    int DecValue(void);
    WNDPROC mEditWindowProcedure;

    DataField *mDataField;

    void UpdateButtonData(int Value);
    bool mDialogStyle;

  public:


    typedef int (*DataChangeCallback_t)(WindowControl * Sender, int Mode, int Value);

    WndProperty(WindowControl *Parent, TCHAR *Name, TCHAR *Caption, int X, int Y, int Width, int Height, int CaptionWidth, int (*DataChangeNotify)(WindowControl * Sender, int Mode, int Value), int MultiLine=false);
    ~WndProperty(void);
    virtual void Destroy(void);

    int WndProcEditControl(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    bool SetFocused(bool Value, HWND FromTo);

    bool SetReadOnly(bool Value);

    void RefreshDisplay(void);

    const Font *SetFont(const Font &font);

    int OnKeyDown(WPARAM wParam, LPARAM lParam);
    int OnEditKeyDown(WPARAM wParam, LPARAM lParam);
    int OnLButtonDown(WPARAM wParam, LPARAM lParam);
    int OnLButtonUp(WPARAM wParam, LPARAM lParam);
    int OnLButtonDoubleClick(WPARAM wParam, LPARAM lParam);

//    int GetAsInteger(void){return(mValue);};
//    int SetAsInteger(int Value);

    DataField *GetDataField(void){return(mDataField);};
    DataField *SetDataField(DataField *Value);
    void SetText(const TCHAR *Value);
    int SetButtonSize(int Value);

};

#ifndef ALTAIRSYNC

typedef void (*webpt2Event)(const TCHAR *);

class WndEventButton:public WndButton {
 public:
  WndEventButton(WindowControl *Parent, const TCHAR *Name, const TCHAR *Caption,
		 int X, int Y, int Width, int Height,
		 const TCHAR *ename,
		 const TCHAR *eparameters);
  ~WndEventButton();
 public:
  void CallEvent(void);
 private:
  webpt2Event inputEvent;
  TCHAR *parameters;
};


#endif

#endif

