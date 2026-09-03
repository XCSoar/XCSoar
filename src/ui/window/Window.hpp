// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"
#include "MinimumSize.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/dim/Point.hpp"
#include "ui/dim/Size.hpp"
#include "util/IntrusiveList.hxx"

#include <cassert>

class Canvas;
class ContainerWindow;

/**
 * A portable wrapper for describing a window's style settings on
 * creation.
 */
class WindowStyle {
protected:
  bool visible = true;
  bool enabled = true;
  bool tab_stop = false, control_parent = false;
  bool has_border = false;

public:
  /** The window is initially not visible. */
  void Hide() noexcept {
    visible = false;
  }

  /**
   * The window is initially disabled.
   * A disabled window cannot receive input from the user.
   */
  void Disable() noexcept {
    enabled = false;
  }

  /**
   * The window is a control that can receive the keyboard focus when
   * the user presses the TAB key. Pressing the TAB key changes the
   * keyboard focus to the next control with the tab_stop style.
   */
  void TabStop() noexcept {
    tab_stop = true;
  }

  /**
   * If the search for the next control with the tab_stop style
   * encounters a window with the control_parent style, the system
   * recursively searches the window's children.
   */
  void ControlParent() noexcept {
    control_parent = true;
  }

  /** The window has a thin-line border. */
  void Border() noexcept {
    has_border = true;
  }

  /** The window has a sunken 3D border. */
  void SunkenEdge() noexcept {
    Border();
  }

  /** The window has a vertical scroll bar. */
  void VerticalScroll() noexcept {
  }

  void Popup() {
  }

  friend class Window;
};

/**
 * A Window is a portion on the screen which displays something, and
 * which optionally interacts with the user.  To draw custom graphics
 * into a Window, derive your class from #PaintWindow.
 */
class Window {
  friend class ContainerWindow;

  friend class WindowList;
  IntrusiveListHook<IntrusiveHookMode::NORMAL> siblings;

protected:
  ContainerWindow *parent = nullptr;

private:
  PixelPoint position;
  PixelSize size = {0, 0};

private:
  bool tab_stop, control_parent;

  bool visible = true;
  bool transparent = false;
  bool enabled;
  bool focused = false;
  bool capture = false;
  bool has_border = false;

public:
  Window() = default;
  virtual ~Window() noexcept;

  Window(const Window &other) = delete;
  Window &operator=(const Window &other) = delete;

  ContainerWindow *GetParent() const noexcept {
    assert(IsDefined());

    return parent;
  }

protected:
  /**
   * Assert that the current thread is the one which created this
   * window.
   */
#ifdef NDEBUG
  void AssertThread() const noexcept {}
  void AssertThreadOrUndefined() const noexcept {}
#else
  void AssertThread() const noexcept;
  void AssertThreadOrUndefined() const noexcept;
#endif

  bool HasBorder() const noexcept {
    return has_border;
  }

public:
  bool IsDefined() const noexcept {
    return size.width > 0;
  }

  PixelPoint GetTopLeft() const noexcept {
    assert(IsDefined());

    return position;
  }

  void Create(ContainerWindow *parent, const PixelRect rc,
              const WindowStyle window_style=WindowStyle()) noexcept;

  void Destroy() noexcept;

  /**
   * Determines the root owner window of this Window.  This is
   * probably a pointer to the #MainWindow instance.
   */
  [[gnu::pure]]
  ContainerWindow *GetRootOwner() noexcept;

  /**
   * Checks whether the window is "maximised" within its parent
   * window, i.e. whether its dimensions are not smaller than its
   * parent's dimensions.
   */
  [[gnu::pure]]
  bool IsMaximised() const noexcept;

  void Move(PixelPoint _position) noexcept {
    AssertThread();

    position = _position;
    Invalidate();
  }

  void Move(PixelPoint _position, PixelSize _size) noexcept {
    AssertThread();

    Move(_position);
    Resize(_size);
  }

  void Move(const PixelRect rc) noexcept {
    assert(rc.left < rc.right);
    assert(rc.top < rc.bottom);

    Move(rc.GetTopLeft(), rc.GetSize());
  }

  void MoveToCenter() noexcept {
    const PixelSize window_size = GetSize();
    const PixelRect parent_rect = GetParentClientRect();
    const PixelSize parent_size = parent_rect.GetSize();
    int dialog_x = parent_rect.left + (int(parent_size.width) - int(window_size.width)) / 2;
    int dialog_y = parent_rect.top + (int(parent_size.height) - int(window_size.height)) / 2;
    
    Move({dialog_x, dialog_y});
  }

  /**
   * Like Move(), but does not trigger a synchronous redraw.  The
   * caller is responsible for redrawing.
   */
  void FastMove(PixelPoint _position, PixelSize _size) noexcept {
    AssertThread();

    Move(_position, _size);
  }

  void FastMove(const PixelRect rc) noexcept {
    FastMove(rc.GetTopLeft(), rc.GetSize());
  }

  /**
   * Move the Window to the specified position within the parent
   * ContainerWindow and make it visible.
   */
  void MoveAndShow(const PixelRect rc) noexcept {
    AssertThread();

    Move(rc);
    Show();
  }

  void Resize(PixelSize _size) noexcept {
    AssertThread();

    if (_size == size)
      return;

    // Enforce minimum size only for top-level windows (issue #2110)
    if (parent == nullptr)
      _size = UI::ClampToMinimumSize(_size);

    size = _size;

    Invalidate();
    OnResize(size);
  }

  void BringToTop() noexcept;
  void BringToBottom() noexcept;

  void ShowOnTop() noexcept {
    AssertThread();

    BringToTop();
    Show();
  }

  /**
   * Determine whether this Window is visible.  This method disregards
   * the visibility of parent windows, it just checks if the "visible"
   * flag is set for this Window.
   */
  [[gnu::pure]]
  bool IsVisible() const noexcept {
    assert(IsDefined());

    return visible;
  }

  void Show() noexcept;
  void Hide() noexcept;

  /**
   * Like Hide(), but does not trigger a synchronous redraw of the
   * parent window's background.
   */
  void FastHide() noexcept {
    AssertThread();

    Hide();
  }

  void SetVisible(bool visible) noexcept {
    if (visible)
      Show();
    else
      Hide();
  }

  /**
   * Attempt to make this window visible by scrolling the parent (via
   * ContainerWindow::ScrollTo()).
   */
  void ScrollParentTo() noexcept;

  bool IsTransparent() const noexcept {
    return transparent;
  }

  /**
   * Declare this window "transparent".  This means that portions of
   * the windows below it may be visible, and it will not be
   * considered "covering" windows behind it completely.  This flag is
   * evaluated by WindowList::IsCovered().
   */
  void SetTransparent() noexcept {
    assert(!transparent);

    transparent = true;
  }

  [[gnu::pure]]
  bool IsTabStop() const noexcept {
    assert(IsDefined());

    return tab_stop;
  }

  [[gnu::pure]]
  bool IsControlParent() const noexcept {
    assert(IsDefined());

    return control_parent;
  }

  /**
   * Can this window get user input?
   */
  [[gnu::pure]]
  bool IsEnabled() const noexcept {
    assert(IsDefined());

    return enabled;
  }

  /**
   * Specifies whether this window can get user input.
   */
  void SetEnabled(bool enabled) noexcept;

  virtual Window *GetFocusedWindow() noexcept;
  virtual void SetFocus() noexcept;

  /**
   * Called by the parent window when this window loses focus, or when
   * one of its (indirect) child windows loses focus.  This method is
   * responsible for invoking OnKillFocus().
   */
  virtual void ClearFocus() noexcept;

  /**
   * Send keyboard focus to this window's parent.  This should usually
   * only be called when this window owns the keyboard focus, and
   * doesn't want it anymore.
   */
  void FocusParent() noexcept;

  [[gnu::pure]]
  bool HasFocus() const noexcept {
    assert(IsDefined());

    return focused;
  }

  void SetCapture() noexcept;
  void ReleaseCapture() noexcept;
  virtual void ClearCapture() noexcept;

protected:
#if defined(USE_X11) || defined(USE_WAYLAND)
  virtual void EnableCapture() noexcept {}
  virtual void DisableCapture() noexcept {}
#else
  void EnableCapture() noexcept {}
  void DisableCapture() noexcept {}
#endif

public:
  /**
   * Returns the position within the parent window.
   */
  [[gnu::pure]]
  const PixelRect GetPosition() const noexcept
  {
    assert(IsDefined());

    return { position, size };
  }

  /**
   * Translate coordinates relative to this window to coordinates
   * relative to the parent window.
   */
  [[gnu::pure]]
  PixelPoint ToParentCoordinates(const PixelPoint p) const noexcept {
    return GetPosition().GetTopLeft() + p;
  }

  [[gnu::pure]]
  PixelRect ToParentCoordinates(const PixelRect &r) const noexcept {
    return {ToParentCoordinates(r.GetTopLeft()), r.GetSize()};
  }

  [[gnu::pure]]
  virtual const PixelRect GetClientRect() const noexcept
  {
    assert(IsDefined());

    return PixelRect(size);
  }

  [[gnu::pure]]
  const PixelSize GetSize() const noexcept
  {
    assert(IsDefined());

    return size;
  }

  /**
   * Checks whether the specified coordinates are inside the Window's
   * client area.
   */
  [[gnu::pure]]
  bool IsInside(PixelPoint p) const noexcept {
    assert(IsDefined());

    const PixelSize size = GetSize();
    return unsigned(p.x) < size.width &&
        unsigned(p.y) < size.height;
  }

  /**
   * Returns the parent's client area rectangle.
   */
  [[gnu::pure]]
  PixelRect GetParentClientRect() const noexcept;

  virtual void Invalidate() noexcept;

  /**
   * Inject a key-press event from outside the message loop.
   */
  bool InjectKeyPress(unsigned key_code) noexcept {
    return OnKeyDown(key_code);
  }

public:
  /**
   * @return true on success, false if the window should not be
   * created
   */
  virtual void OnCreate();
  virtual void OnDestroy() noexcept;
  virtual void OnResize(PixelSize new_size) noexcept;
  virtual bool OnMouseMove(PixelPoint p, unsigned keys) noexcept;
  virtual bool OnMouseDown(PixelPoint p) noexcept;
  virtual bool OnMouseUp(PixelPoint p) noexcept;
  virtual bool OnMouseDouble(PixelPoint p) noexcept;
  virtual bool OnMouseWheel(PixelPoint p, int delta) noexcept;

#ifdef HAVE_MULTI_TOUCH
  /**
   * A secondary pointer is being pressed.
   */
  virtual bool OnMultiTouchDown() noexcept;

  /**
   * Two pointers are moving.  Coordinates are relative to this
   * window.  Used for pinch-to-zoom where both finger positions are
   * available (e.g. SDL).
   */
  virtual bool OnMultiTouchMove(PixelPoint a, PixelPoint b) noexcept;

  /**
   * A secondary pointer is being released.
   */
  virtual bool OnMultiTouchUp() noexcept;
#endif

  /**
   * Checks if the window wishes to handle a special key, like cursor
   * keys and tab.
   *
   * @return true if the window will handle they key, false if the
   * dialog manager may use it
   */
  [[gnu::pure]]
  virtual bool OnKeyCheck(unsigned key_code) const noexcept;

  virtual bool OnKeyDown(unsigned key_code) noexcept;
  virtual bool OnKeyUp(unsigned key_code) noexcept;

  /**
   * A character was entered with the (virtual) keyboard.  This will
   * not be called if OnKeydown() has been handled already.
   *
   * @param ch the unicode character
   * @return true if the event was handled
   */
  virtual bool OnCharacter(unsigned ch) noexcept;

  virtual void OnCancelMode() noexcept;
  virtual void OnSetFocus() noexcept;
  virtual void OnKillFocus() noexcept;
};
