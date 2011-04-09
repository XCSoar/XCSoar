/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "InfoBoxes/Content/Base.hpp"
#include "Screen/PaintWindow.hpp"
#include "PeriodClock.hpp"

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

class Font;
class Angle;

struct InfoBoxLook {
  Pen border_pen, selector_pen;
  Brush background_brush;

  struct {
    Color fg_color;
    const Font *font;
  } title, value, comment;

  const Font *small_font;

  Color colors[6];

  Color get_color(int i, Color default_color) const {
    if (i < 0)
      return colors[0];
    else if (i >= 1 && (unsigned)i < sizeof(colors) / sizeof(colors[0]))
      return colors[i];
    else
      return default_color;
  }

  Color get_title_color(int i) const {
    return get_color(i, title.fg_color);
  }

  Color get_value_color(int i) const {
    return get_color(i, value.fg_color);
  }

  Color get_comment_color(int i) const {
    return get_color(i, comment.fg_color);
  }
};

class InfoBoxWindow : public PaintWindow
{
  /** timeout in quarter seconds of infobox focus */
  static const unsigned FOCUSTIMEOUTMAX = 24*4;

public:
  enum {
    BORDER_WIDTH = 1,

    TITLESIZE = 32,
    VALUESIZE = 32,
    COMMENTSIZE = 32,
  };

private:
  InfoBoxContent *content;
  ContainerWindow &parent;
  const InfoBoxLook &look;

  int  mBorderKind;

  TCHAR mTitle[TITLESIZE+1];
  TCHAR mValue[VALUESIZE+1];
  TCHAR mComment[COMMENTSIZE+1];
  Units_t mValueUnit;
  int mID;

  /** a timer which returns keyboard focus back to the map window after a while */
  timer_t focus_timer;

  PixelRect recTitle;
  PixelRect recValue;
  PixelRect recComment;

  int colorValue;
  int colorComment;
  int colorTitle;

  PeriodClock click_clock;

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
  /**
   * Paints the InfoBox with borders, title, comment and value
   */
  void Paint(Canvas &canvas);

public:
  void PaintInto(Canvas &dest, int xoff, int yoff, int width, int height);

  /**
   * Sets the unit of the InfoBox value
   * @param Value New unit of the InfoBox value
   */
  void SetValueUnit(Units_t Value);
  /**
   * Sets the InfoBox ID to the given Value
   * @param id New value of the InfoBox ID
   */
  void SetID(const int id);
  int GetID() { return mID; };
  /**
   * Sets the InfoBox title to the given Value
   * @param Value New value of the InfoBox title
   */
  void SetTitle(const TCHAR *Value);
  const TCHAR* GetTitle() { return mTitle; };
  /**
   * Sets the InfoBox value to the given Value
   * @param Value New value of the InfoBox value
   */
  void SetValue(const TCHAR *Value);

  /**
   * Sets the InfoBox value to the given angle.
   */
  void SetValue(Angle value, const TCHAR *suffix=_T(""));

  /**
   * Sets the InfoBox comment to the given Value
   * @param Value New value of the InfoBox comment
   */
  void SetComment(const TCHAR *Value);

  /**
   * Sets the InfoBox comment to the given angle.
   */
  void SetComment(Angle value, const TCHAR *suffix=_T(""));

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
   * Calls SetValueInvalid() then SetCommentInvalid()
   */
  void SetInvalid();
  /**
   * Resets value to --- and unassigns the unit
   */
  void SetValueInvalid();
  /**
   * Clears comment
   */
  void SetCommentInvalid();

  /**
   * Constructor of the InfoBoxWindow class
   * @param Parent The parent ContainerWindow (usually MainWindow)
   * @param X x-Coordinate of the InfoBox
   * @param Y y-Coordinate of the InfoBox
   * @param Width Width of the InfoBox
   * @param Height Height of the InfoBox
   */
  InfoBoxWindow(ContainerWindow &Parent, int X, int Y, int Width, int Height,
          int border_flags,
          const InfoBoxLook &_look);

  ~InfoBoxWindow() { delete content; }

  void SetContentProvider(InfoBoxContent *_content);
  bool UpdateContent();
  bool HandleKey(InfoBoxContent::InfoBoxKeyCodes keycode);

  /**
   * This passes a given value to the InfoBoxContent for further processing
   * and updates the InfoBox.
   * @param Value Value to handle
   * @return True on success, Fales otherwise
   */
  bool HandleQuickAccess(const TCHAR *Value);

  InfoBoxContent::DialogContent* GetDialogContent();

  const PixelRect get_value_rect() const {
    return recValue;
  }

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

  virtual bool on_mouse_up(int x, int y);

  /**
   * This event handler is called when a mouse button is double clicked over
   * the InfoBox
   * @param x x-Coordinate where the mouse button was pressed
   * @param y y-Coordinate where the mouse button was pressed
   * @return True if the event has been handled, False otherwise
   */
  virtual bool on_mouse_double(int x, int y);

  virtual bool on_resize(unsigned width, unsigned height);

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
