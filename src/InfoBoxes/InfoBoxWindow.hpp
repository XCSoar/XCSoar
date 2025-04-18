// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"
#include "ui/window/LazyPaintWindow.hpp"
#include "ui/event/Timer.hpp"
#include "Data.hpp"

#include <memory>

struct InfoBoxSettings;
struct InfoBoxLook;
class Color;

class InfoBoxWindow : public LazyPaintWindow
{
  std::unique_ptr<InfoBoxContent> content;

  const InfoBoxSettings &settings;
  const InfoBoxLook &look;

  const unsigned border_kind;

  const unsigned id;

  /**
   * This value is an identifier for the #content field; it is
   * incremented each time a new #InfoBoxContent is installed.  It is
   * used to check whether a custom repaint is necessary.
   */
  unsigned content_serial;

  InfoBoxData data;

  bool dragging = false;

  /**
   * Is the mouse currently pressed inside this InfoBox?
   */
  bool pressed = false;

  /**
   * draw the selector event if the InfoBox window is not the system focus
   */
  bool force_draw_selector = false;

  /** a timer which returns keyboard focus back to the map window after a while */
  UI::Timer focus_timer{[this]{ FocusParent(); }};

  /**
   * This timer opens the dialog.  It is used to check for "long
   * click" and to delay the dialog a bit (for double click
   * detection).
   */
  UI::Timer dialog_timer{[this]{ OnDialogTimer(); }};

  PixelRect title_rect;
  PixelRect value_rect;
  PixelRect comment_rect;
  PixelRect value_and_comment_rect;

  unsigned unit_width = 0;

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

  const InfoBoxLook &GetLook() const {
    return look;
  }

  void SetContentProvider(std::unique_ptr<InfoBoxContent> _content);
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
  [[gnu::pure]]
  const InfoBoxPanel *GetDialogContent() const;

  const PixelRect GetValueRect() const {
    return value_rect;
  }
  const PixelRect GetValueAndCommentRect() const {
    return value_and_comment_rect;
  }

private:
  void OnDialogTimer() noexcept;

protected:
  void OnDestroy() noexcept override;
  void OnResize(PixelSize new_size) noexcept override;
  void OnSetFocus() noexcept override;
  void OnKillFocus() noexcept override;
  void OnCancelMode() noexcept override;

  bool OnKeyDown(unsigned key_code) noexcept override;

  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseDouble(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;

  /* methods from class LazyPaintWindow */
  void OnPaintBuffer(Canvas &canvas) noexcept override;
};
