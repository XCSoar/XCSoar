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

#ifndef XCSOAR_FORM_FORM_HPP
#define XCSOAR_FORM_FORM_HPP

#include "Screen/ContainerWindow.hpp"
#include "Util/StaticString.hpp"
#include "Util/tstring.hpp"

#ifdef EYE_CANDY
#include "Screen/Bitmap.hpp"
#endif

#include <map>
#include <forward_list>

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
class WndForm: public ContainerWindow
{
  struct tstring_less_than {
    bool operator()(const tstring &a, const tstring &b) const
    {
      return a.compare(b) < 0;
    }
  };

  typedef std::forward_list<Window *> window_list_t;
  typedef std::map<tstring, Window *, tstring_less_than> name_to_window_t;

  class ClientAreaWindow : public ContainerWindow {
    const DialogLook &look;

  public:
    typedef bool (*CommandCallback_t)(unsigned cmd);
    CommandCallback_t mCommandCallback;

  public:
    ClientAreaWindow(const DialogLook &_look)
      :look(_look), mCommandCallback(NULL) {}

  protected:
    virtual bool on_command(unsigned id, unsigned code);
    virtual const Brush *on_color(Window &window, Canvas &canvas);
    virtual void on_paint(Canvas &canvas);
  };

public:
  typedef void (*TimerNotifyCallback_t)(WndForm &Sender);
  typedef bool (*KeyDownNotifyCallback_t)(WndForm &Sender, unsigned key_code);

private:
  /**
   * List of windows which will be deleted by the destructor of this
   * class.
   */
  window_list_t destruct_windows;

  /**
   * Mapping of control names to #Window objects.
   */
  name_to_window_t name_to_window;

  /**
   * List of windows which should only be visible in "advanced" mode.
   */
  window_list_t expert_windows;

protected:
  SingleWindow &main_window;

  const DialogLook &look;

  int mModalResult;

  /**
   * The dialog stays open as long as this flag is set, even if
   * SetModalResult has been called.
   */
  bool force;

  /** Font of the titlebar */
#ifdef EYE_CANDY
  Bitmap bitmap_title;
#endif
  /** The ClientWindow */
  ClientAreaWindow client_area;
  /** Coordinates of the ClientWindow */
  PixelRect mClientRect;
  /** Coordinates of the titlebar */
  PixelRect mTitleRect;

  TimerNotifyCallback_t mOnTimerNotify;
  KeyDownNotifyCallback_t mOnKeyDownNotify;

  /**
   * The on_paint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  virtual void on_paint(Canvas &canvas);

  timer_t cbTimerID;

  StaticString<256> mCaption;

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
          int X, int Y, int Width, int Height,
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

  ContainerWindow &GetClientAreaWindow(void);

  unsigned GetTitleHeight() const {
    return mTitleRect.bottom - mTitleRect.top;
  }

  /**
   * Add a #Window to the "destruct" list: the object will be deleted
   * by the destructor of this class.  This means that the caller
   * doesn't have to keep track of the specified Window, because this
   * WndForm is now responsible for freeing memory.
   */
  void AddDestruct(Window *window) {
    destruct_windows.push_front(window);
  }

  /**
   * Adds a #Window to the name-to-window map.
   */
  void AddNamed(const TCHAR *name, Window *window) {
    name_to_window[name] = window;
  }

  /**
   * Finds the ancestor window with the specified name.
   *
   * @param name the name of the #Window that is searched
   * @return the Window, or NULL if not found
   */
  Window *FindByName(const TCHAR *name);

  /**
   * Finds the ancestor window with the specified name.
   *
   * @param name the name of the #Window that is searched
   * @return the Window, or NULL if not found
   */
  const Window *FindByName(const TCHAR *name) const {
    return const_cast<WndForm *>(this)->FindByName(name);
  }

  /**
   * Adds a #Window to the "advanced window list" (#advanced_windows).
   */
  void AddExpert(Window *window) {
    expert_windows.push_front(window);
  }

  /**
   * Removes a #Window from the "advanced window list" (#advanced_windows).
   */
  void RemoveExpert(Window *window) {
    expert_windows.remove(window);
  }

  /**
   * Shows/Hides the ClientControls depending on the given value of advanced and
   * whether their caption includes an asterisk.
   * @param advanced True if advanced mode activated
   */
  void FilterAdvanced(bool advanced);

  void SetForceOpen(bool _force) {
    force = _force;
  }

  int GetModalResult(void) { return mModalResult; }
  int SetModalResult(int Value) {
    mModalResult = Value;
    return Value;
  }

  /**
   * @param mouse_allowed a Window which is allowed to get mouse
   * input, even though the dialog is modal (a hack for dlgTarget)
   */
  int ShowModal();

  const TCHAR *GetCaption() const {
    return mCaption.c_str();
  }

  /** Set the titlebar text */
  void SetCaption(const TCHAR *Value);

  /** from class Window */
  virtual bool on_resize(UPixelScalar width, UPixelScalar height);
  virtual bool on_destroy();
  virtual bool on_timer(timer_t id);

#ifdef WIN32
  virtual bool on_command(unsigned id, unsigned code);
#endif

  void SetKeyDownNotify(KeyDownNotifyCallback_t KeyDownNotify) {
    mOnKeyDownNotify = KeyDownNotify;
  }

  void SetTimerNotify(TimerNotifyCallback_t OnTimerNotify, unsigned ms = 500);

  void SetCommandCallback(ClientAreaWindow::CommandCallback_t CommandCallback) {
    client_area.mCommandCallback = CommandCallback;
  }

#ifdef ANDROID
  /**
   * Reposition window, if possible, or fail with mrChangeLayout in case
   * there is not enough space. Will be called whenever the parent window
   * changes.
   */
  void ReinitialiseLayout();
#endif

private:
  static PeriodClock timeAnyOpenClose; // when any dlg opens or child closes
};

#endif
