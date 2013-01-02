/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_FORM_FORM_HPP
#define XCSOAR_FORM_FORM_HPP

#include "ActionListener.hpp"
#include "SubForm.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Timer.hpp"
#include "Util/StaticString.hpp"
#include "Util/tstring.hpp"

struct DialogLook;
class SingleWindow;
class PeriodClock;

enum ModalResult {
  mrOK = 2,
  mrCancel = 3,
  mrChangeLayout = 4,
};

/**
 * A WndForm represents a Window with a titlebar.
 * It is used to display the XML dialogs and MessageBoxes.
 */
class WndForm : public ContainerWindow, public SubForm,
                public ActionListener
{
  class ClientAreaWindow : public ContainerWindow {
    const DialogLook &look;

  public:
    typedef bool (*CommandCallback_t)(unsigned cmd);
    CommandCallback_t command_callback;

  public:
    ClientAreaWindow(const DialogLook &_look)
      :look(_look), command_callback(NULL) {}

  protected:
    virtual bool OnCommand(unsigned id, unsigned code);
    virtual const Brush *on_color(Window &window, Canvas &canvas);
    virtual void OnPaint(Canvas &canvas);
  };

public:
  typedef void (*TimerNotifyCallback)(WndForm &sender);
  typedef bool (*KeyDownNotifyCallback)(WndForm &sender, unsigned key_code);

protected:
  SingleWindow &main_window;

  const DialogLook &look;

  int modal_result;

  /**
   * The dialog stays open as long as this flag is set, even if
   * SetModalResult has been called.
   */
  bool force;

  /**
   * Show in modeless mode.  Close if screen is clicked
   * outside dialog
   */
  bool modeless;

  /** The ClientWindow */
  ClientAreaWindow client_area;
  /** Coordinates of the ClientWindow */
  PixelRect client_rect;
  /** Coordinates of the titlebar */
  PixelRect title_rect;

  TimerNotifyCallback timer_notify_callback;
  KeyDownNotifyCallback key_down_notify_callback;

  /*
   * Control which should get the focus by default
   */
  Window *default_focus;

  /**
   * The OnPaint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  virtual void OnPaint(Canvas &canvas);

  WindowTimer timer;

  StaticString<256> caption;

public:
  /**
   * Constructor of the WndForm class
   * @param _main_window
   * @param Caption Titlebar text of the Window
   * @param X x-Coordinate of the Window
   * @param Y y-Coordinate of the Window
   * @param Width Width of the Window
   * @param Height Height of the Window
   */
  WndForm(SingleWindow &_main_window, const DialogLook &_look,
          const PixelRect &rc,
          const TCHAR *Caption = _T(""),
          const WindowStyle style = WindowStyle());

  /** Destructor */
  virtual ~WndForm();

protected:
  void UpdateLayout();

public:
  /**
   * Returns a reference to the main window.  This is used by dialogs
   * when they want to open another dialog.
   */
  SingleWindow &GetMainWindow() {
    return main_window;
  }

  const DialogLook &GetLook() const {
    return look;
  }

  ContainerWindow &GetClientAreaWindow();

  unsigned GetTitleHeight() const {
    return title_rect.bottom - title_rect.top;
  }

  void SetForceOpen(bool _force) {
    force = _force;
  }

  int GetModalResult() { return modal_result; }
  int SetModalResult(int Value) {
    modal_result = Value;
    return Value;
  }

  /** inherited from ActionListener */
  virtual void OnAction(int id) {
    SetModalResult(id);
  }

  /**
   * @param mouse_allowed a Window which is allowed to get mouse
   * input, even though the dialog is modal (a hack for dlgTarget)
   */
  int ShowModal();

  /**
   * Opens modeless dialog.  Dialog will close if mouse is clicked
   * anywhere on screen outside the dialog
   */
  int ShowModeless();

  const TCHAR *GetCaption() const {
    return caption.c_str();
  }

  /** Set the titlebar text */
  void SetCaption(const TCHAR *_caption);

  /** from class Window */
  virtual void OnResize(UPixelScalar width, UPixelScalar height);
  virtual void OnDestroy();
  virtual bool OnTimer(WindowTimer &timer);

#ifdef WIN32
  virtual bool OnCommand(unsigned id, unsigned code);
#endif

  void SetKeyDownNotify(KeyDownNotifyCallback KeyDownNotify) {
    key_down_notify_callback = KeyDownNotify;
  }

  void SetTimerNotify(TimerNotifyCallback OnTimerNotify, unsigned ms = 500);

  void SetCommandCallback(ClientAreaWindow::CommandCallback_t CommandCallback) {
    client_area.command_callback = CommandCallback;
  }

  void SetDefaultFocus(Window *_defaultFocus) {
    default_focus = _defaultFocus;
  }

  Window *GetDefaultFocus() {
    return default_focus;
  }

#ifdef ANDROID
  /**
   * Reposition window, if possible, or fail with mrChangeLayout in case
   * there is not enough space. Will be called whenever the parent window
   * changes.
   */
  void ReinitialiseLayout();
#endif
};

#endif
