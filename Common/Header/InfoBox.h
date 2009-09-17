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
#ifndef INFOBOX_H
#define INFOBOX_H

#include "Units.hpp"
#include "Screen/Font.hpp"
#include "Screen/BufferWindow.hpp"

#define BORDERTOP    (1<<bkTop)
#define BORDERRIGHT  (1<<bkRight)
#define BORDERBOTTOM (1<<bkBottom)
#define BORDERLEFT   (1<<bkLeft)
#define BORDERTAB    (1<<(bkLeft+1))

#define TITLESIZE    32
#define VALUESIZE    32
#define COMMENTSIZE  32


class InfoBox : public BufferWindow {
 public:
 private:

    int mX;
    int mY;
    int mWidth;
    int mHeight;
    int  mBorderKind;
    Color mColorBack;
    Color mColorFore;
    Color mColorTitle;
    Color mColorTitleBk;
    Color mColorValue;
    Color mColorValueBk;
    Color mColorComment;
    Color mColorCommentBk;

    Color mColorRed;
    Color mColorBlue;

    bool mTitleChanged;

    Brush mhBrushBk;
    Brush mhBrushBkSel;
    Pen mhPenBorder;
    Pen mhPenSelector;
    TCHAR mTitle[TITLESIZE+1];
    TCHAR mValue[VALUESIZE+1];
    TCHAR mComment[COMMENTSIZE+1];
    Units_t mValueUnit;
    const Font *mphFontTitle;
    const Font *mphFontValue;
    const Font *mphFontComment;
    const Font *valueFont;
    FontHeightInfo_t *mpFontHeightTitle;
    FontHeightInfo_t *mpFontHeightValue;
    FontHeightInfo_t *mpFontHeightComment;
    bool   mHasFocus;

    /** a timer which returns keyboard focus back to the map window
        after a while */
    timer_t focus_timer;

    RECT   recTitle;
    RECT   recValue;
    RECT   recComment;
    const Bitmap *mhBitmapUnit;
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
    void PaintTitle(Canvas &canvas);
    void PaintValue(Canvas &canvas);
    void PaintComment(Canvas &canvas);
    void PaintSelector(Canvas &canvas);

    // LRESULT CALLBACK InfoBoxWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  public:
    void Paint();
    void PaintFast(void);
    void PaintInto(Canvas &dest, int xoff, int yoff, int width, int height);

    Units_t SetValueUnit(Units_t Value);
    void SetTitle(const TCHAR *Value);
    void SetValue(const TCHAR *Value);
    void SetComment(const TCHAR *Value);
    void SetSmallerFont(bool smallerFont);

    void SetFocus(bool Value);
    bool SetVisible(bool Value);

    int GetBorderKind(void);
    int SetBorderKind(int Value);

    void SetColor(int Value);
    void SetColorBottom(int Value);
    void SetColorTop(int Value);

    InfoBox(ContainerWindow &Parent, int X, int Y, int Width, int Height);
    ~InfoBox(void);

    Canvas &GetCanvas(void);

protected:
  virtual bool on_key_down(unsigned key_code);
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_double(int x, int y);
  virtual void on_paint(Canvas &canvas);
  virtual bool on_setfocus();
  virtual bool on_killfocus();
  virtual bool on_timer(timer_t id);
};

#endif
