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

#ifndef XCSOAR_INFO_BOX_HPP
#define XCSOAR_INFO_BOX_HPP

#include "Units.hpp"
#include "Screen/BufferWindow.hpp"

typedef enum {
  bkNone,
  bkTop,
  bkRight,
  bkBottom,
  bkLeft
} BorderKind_t;

#define BORDERTOP    (1<<bkTop)
#define BORDERRIGHT  (1<<bkRight)
#define BORDERBOTTOM (1<<bkBottom)
#define BORDERLEFT   (1<<bkLeft)
#define BORDERTAB    (1<<(bkLeft+1))

#define TITLESIZE    32
#define VALUESIZE    32
#define COMMENTSIZE  32

class Font;

class InfoBox: public BufferWindow
{
public:
  enum {
    BORDER_WIDTH = 1,
  };

private:
  int mX;
  int mY;
  int mWidth;
  int mHeight;
  int  mBorderKind;
  Color mColorTitle;
  Color mColorTitleBk;
  Color mColorValue;
  Color mColorValueBk;
  Color mColorComment;
  Color mColorCommentBk;

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
  bool   mHasFocus;

  /** a timer which returns keyboard focus back to the map window after a while */
  timer_t focus_timer;

  RECT   recTitle;
  RECT   recValue;
  RECT   recComment;

  int color;
  int colorBottom;
  int colorTop;
  bool mSmallerFont;

  void InitializeDrawHelpers(void);
  /**
   * Paints the InfoBox title to the given canvas
   * @param canvas The canvas to paint on
   */
  void PaintTitle(Canvas &canvas);
  /**
   * Paints the InfoBox value to the given canvas
   * @param canvas The canvas to paint on
   */
  void PaintValue(Canvas &canvas);
  /**
   * Paints the InfoBox comment on the given canvas
   * @param canvas The canvas to paint on
   */
  void PaintComment(Canvas &canvas);
  /**
   * Paints the InfoBox selector on the given canvas if the InfoBox is focused
   * @param canvas The canvas to paint on
   */
  void PaintSelector(Canvas &canvas);

  // LRESULT CALLBACK InfoBoxWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
  /**
   * Paints the InfoBox with borders, title, comment and value
   */
  void Paint();
  void PaintInto(Canvas &dest, int xoff, int yoff, int width, int height);

  /**
   * Sets the unit of the InfoBox value
   * @param Value New unit of the InfoBox value
   */
  void SetValueUnit(Units_t Value);
  /**
   * Sets the InfoBox title to the given Value
   * @param Value New value of the InfoBox title
   */
  void SetTitle(const TCHAR *Value);
  /**
   * Sets the InfoBox value to the given Value
   * @param Value New value of the InfoBox value
   */
  void SetValue(const TCHAR *Value);
  /**
   * Sets the InfoBox comment to the given Value
   * @param Value New value of the InfoBox comment
   */
  void SetComment(const TCHAR *Value);
  void SetSmallerFont(bool smallerFont);

  int GetBorderKind(void);
  int SetBorderKind(int Value);

  /**
   * Sets the color of the InfoBox value to the given value
   * @param value New color of the InfoBox value
   */
  void SetColor(int Value);
  /**
   * Sets the color of the InfoBox comment to the given value
   * @param value New color of the InfoBox comment
   */
  void SetColorBottom(int Value);
  /**
   * Sets the color of the InfoBox title to the given value
   * @param value New color of the InfoBox title
   */
  void SetColorTop(int Value);

  /**
   * Constructor of the InfoBox class
   * @param Parent The parent ContainerWindow (usually MainWindow)
   * @param X x-Coordinate of the InfoBox
   * @param Y y-Coordinate of the InfoBox
   * @param Width Width of the InfoBox
   * @param Height Height of the InfoBox
   */
  InfoBox(ContainerWindow &Parent, int X, int Y, int Width, int Height);
  /** Destructor */
  ~InfoBox(void);

protected:
  /**
   * This event handler is called when a key is pressed down while the InfoBox
   * is focused
   * @param key_code The code of the key that was pressed
   * @return True if the event has been handled, False otherwise
   */
  virtual bool on_key_down(unsigned key_code);
  /**
   * This event handler is called when a mouse button is pressed down over
   * the InfoBox
   * @param x x-Coordinate where the mouse button was pressed
   * @param y y-Coordinate where the mouse button was pressed
   * @return True if the event has been handled, False otherwise
   */
  virtual bool on_mouse_down(int x, int y);
  /**
   * This event handler is called when a mouse button is double clicked over
   * the InfoBox
   * @param x x-Coordinate where the mouse button was pressed
   * @param y y-Coordinate where the mouse button was pressed
   * @return True if the event has been handled, False otherwise
   */
  virtual bool on_mouse_double(int x, int y);
  /**
   * This event handler is called when the InfoBox needs to be repainted
   * @param canvas The canvas to paint on
   */
  virtual void on_paint(Canvas &canvas);
  /**
   * This event handler is called when the InfoBox is getting focused
   */
  virtual bool on_setfocus();
  /**
   * This event handler is called when the InfoBox lost focus
   */
  virtual bool on_killfocus();
  /**
   * This event handler is called when a timer is triggered
   * @param id Id of the timer that triggered the handler
   */
  virtual bool on_timer(timer_t id);
};

#endif
