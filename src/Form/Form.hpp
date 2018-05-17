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

#ifndef XCSOAR_FORM_FORM_HPP
#define XCSOAR_FORM_FORM_HPP

#include "ActionListener.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/SolidContainerWindow.hpp"
#include "Util/StaticString.hxx"

#include <functional>

#include <tchar.h>

struct DialogLook;
class SingleWindow;
class PeriodClock;

enum ModalResult {
  mrOK = 2,
  mrCancel = 3,
};

/**
 * A WndForm represents a Window with a titlebar.
 * It is used to display the XML dialogs and MessageBoxes.
 */
class WndForm : public ContainerWindow,
                public ActionListener
{
public:
  typedef std::function<bool(unsigned)> KeyDownFunction;
  typedef std::function<bool(unsigned)> CharacterFunction;

protected:
  const DialogLook &look;

  int modal_result = 0;

  /**
   * The dialog stays open as long as this flag is set, even if
   * SetModalResult has been called.
   */
  bool force = false;

  /**
   * Show in modeless mode.  Close if screen is clicked
   * outside dialog
   */
  bool modeless = false;

  bool dragging = false;

  /** The ClientWindow */
  SolidContainerWindow client_area;
  /** Coordinates of the ClientWindow */
  PixelRect client_rect;
  /** Coordinates of the titlebar */
  PixelRect title_rect;

  KeyDownFunction key_down_function;
  CharacterFunction character_function;

  PixelPoint last_drag;

  /**
   * The OnPaint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  void OnPaint(Canvas &canvas) override;

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
    return title_rect.GetHeight();
  }

  void SetForceOpen(bool _force) {
    force = _force;
  }

  void SetModalResult(int Value) {
    modal_result = Value;
  }

  /** inherited from ActionListener */
  void OnAction(int id) override {
    SetModalResult(id);
  }

  /**
   * Enables "modeless": dialog will close if mouse is clicked
   * anywhere on screen outside the dialog
   */
  void SetModeless() {
    modeless = true;
  }

  int ShowModal();

  const TCHAR *GetCaption() const {
    return caption.c_str();
  }

  /** Set the titlebar text */
  void SetCaption(const TCHAR *_caption);

  /** from class Window */
  void OnCreate() override;
  void OnResize(PixelSize new_size) override;
  void OnDestroy() override;

  bool OnMouseMove(PixelPoint p, unsigned keys) override;
  bool OnMouseDown(PixelPoint p) override;
  bool OnMouseUp(PixelPoint p) override;
  void OnCancelMode() override;

#ifdef WIN32
  bool OnCommand(unsigned id, unsigned code) override;
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

  /**
   * Reposition window, if possible.  Will be called whenever the
   * parent window changes.
   *
   * @param parent_rc the parent's client rect
   */
  virtual void ReinitialiseLayout(const PixelRect &parent_rc);

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
