// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/ContainerWindow.hpp"
#include "ui/window/SolidContainerWindow.hpp"
#include "util/tstring.hpp"

#include <functional>
#include <tchar.h>

struct DialogLook;
namespace UI { class SingleWindow; }
class PeriodClock;

enum ModalResult {
  mrOK = 2,
  mrCancel = 3,
};

/**
 * A modal dialog.
 */
class WndForm : public ContainerWindow
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

  void OnPaint(Canvas &canvas) noexcept override;

  tstring caption;

public:
  WndForm(const DialogLook &_look);

  /**
   * Constructor of the WndForm class
   *
   * @param caption titlebar text of the dialog
   */
  WndForm(UI::SingleWindow &_main_window, const DialogLook &_look,
          const PixelRect &rc,
          const TCHAR *caption=nullptr,
          const WindowStyle style = WindowStyle());

  /**
   * Construct a full-screen dialog.
   */
  WndForm(UI::SingleWindow &_main_window, const DialogLook &_look,
          const TCHAR *caption=nullptr,
          const WindowStyle style={}) noexcept;

  void Create(UI::SingleWindow &main_window, const PixelRect &rc,
              const TCHAR *caption=nullptr,
              const WindowStyle style=WindowStyle());

  /**
   * Create a full-screen dialog.
   */
  void Create(UI::SingleWindow &main_window,
              const TCHAR *caption=nullptr,
              const WindowStyle style=WindowStyle());

protected:
  void UpdateLayout();

public:
  /**
   * Returns a reference to the main window.  This is used by dialogs
   * when they want to open another dialog.
   */
  [[gnu::pure]]
  UI::SingleWindow &GetMainWindow();

  const DialogLook &GetLook() const {
    return look;
  }

  ContainerWindow &GetClientAreaWindow() {
    return client_area;
  }

  /**
   * Calculate the dialog size from the desired effective client area
   * size.
   */
  PixelSize ClientAreaToDialogSize(PixelSize s) const noexcept {
    /* the "2" is the 1 pixel border at each side */
    return PixelSize(s.width + 2,
                     s.height + title_rect.GetHeight() + 2);
  }

  void SetForceOpen(bool _force) {
    force = _force;
  }

  virtual void SetModalResult(int Value) noexcept {
    modal_result = Value;
  }

  auto MakeModalResultCallback(int value) noexcept {
    return [this, value](){
      SetModalResult(value);
    };
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
  void OnResize(PixelSize new_size) noexcept override;
  void OnDestroy() noexcept override;

  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  void OnCancelMode() noexcept override;

#ifdef _WIN32
  bool OnCommand(unsigned id, unsigned code) noexcept override;
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
  virtual void ReinitialiseLayout(const PixelRect &parent_rc) noexcept;

protected:
  /**
   * Assign the initial keyboard focus.
   */
  virtual void SetDefaultFocus() noexcept;

  /**
   * This method can intercept a "key down" event before it gets
   * delivered to the focused dialog control.
   *
   * @return true if the event has been handled and shall be consumed
   */
  virtual bool OnAnyKeyDown(unsigned key_code) noexcept;
};
