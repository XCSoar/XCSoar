/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Util/StaticString.hpp"
#include "InfoBoxes/Content/Base.hpp"
#include "Screen/PaintWindow.hpp"
#include "Screen/Timer.hpp"
#include "PeriodClock.hpp"
#include "Data.hpp"

struct InfoBoxSettings;
struct InfoBoxLook;
struct UnitsLook;

class InfoBoxWindow : public PaintWindow
{
  /** timeout of infobox focus [ms] */
  static constexpr unsigned FOCUS_TIMEOUT_MAX = 20 * 1000;

private:
  InfoBoxContent *content;

  const InfoBoxSettings &settings;
  const InfoBoxLook &look;
  const UnitsLook &units_look;

  const unsigned border_kind;

  const unsigned id;

  InfoBoxData data;

  /**
   * draw the selector event if the InfoBox window is not the system focus
   */
  bool force_draw_selector;

  /** a timer which returns keyboard focus back to the map window after a while */
  WindowTimer focus_timer;

  PixelRect title_rect;
  PixelRect value_rect;
  PixelRect comment_rect;
  PixelRect value_and_comment_rect;

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
   * Paints the InfoBox with borders, title, comment and value
   */
  void Paint(Canvas &canvas);

public:
  void PaintInto(Canvas &dest, PixelScalar xoff, PixelScalar yoff,
                 UPixelScalar width, UPixelScalar height);

  /**
   * Sets the InfoBox title to the given Value
   * @param Value New value of the InfoBox title
   */
  void SetTitle(const TCHAR *title);

  const TCHAR* GetTitle() {
    return data.title;
  };

  /**
   * Constructor of the InfoBoxWindow class
   * @param Parent The parent ContainerWindow (usually MainWindow)
   */
  InfoBoxWindow(ContainerWindow &parent, PixelRect rc, unsigned border_flags,
                const InfoBoxSettings &settings, const InfoBoxLook &_look,
                const UnitsLook &units_look,
                unsigned id,
                WindowStyle style=WindowStyle());

  ~InfoBoxWindow();

  const InfoBoxLook &GetLook() const {
    return look;
  }

  void SetContentProvider(InfoBoxContent *_content);
  void UpdateContent();

protected:
  void ShowDialog();

  bool HandleKey(InfoBoxContent::InfoBoxKeyCodes keycode);

public:
  /**
   * This passes a given value to the InfoBoxContent for further processing
   * and updates the InfoBox.
   * @param Value Value to handle
   * @return True on success, Fales otherwise
   */
  bool HandleQuickAccess(const TCHAR *value);

  const InfoBoxContent::DialogContent *GetDialogContent();

  const PixelRect GetValueRect() const {
    return value_rect;
  }
  const PixelRect GetValueAndCommentRect() const {
    return value_and_comment_rect;
  }

protected:
  virtual void OnDestroy();

  /**
   * This event handler is called when a key is pressed down while the InfoBox
   * is focused
   * @param key_code The code of the key that was pressed
   * @return True if the event has been handled, False otherwise
   */
  virtual bool OnKeyDown(unsigned key_code);
  /**
   * This event handler is called when a mouse button is pressed down over
   * the InfoBox
   * @param x x-Coordinate where the mouse button was pressed
   * @param y y-Coordinate where the mouse button was pressed
   * @return True if the event has been handled, False otherwise
   */
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y);

  virtual bool OnMouseUp(PixelScalar x, PixelScalar y);

  /**
   * This event handler is called when a mouse button is double clicked over
   * the InfoBox
   * @param x x-Coordinate where the mouse button was pressed
   * @param y y-Coordinate where the mouse button was pressed
   * @return True if the event has been handled, False otherwise
   */
  virtual bool OnMouseDouble(PixelScalar x, PixelScalar y);

  virtual void OnResize(UPixelScalar width, UPixelScalar height);

  /**
   * This event handler is called when the InfoBox needs to be repainted
   * @param canvas The canvas to paint on
   */
  virtual void OnPaint(Canvas &canvas);

  virtual bool OnCancelMode();
  virtual void OnSetFocus();
  virtual void OnKillFocus();

  /**
   * This event handler is called when a timer is triggered
   * @param id Id of the timer that triggered the handler
   */
  virtual bool OnTimer(WindowTimer &timer);
};

#endif
