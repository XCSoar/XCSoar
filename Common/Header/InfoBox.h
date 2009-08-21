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

#include "Units.h"
#include "Screen/Font.hpp"
#include "Dialogs.h"

#define BORDERTOP    (1<<bkTop)
#define BORDERRIGHT  (1<<bkRight)
#define BORDERBOTTOM (1<<bkBottom)
#define BORDERLEFT   (1<<bkLeft)
#define BORDERTAB    (1<<(bkLeft+1))

#define TITLESIZE    32
#define VALUESIZE    32
#define COMMENTSIZE  32


class InfoBox{
 public:
    static COLORREF redColor;
    static COLORREF blueColor;
    static COLORREF inv_redColor;
    static COLORREF inv_blueColor;

// Not really used, tested and dropped. But useful for the future
    static COLORREF yellowColor;
    static COLORREF greenColor;
    static COLORREF magentaColor;
    static COLORREF inv_yellowColor;
    static COLORREF inv_greenColor;
    static COLORREF inv_magentaColor;
  private:

    int mX;
    int mY;
    int mWidth;
    int mHeight;
    HWND mParent;
    HWND mHWnd;
    HDC  mHdc;
    HDC  mHdcTemp;
    HDC mHdcBuf;
    int  mBorderKind;
    COLORREF mColorBack;
    COLORREF mColorFore;
    COLORREF mColorTitle;
    COLORREF mColorTitleBk;
    COLORREF mColorValue;
    COLORREF mColorValueBk;
    COLORREF mColorComment;
    COLORREF mColorCommentBk;

    COLORREF mColorRed;
    COLORREF mColorBlue;

    bool mTitleChanged;

    HBRUSH mhBrushBk;
    HBRUSH mhBrushBkSel;
    HPEN mhPenBorder;
    HPEN mhPenSelector;
    TCHAR mTitle[TITLESIZE+1];
    TCHAR mValue[VALUESIZE+1];
    TCHAR mComment[COMMENTSIZE+1];
    Units_t mValueUnit;
    HFONT  *mphFontTitle;
    HFONT  *mphFontValue;
    HFONT  *mphFontComment;
    HFONT  *valueFont;
    FontHeightInfo_t *mpFontHeightTitle;
    FontHeightInfo_t *mpFontHeightValue;
    FontHeightInfo_t *mpFontHeightComment;
    bool   mHasFocus;
    RECT   recTitle;
    RECT   recValue;
    RECT   recComment;
    HBITMAP mhBitmapUnit;
    HBITMAP mBufBitMap;
    POINT  mBitmapUnitPos;
    POINT  mBitmapUnitSize;

    int color;
    int colorBottom;
    int colorTop;
    int mBorderSize;
    int mUnitBitmapKind;
    bool mVisible;
	bool mSmallerFont;

    void InitializeDrawHelpers(void);
    void PaintTitle(HDC dc);
    void PaintValue(HDC dc);
    void PaintComment(HDC dc);
    void PaintSelector(HDC dc);

    // LRESULT CALLBACK InfoBoxWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  public:
    void Paint();
    void PaintFast(void);
    void PaintInto(HDC mHdcDest, int xoff, int yoff, int width, int height);

    Units_t SetValueUnit(Units_t Value);
    void SetTitle(const TCHAR *Value);
    void SetValue(const TCHAR *Value);
    void SetComment(const TCHAR *Value);
	void SetSmallerFont(bool smallerFont);

    void SetFocus(bool Value);
    bool SetVisible(bool Value);

    int GetBorderKind(void);
    int SetBorderKind(int Value);

    HWND GetHandle(void);
    HWND GetParent(void);

    void SetColor(int Value);
    void SetColorBottom(int Value);
    void SetColorTop(int Value);

    InfoBox(HWND Parent, int X, int Y, int Width, int Height);
    ~InfoBox(void);

    HDC GetHdcBuf(void);

};
