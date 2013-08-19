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
#include "Util/StaticString.hpp"
#include "Util/tstring.hpp"

#include <functional>

#include <tchar.h>

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
    ClientAreaWindow(const DialogLook &_look)
      :look(_look) {}

  protected:
#ifdef USE_GDI
    virtual const Brush *OnChildColor(Window &window,
                                      Canvas &canvas) override;
#endif

    virtual void OnPaint(Canvas &canvas) override;
  };

public:
  typedef std::function<bool(unsigned)> KeyDownFunction;
  typedef std::function<bool(unsigned)> CharacterFunction;

protected:
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

  bool dragging;

  /** The ClientWindow */
  ClientAreaWindow client_area;
  /** Coordinates of the ClientWindow */
  PixelRect client_rect;
  /** Coordinates of the titlebar */
  PixelRect title_rect;

  KeyDownFunction key_down_function;
  CharacterFunction character_function;

  /*
   * Control which should get the focus by default
   */
  Window *default_focus;

  RasterPoint last_drag;

  /**
   * The OnPaint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  virtual void OnPaint(Canvas &canvas) override;

  StaticString<256> caption;

public:
  WndForm(const DialogLook &_look);

  /**
   * Constructor of the WndForm class
   * @param _main_window
   * @param Caption Titlebar text of the Window
   */
  WndForm(SingleWindow &_main_window, const DialogLook &_look,
          const PixelRect &rc,
          const TCHAR *caption=nullptr,
          const WindowStyle style = WindowStyle());

  /** Destructor */
  virtual ~WndForm();

  void Create(SingleWindow &main_window, const PixelRect &rc,
              const TCHAR *caption=nullptr,
              const WindowStyle style=WindowStyle());

  /**
   * Create a full-screen dialog.
   */
  void Create(SingleWindow &main_window,
              const TCHAR *caption=nullptr,
              const WindowStyle style=WindowStyle());

protected:
  void UpdateLayout();

public:
  /**
   * Returns a reference to the main window.  This is used by dialogs
   * when they want to open another dialog.
   */
  gcc_pure
  SingleWindow &GetMainWindow();

  const DialogLook &GetLook() const {
    return look;
  }

  ContainerWindow &GetClientAreaWindow() {
    return client_area;
  }

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
  virtual void OnAction(int id) override {
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
  virtual void OnCreate() override;
  virtual void OnResize(PixelSize new_size) override;
  virtual void OnDestroy() override;

  virtual bool OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys) override;
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y) override;
  virtual void OnCancelMode() override;

#ifdef WIN32
  virtual bool OnCommand(unsigned id, unsigned code) override;
#endif

  void SetKeyDownFunction(KeyDownFunction function) {
    key_down_function = function;
  }

  void ClearKeyDownFunction() {
    key_down_function = KeyDownFunction();
  }

  void SetCharacterFunction(CharacterFunction function) {
    character_function = function;
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

protected:
  /**
   * Assign the initial keyboard focus.
   */
  virtual void SetDefaultFocus();

  /**
   * This method can intercept a "key down" event before it gets
   * delivered to the focused dialog control.
   *
   * @return true if the event has been handled and shall be consumed
   */
  virtual bool OnAnyKeyDown(unsigned key_code);
};

#endif
