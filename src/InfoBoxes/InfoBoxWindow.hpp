/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "InfoBoxes/Content/Base.hpp"
#include "Screen/LazyPaintWindow.hpp"
#include "Screen/Timer.hpp"
#include "Data.hpp"

struct InfoBoxSettings;
struct InfoBoxLook;
class Color;

class InfoBoxWindow : public LazyPaintWindow
{
  /** timeout of infobox focus [ms] */
  static constexpr unsigned FOCUS_TIMEOUT_MAX = 20 * 1000;

private:
  InfoBoxContent *content;

  const InfoBoxSettings &settings;
  const InfoBoxLook &look;

  const unsigned border_kind;

  const unsigned id;

  InfoBoxData data;

  bool dragging;

  /**
   * Is the mouse currently pressed inside this InfoBox?
   */
  bool pressed;

  /**
   * draw the selector event if the InfoBox window is not the system focus
   */
  bool force_draw_selector;

  /** a timer which returns keyboard focus back to the map window after a while */
  WindowTimer focus_timer;

  /**
   * This timer opens the dialog.  It is used to check for "long
   * click" and to delay the dialog a bit (for double click
   * detection).
   */
  WindowTimer dialog_timer;

  PixelRect title_rect;
  PixelRect value_rect;
  PixelRect comment_rect;
  PixelRect value_and_comment_rect;

  /**
   * Paints the InfoBox title to the given canvas
   * @param canvas The canvas to paint on
   */
  void PaintTitle(Canvas &canvas);
  /**
   * Paints the InfoBox value to the given canvas
   * @param canvas The canvas to paint on
   */
  void PaintValue(Canvas &canvas, Color background_color);
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
  void PaintInto(Canvas &dest, int xoff, int yoff,
                 unsigned width, unsigned height);

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
                unsigned id,
                WindowStyle style=WindowStyle());

  ~InfoBoxWindow();

  const InfoBoxLook &GetLook() const {
    return look;
  }

  void SetContentProvider(InfoBoxContent *_content);
  void UpdateContent();

private:
  void SetPressed(bool _pressed) {
    if (_pressed == pressed)
      return;

    pressed = _pressed;
    Invalidate();
  }

protected:
  void ShowDialog();

  bool HandleKey(InfoBoxContent::InfoBoxKeyCodes keycode);

public:
  gcc_pure
  const InfoBoxPanel *GetDialogContent() const;

  const PixelRect GetValueRect() const {
    return value_rect;
  }
  const PixelRect GetValueAndCommentRect() const {
    return value_and_comment_rect;
  }

protected:
  virtual void OnDestroy() override;
  virtual void OnResize(PixelSize new_size) override;
  virtual void OnSetFocus() override;
  virtual void OnKillFocus() override;
  virtual void OnCancelMode() override;
  virtual bool OnTimer(WindowTimer &timer) override;

  virtual bool OnKeyDown(unsigned key_code) override;

  bool OnMouseDown(PixelPoint p) override;
  bool OnMouseUp(PixelPoint p) override;
  bool OnMouseDouble(PixelPoint p) override;
  bool OnMouseMove(PixelPoint p, unsigned keys) override;

  /* virtual methods from class LazyPaintWindow */
  void OnPaintBuffer(Canvas &canvas) override;
};

#endif
