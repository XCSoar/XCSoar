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

#if !defined(__WINDOWSCONTROL_H)
#define __WINDOWSCONTROL_H

#include "units.h"
#include "xcsoar.h"

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

#define FORMATSIZE 32
#define UNITSIZE 10

class DataField{

  public:

    typedef enum{
     daGet,
     daPut,
     daChange,
     daInc,
     daDec
    }DataAccessKind_t;

    typedef void (*DataAccessCallback_t)(DataField * Sender, DataAccessKind_t Mode);

    DataField(TCHAR *EditFormat, TCHAR *DisplayFormat, void(*OnDataAccess)(DataField *Sender, DataAccessKind_t Mode)=NULL);
    virtual ~DataField(void){};

  virtual void Inc(void);
  virtual void Dec(void);

  virtual void GetData(void);
  virtual void SetData(void);

  virtual bool GetAsBoolean(void){return(false);};
  virtual int GetAsInteger(void){return(0);};
  virtual double GetAsFloat(void){return(0);};
  virtual TCHAR *GetAsString(void){return(NULL);};
  virtual TCHAR *GetAsDisplayString(void){return(NULL);};

  virtual bool SetAsBoolean(bool Value){ (void)Value;
	  return(false);};
	  virtual int SetAsInteger(int Value){ (void)Value;
	  return(0);};
	  virtual double SetAsFloat(double Value){ (void) Value;
	  return(NULL);};
  virtual TCHAR *SetAsString(TCHAR *Value){(void)Value; return(NULL);};

  virtual void Set(bool Value){ (void)Value; };
  virtual void Set(int Value){ (void)Value;};
  virtual void Set(double Value){ (void)Value; };
  virtual void Set(TCHAR *Value){ (void)Value; };

  virtual int SetMin(int Value){(void)Value; return(0);};
  virtual double SetMin(double Value){(void)Value; return(false);};

  virtual int SetMax(int Value){(void)Value; return(0);};
  virtual double SetMax(double Value){(void)Value; return(0);};
  void SetUnits(const TCHAR *text) { _tcscpy(mUnits, text); }

  void Use(void){
    mUsageCounter++;
  }

  int Unuse(void){
    mUsageCounter--;
    return(mUsageCounter);
  }

  void SetDisplayFormat(TCHAR *Value);

  protected:
    void (*mOnDataAccess)(DataField *Sender, DataAccessKind_t Mode);
    TCHAR mEditFormat[FORMATSIZE+1];
    TCHAR mDisplayFormat[FORMATSIZE+1];
    TCHAR mUnits[UNITSIZE+1];

  private:

    int mUsageCounter;

};

class DataFieldBoolean:public DataField{

  private:
    bool mValue;
    TCHAR mTextTrue[FORMATSIZE+1];
    TCHAR mTextFalse[FORMATSIZE+1];

  public:
    DataFieldBoolean(TCHAR *EditFormat, TCHAR *DisplayFormat, int Default, TCHAR *TextTrue, TCHAR *TextFalse, void(*OnDataAccess)(DataField *Sender, DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
		  if (Default) {mValue=true;} else {mValue=false;}
      _tcscpy(mTextTrue, TextTrue);
      _tcscpy(mTextFalse, TextFalse);

     (mOnDataAccess)(this, daGet);

    };

  void Inc(void);
  void Dec(void);

  bool GetAsBoolean(void);
  int GetAsInteger(void);
  double GetAsFloat(void);
  TCHAR *GetAsString(void);
  TCHAR *GetAsDisplayString(void){
    return(GetAsString());
  };

  virtual void Set(int Value){ 
    if (Value>0) 
      Set(true); 
    else 
      Set(false);
  };

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(bool Value);
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  bool SetAsBoolean(bool Value);
  int SetAsInteger(int Value);
  double SetAsFloat(double Value);
  TCHAR *SetAsString(TCHAR *Value);

};

#define DFE_MAX_ENUMS 100

class DataFieldEnum: public DataField {

  private:
    unsigned int nEnums;
    unsigned int mValue;
    TCHAR *mTextEnum[DFE_MAX_ENUMS];

  public:
    DataFieldEnum(TCHAR *EditFormat, 
		  TCHAR *DisplayFormat, 
		  int Default, 
		  void(*OnDataAccess)(DataField *Sender, 
				      DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
      if (Default>=0) 
	{ mValue = Default; } 
      else 
	{mValue = 0;}
      nEnums = 0;
      if (mOnDataAccess) {
	(mOnDataAccess)(this, daGet);
      }
    };
      ~DataFieldEnum();

  void Inc(void);
  void Dec(void);

  void addEnumText(TCHAR *Text);

  int GetAsInteger(void);
  TCHAR *GetAsString(void);
  TCHAR *GetAsDisplayString(void){
    return(GetAsString());
  };

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(int Value);
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  int SetAsInteger(int Value);

};

#define DFE_MAX_FILES 100

typedef struct {
  TCHAR *mTextFile;
  TCHAR *mTextPathFile;
} DataFieldFileReaderEntry;

class DataFieldFileReader: public DataField {

 private:
  unsigned int nFiles;
  unsigned int mValue;
  DataFieldFileReaderEntry fields[DFE_MAX_FILES];

  public:
  DataFieldFileReader(TCHAR *EditFormat, 
		      TCHAR *DisplayFormat, 
		      void(*OnDataAccess)(DataField *Sender, DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
      mValue = 0;
      fields[0].mTextFile= NULL;
      fields[0].mTextPathFile= NULL; // first entry always exists and is blank
      nFiles = 1;

      (mOnDataAccess)(this, daGet);
      
    };
    ~DataFieldFileReader() {
	for (unsigned int i=1; i<nFiles; i++) {
	  if (fields[i].mTextFile) {
	    free(fields[i].mTextFile);
	    fields[i].mTextFile= NULL;
	  }
	  if (fields[i].mTextPathFile) {
	    free(fields[i].mTextPathFile);
	    fields[i].mTextPathFile= NULL;
	  }
	}
	nFiles = 1;
    }

  void Inc(void);
  void Dec(void);

  void addFile(TCHAR *fname, TCHAR *fpname);
  bool checkFilter(const TCHAR *fname, const TCHAR* filter);
  int GetNumFiles(void);

  int GetAsInteger(void);
  TCHAR *GetAsString(void);
  TCHAR *GetAsDisplayString(void);
  void Lookup(TCHAR* text);
  TCHAR* GetPathFile(void);

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(int Value);
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  int SetAsInteger(int Value);

  void Sort();
  void ScanDirectoryTop(const TCHAR *filter);

 protected:
  BOOL ScanFiles(const TCHAR *pattern, const TCHAR *filter);
  BOOL ScanDirectories(const TCHAR *pattern, const TCHAR *filter);

};



#define OUTBUFFERSIZE 128

class DataFieldInteger:public DataField{

  private:
    int mValue;
    int mMin;
    int mMax;
    int mStep;
    DWORD mTmLastStep;
    int mSpeedup;
    TCHAR mOutBuf[OUTBUFFERSIZE+1];

  protected:
    int SpeedUp(bool keyup);

  public:
    DataFieldInteger(TCHAR *EditFormat, TCHAR *DisplayFormat, int Min, int Max, int Default, int Step, void(*OnDataAccess)(DataField *Sender, DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
      mMin = Min;
      mMax = Max;
      mValue = Default;
      mStep = Step;

     (mOnDataAccess)(this, daGet);

    };

  void Inc(void);
  void Dec(void);

  bool GetAsBoolean(void);
  int GetAsInteger(void);
  double GetAsFloat(void);
  TCHAR *GetAsString(void);
  TCHAR *GetAsDisplayString(void);

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(int Value);
  int SetMin(int Value){mMin=Value; return(mMin);};
  int SetMax(int Value){mMax=Value; return(mMax);};
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  bool SetAsBoolean(bool Value);
  int SetAsInteger(int Value);
  double SetAsFloat(double Value);
  TCHAR *SetAsString(TCHAR *Value);

};

class DataFieldFloat:public DataField{

  private:
    double mValue;
    double mMin;
    double mMax;
    double mStep;
    DWORD mTmLastStep;
    int mSpeedup;
    TCHAR mOutBuf[OUTBUFFERSIZE+1];

  protected:
    double SpeedUp(bool keyup);


  public:
    DataFieldFloat(TCHAR *EditFormat, TCHAR *DisplayFormat, double Min, double Max, double Default, double Step, void(*OnDataAccess)(DataField *Sender, DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
      mMin = Min;
      mMax = Max;
      mValue = Default;
      mStep = Step;

     (mOnDataAccess)(this, daGet);

    };

  void Inc(void);
  void Dec(void);

  bool GetAsBoolean(void);
  int GetAsInteger(void);
  double GetAsFloat(void);
  TCHAR *GetAsString(void);
  TCHAR *GetAsDisplayString(void);

  virtual void Set(int Value){ Set((double)Value); };

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(double Value);
  double SetMin(double Value);
  double SetMax(double Value);
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  bool SetAsBoolean(bool Value);
  int SetAsInteger(int Value);
  double SetAsFloat(double Value);
  TCHAR *SetAsString(TCHAR *Value);

};

#define EDITSTRINGSIZE 32

class DataFieldString:public DataField{

  private:
    TCHAR mValue[EDITSTRINGSIZE];

  public:
    DataFieldString(TCHAR *EditFormat, TCHAR *DisplayFormat, TCHAR *Default, void(*OnDataAccess)(DataField *Sender, DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
      _tcscpy(mValue, Default);
    };

  TCHAR *SetAsString(TCHAR *Value);
  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(TCHAR *Value);
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  TCHAR *GetAsString(void);
  TCHAR *GetAsDisplayString(void);

};

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

    HWND mParent;
    WindowControl *mOwner;
    WindowControl *mTopOwner;
    HDC  mHdc;
    HDC  mHdcTemp;
    HBITMAP mBmpMem;
    int  mBorderKind;
    COLORREF mColorBack;
    COLORREF mColorFore;
    HBRUSH mhBrushBk;
    HPEN mhPenBorder;
    HPEN mhPenSelector;
    RECT mBoundRect;
    HFONT mhFont;
    TCHAR mName[64];
    TCHAR *mHelpText;

    OnHelpCallback_t mOnHelpCallback;

    int mTag;
    bool mReadOnly;
    bool mHasFocus;

    int  mBorderSize;
    bool mVisible;

    WindowControl *mActiveClient;

    LONG mSavWndProcedure;

    static int InstCount;
    static HBRUSH hBrushDefaultBk;
    static HPEN hPenDefaultBorder;
    static HPEN hPenDefaultSelector;

  protected:

    HWND mHWnd;
    bool mCanFocus;
    TCHAR mCaption[254];
    bool mDontPaintSelector;

    WindowControl *mClients[25];
    int mClientCount;

    virtual void PaintSelector(HDC hDC);
    virtual WindowControl *SetOwner(WindowControl *Value);
    void UpdatePosSize(void);
    bool HasFocus(void) { return mHasFocus; };

  public:

    int WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    virtual void AddClient(WindowControl *Client);

    virtual void Paint(HDC hDC);

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

    HFONT GetFont(void){return(mhFont);};
    virtual HFONT SetFont(HFONT Value);

    virtual COLORREF SetForeColor(COLORREF Value);
    COLORREF GetForeColor(void){return(mColorFore);};

    virtual COLORREF SetBackColor(COLORREF Value);
    COLORREF GetBackColor(void){return(mColorBack);};

    HBRUSH   GetBackBrush(void){return(mhBrushBk);};
    HPEN     GetBorderPen(void){return(mhPenBorder);};
    HPEN     GetSelectorPen(void){return(mhPenSelector);};

    virtual void SetCaption(TCHAR *Value);
    void SetHelpText(const TCHAR *Value);

    HWND GetHandle(void){return(mHWnd);};
    virtual HWND GetClientAreaHandle(void){return(mHWnd);};
    HWND GetParent(void){return(mParent);};
    HDC  GetDeviceContext(void){return(mHdc);};
    HDC  GetTempDeviceContext(void){return(mHdcTemp);};
    WindowControl *GetOwner(void){return(mOwner);};

    void SetParentHandle(HWND hwnd);

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

    WindowControl(WindowControl *Owner, HWND Parent, TCHAR *Name, int X, int Y, int Width, int Height, bool Visible=true);
    virtual ~WindowControl(void);

    virtual void Destroy(void);

    virtual void Redraw(void);

    void PaintSelector(bool Value){mDontPaintSelector = Value;};

    WindowControl *FindByName(TCHAR *Name);

    void FilterAdvanced(bool advanced);

};

class WndFrame:public WindowControl{

  public:

    WndFrame(WindowControl *Owner, TCHAR *Name, 
             int X, int Y, int Width, int Height):
      WindowControl(Owner, NULL, Name, X, Y, Width, Height)
    {

      mLastDrawTextHeight = 0;
      mIsListItem = false;

      SetForeColor(GetOwner()->GetForeColor());
      SetBackColor(GetOwner()->GetBackColor());
      mCaptionStyle = DT_EXPANDTABS
      | DT_LEFT
      | DT_NOCLIP
      | DT_WORDBREAK;
    };

    virtual void Destroy(void);

    void SetCaption(TCHAR *Value);
    TCHAR *GetCaption(void){return(mCaption);};

    UINT GetCaptionStyle(void){return(mCaptionStyle);};
    UINT SetCaptionStyle(UINT Value);

    int GetLastDrawTextHeight(void){return(mLastDrawTextHeight);};

    void SetIsListItem(bool Value){mIsListItem = Value;};


    int OnLButtonDown(WPARAM wParam, LPARAM lParam);

  protected:

    int OnKeyDown(WPARAM wParam, LPARAM lParam);

    bool mIsListItem;

    int mLastDrawTextHeight;
    UINT mCaptionStyle;

    void Paint(HDC hDC);

};

class WndListFrame:public WndFrame{

  public:

    typedef struct{
      int TopIndex;
      int BottomIndex;
      int ItemIndex;
      int DrawIndex;
      int SelectedIndex;
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
    
    int OnItemKeyDown(WindowControl *Sender, WPARAM wParam, LPARAM lParam);
    int PrepareItemDraw(void);
    void ResetList(void);
    void SetEnterCallback(void (*OnListCallback)(WindowControl * Sender, ListInfo_t *ListInfo));
    void RedrawScrolled(bool all);
    void DrawScrollBar(HDC hDC);
    int RecalculateIndices(bool bigscroll);
    void Redraw(void);
    int GetItemIndex(void){return(mListInfo.ItemIndex);}
    void SelectItemFromScreen(int xPos, int yPos, RECT *rect);

  protected:

    int OnLButtonDown(WPARAM wParam, LPARAM lParam);

    OnListCallback_t mOnListCallback;
    OnListCallback_t mOnListEnterCallback;
    ListInfo_t mListInfo;
    void Paint(HDC hDC);


};

class WndOwnerDrawFrame:public WndFrame{

  public:

    typedef void (*OnPaintCallback_t)(WindowControl * Sender, HDC hDC);

    WndOwnerDrawFrame(WindowControl *Owner, TCHAR *Name, int X, int Y, int Width, int Height, void(*OnPaintCallback)(WindowControl * Sender, HDC hDC)):
      WndFrame(Owner, Name, X, Y, Width, Height)
    {
      mCaption[0] = '\0';
      mOnPaintCallback = OnPaintCallback;
      SetForeColor(GetOwner()->GetForeColor());
      SetBackColor(GetOwner()->GetBackColor());

    };

    virtual void Destroy(void);

    void SetOnPaintNotify(void (*OnPaintCallback)(WindowControl * Sender, HDC hDC)){
      mOnPaintCallback = OnPaintCallback;
    }

  protected:

    OnPaintCallback_t mOnPaintCallback;
    void Paint(HDC hDC);

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
    HFONT mhTitleFont;
    WindowControl *mClientWindow;
    RECT mClientRect;
    RECT mTitleRect;

    int (*mOnTimerNotify)(WindowControl * Sender);
    int (*mOnKeyDownNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam);
    int (*mOnKeyUpNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam);
    int (*mOnLButtonUpNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam);
    int (*mOnUserMsgNotify)(WindowControl * Sender, MSG *msg);


    int OnUnhandledMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void Paint(HDC hDC);
    int cbTimerID;

  public:

    WndForm(HWND Parent, TCHAR *Name, TCHAR *Caption, int X, int Y, int Width, int Height);
    ~WndForm(void);
    virtual void Destroy(void);

    HWND GetClientAreaHandle(void);
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

    HFONT SetTitleFont(HFONT Value);

    int ShowModal(void);
    void Show(void);

    void SetCaption(TCHAR *Value);
    TCHAR *GetCaption(void){return(mCaption);};

    virtual int OnCommand(WPARAM wParam, LPARAM lParam);

    COLORREF SetForeColor(COLORREF Value);
    COLORREF SetBackColor(COLORREF Value);
    HFONT SetFont(HFONT Value);
    void SetKeyDownNotify(int (*KeyDownNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam));
    void SetKeyUpNotify(int (*KeyUpNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam));
    void SetLButtonUpNotify(int (*LButtonUpNotify)(WindowControl * Sender, WPARAM wParam, LPARAM lParam));

    void SetTimerNotify(int (*OnTimerNotify)(WindowControl * Sender));

    void SetUserMsgNotify(int (*OnUserMsgNotify)(WindowControl * Sender, MSG *msg));

};

class WndButton:public WindowControl{

  private:

    void Paint(HDC hDC);
    bool mDown;
    bool mDefault;
    int mLastDrawTextHeight;
    void (*mOnClickNotify)(WindowControl * Sender);

  public:

    typedef void (*ClickNotifyCallback_t)(WindowControl * Sender);

    WndButton(WindowControl *Parent, TCHAR *Name, TCHAR *Caption, int X, int Y, int Width, int Height, void(*Function)(WindowControl * Sender) = NULL);
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
    static HBITMAP hBmpLeft16;
    static HBITMAP hBmpRight32;
    static HBITMAP hBmpRight16;
    static int InstCount;

    HWND mhEdit;
    POINT mEditSize;
    POINT mEditPos;
    HFONT mhCaptionFont;
    HFONT mhValueFont;
    int  mBitmapSize;
    int  mCaptionWidth;
    RECT mHitRectUp;
    RECT mHitRectDown;
    bool mDownDown;
    bool mUpDown;

    void Paint(HDC hDC);
    void (*mOnClickUpNotify)(WindowControl * Sender);
    void (*mOnClickDownNotify)(WindowControl * Sender);

    int (*mOnDataChangeNotify)(WindowControl * Sender, int Mode, int Value);

    int IncValue(void);
    int DecValue(void);
    WNDPROC mEditWindowProcedure;

    DataField *mDataField;

    void UpdateButtonData(int Value);

  public:

    typedef int (*DataChangeCallback_t)(WindowControl * Sender, int Mode, int Value);

    WndProperty(WindowControl *Parent, TCHAR *Name, TCHAR *Caption, int X, int Y, int Width, int Height, int CaptionWidth, int (*DataChangeNotify)(WindowControl * Sender, int Mode, int Value), int MultiLine=false);
    ~WndProperty(void);
    virtual void Destroy(void);

    int WndProcEditControl(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    TCHAR *GetCaption(void){return(mCaption);};

    bool SetFocused(bool Value, HWND FromTo);

    bool SetReadOnly(bool Value);

    void RefreshDisplay(void);

    HFONT SetFont(HFONT Value);

    int OnKeyDown(WPARAM wParam, LPARAM lParam);
    int OnEditKeyDown(WPARAM wParam, LPARAM lParam);
    int OnLButtonDown(WPARAM wParam, LPARAM lParam);
    int OnLButtonUp(WPARAM wParam, LPARAM lParam);
    int OnLButtonDoubleClick(WPARAM wParam, LPARAM lParam);

//    int GetAsInteger(void){return(mValue);};
//    int SetAsInteger(int Value);

    DataField *GetDataField(void){return(mDataField);};
    DataField *SetDataField(DataField *Value);
    void SetText(TCHAR *Value);
    int SetButtonSize(int Value);

};

#ifndef ALTAIRSYNC

typedef void (*webpt2Event)(TCHAR *);

class WndEventButton:public WndButton {
 public:
  WndEventButton(WindowControl *Parent, TCHAR *Name, TCHAR *Caption, 
		 int X, int Y, int Width, int Height, TCHAR *ename,
		 TCHAR *eparameters);
  ~WndEventButton();
 public:
  void CallEvent(void);
 private:
  webpt2Event inputEvent;
  TCHAR *parameters;
};


#endif

#endif

