// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/Form.hpp"
#include "time/PeriodClock.hpp"
#include "Asset.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "util/ScopeExit.hxx"
#include "Look/DialogLook.hpp"
#include "Renderer/BoxShadowRenderer.hpp"
#include "ui/event/Globals.hpp"

#include <algorithm>
#include <utility>

#include "ui/window/custom/Reference.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

#ifdef ANDROID
#include "ui/event/shared/Event.hpp"
#include "ui/event/android/Loop.hpp"
#elif defined(ENABLE_SDL)
#include "ui/event/sdl/Event.hpp"
#include "ui/event/sdl/Loop.hpp"
#elif defined(USE_POLL_EVENT)
#include "ui/event/shared/Event.hpp"
#include "ui/event/poll/Loop.hpp"
#endif

using namespace UI;

WndForm::WndForm(const DialogLook &_look)
  :look(_look)
{
}

WndForm::WndForm(SingleWindow &main_window, const DialogLook &_look,
                 const PixelRect &rc,
                 const char *Caption,
                 const WindowStyle style)
  :look(_look)
{
  Create(main_window, rc, Caption, style);
}

WndForm::WndForm(SingleWindow &main_window, const DialogLook &_look,
                 const char *caption,
                 const WindowStyle style) noexcept
  :WndForm(main_window, _look, main_window.GetDialogRect(), caption, style)
{
}

void
WndForm::Create(SingleWindow &main_window, const PixelRect &rc,
                const char *_caption, const WindowStyle style)
{
  preferred_size = rc.GetSize();
  full_screen = preferred_size == main_window.GetClientRect().GetSize() ||
    preferred_size == main_window.GetDialogRect().GetSize();

  if (_caption != nullptr)
    caption = _caption;
  else
    caption.clear();

  ContainerWindow::Create(main_window, rc, style);
}

void
WndForm::Create(SingleWindow &main_window,
                const char *_caption, const WindowStyle style)
{
  Create(main_window, main_window.GetDialogRect(), _caption, style);
}

SingleWindow &
WndForm::GetMainWindow()
{
  return *(SingleWindow *)GetRootOwner();
}

bool
WndForm::IsMaximised() const noexcept
{
  if (!IsDefined())
    return false;

  const PixelSize available = GetMainWindow().GetDialogRect().GetSize();
  const PixelSize size = GetSize();

  return size.width >= available.width && size.height >= available.height;
}

void
WndForm::UpdateLayout()
{
  PixelRect rc = GetClientRect();

  title_rect = rc;

  if (!IsMaximised()) {
    ++title_rect.left;
    ++title_rect.top;
    --title_rect.right;
  }

  title_rect.bottom = rc.top +
    (caption.empty() ? 0 : look.caption.font->GetHeight());

  client_rect = rc.RemainingBelowSafe(title_rect);

  if (!IsMaximised()) {
    ++client_rect.left;
    --client_rect.right;
    --client_rect.bottom;
  }
}

void
WndForm::OnCreate()
{
  ContainerWindow::OnCreate();

  UpdateLayout();

  WindowStyle client_style;
  client_style.ControlParent();
  client_area.Create(*this, client_rect, look.background_color, client_style);
  client_area.SetGradientTopColor(look.background_gradient_top_color);
}

void
WndForm::OnResize(PixelSize new_size) noexcept
{
  if (!reinitialising_layout) {
    /* Dialogs may replace their construction size after measuring content,
       or when switching tabs.  Retain that size for overlay restoration.
       On Win32, new_size is the client size, while Move() sizes the window. */
    preferred_size = GetPosition().GetSize();
    auto &main_window = GetMainWindow();
    full_screen = preferred_size == main_window.GetClientRect().GetSize() ||
      preferred_size == main_window.GetDialogRect().GetSize();
  }

  ContainerWindow::OnResize(new_size);
  UpdateLayout();
  client_area.Move(client_rect);

  if (client_layout_function)
    client_layout_function();
}

void
WndForm::OnDestroy() noexcept
{
  if (modal_result == 0)
    modal_result = mrCancel;

  ContainerWindow::OnDestroy();
}

bool
WndForm::OnMouseMove(PixelPoint p, unsigned keys) noexcept
{
  if (ContainerWindow::OnMouseMove(p, keys))
    return true;

  if (dragging) {
    const PixelRect position = GetPosition();
    const int dx = position.left + p.x - last_drag.x;
    const int dy = position.top + p.y - last_drag.y;
    last_drag.x = position.left + p.x;
    last_drag.y = position.top + p.y;

    const PixelRect parent = GetMainWindow().GetDialogRect();
    const PixelPoint origin{
      std::clamp(position.left + dx, parent.left,
                 std::max(parent.left, parent.right - int(GetSize().width))),
      std::clamp(position.top + dy, parent.top,
                 std::max(parent.top, parent.bottom - int(GetSize().height))),
    };
    Move(origin);

    return true;
  }

  return false;
}

bool
WndForm::OnMouseDown(PixelPoint p) noexcept
{
  if (ContainerWindow::OnMouseDown(p))
    return true;

  if (!IsIOS() && !dragging && !IsMaximised()) {
    dragging = true;
    Invalidate();

    const PixelRect position = GetPosition();
    last_drag.x = position.left + p.x;
    last_drag.y = position.top + p.y;
    SetCapture();
    return true;
  }

  return false;
}

bool
WndForm::OnMouseUp(PixelPoint p) noexcept
{
  if (ContainerWindow::OnMouseUp(p))
    return true;

  if (dragging) {
    dragging = false;
    Invalidate();
    ReleaseCapture();
    return true;
  }

  return false;
}

void
WndForm::OnCancelMode() noexcept
{
  ContainerWindow::OnCancelMode();

  if (dragging) {
    dragging = false;
    Invalidate();
    ReleaseCapture();
  }
}

/**
 * Is this key handled by the focused control? (bypassing the dialog
 * manager)
 */
[[gnu::pure]]
static bool
CheckKey(ContainerWindow *container, const Event &event)
{
  Window *focused = container->GetFocusedWindow();
  if (focused == nullptr)
    return false;

  return focused->OnKeyCheck(event.GetKeyCode());
}

int
WndForm::ShowModal()
{
  ContainerWindow *root = GetRootOwner();
  WindowReference old_focus_reference = root->GetFocusedWindowReference();

  PeriodClock enter_clock;
  if (HasTouchScreen())
    enter_clock.Update();

  ShowOnTop();

  modal_result = 0;

  SingleWindow &main_window = GetMainWindow();
  main_window.CancelMode();

  SetDefaultFocus();

  bool hastimed = false;

  main_window.AddDialog(this);

  main_window.Refresh();

  EventLoop loop(*event_queue, main_window);
  Event event;

  while ((modal_result == 0 || force) && loop.Get(event)) {
    const bool dialog_event = main_window.FilterEvent(event, this);
    Window *overlay = main_window.GetDialogOverlay();
    const bool overlay_event =
      !dialog_event && overlay != nullptr && event.IsMouse() &&
      main_window.FilterEvent(event, overlay);
    if (!dialog_event && !overlay_event) {
      if (modeless && event.IsMouseDown())
        break;
      else
        continue;
    }

    // hack to stop exiting immediately
    if (HasTouchScreen() && !hastimed &&
        event.IsUserInput()) {
      if (!enter_clock.Check(std::chrono::milliseconds(200)))
        /* ignore user input in the first 200ms */
        continue;
      else
        hastimed = true;
    }

    if (overlay_event) {
      /* Allow only the registered warning strip outside the modal dialog.
         Keep keyboard focus on the menu after tapping a warning button. */
#ifdef USE_WINUSER
      const HWND dialog_focus = ::GetFocus();
#else
      const auto dialog_focus = GetFocusedWindowReference();
#endif
      /* Bypass Win32's dialog keyboard manager for this sibling window. */
      loop.EventLoop::Dispatch(event);
#ifdef USE_WINUSER
      if (::IsWindow(dialog_focus))
        ::SetFocus(dialog_focus);
      else
        SetDefaultFocus();
#else
      Window *focus = dialog_focus.Defined()
        ? dialog_focus.Get(*this)
        : nullptr;
      if (focus != nullptr)
        focus->SetFocus();
      else
        SetDefaultFocus();
#endif
      continue;
    }

    if (event.IsKeyDown()) {
      if (OnAnyKeyDown(event.GetKeyCode()))
        continue;

#ifdef ENABLE_SDL
      if (event.GetKeyCode() == SDLK_TAB && !CheckKey(this, event)) {
        /* the Tab key moves the keyboard focus, unless the focused
           control handles it itself */
        const Uint8 *keystate = ::SDL_GetKeyboardState(nullptr);
        event.event.key.keysym.sym =
            keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]
          ? SDLK_UP : SDLK_DOWN;
      }
#endif

      if (event.GetKeyCode() == KEY_UP || event.GetKeyCode() == KEY_DOWN) {
        /* KEY_UP and KEY_DOWN move the focus only within the current
           control group - but we want it to behave like Shift-Tab and
           Tab */

        if (!CheckKey(this, event)) {
          /* this window doesn't handle KEY_UP/KEY_DOWN */
          if (event.GetKeyCode() == KEY_DOWN)
            FocusNextControl();
          else
            FocusPreviousControl();
          continue;
        }
      }

      if (event.GetKeyCode() == KEY_ESCAPE) {
        modal_result = mrCancel;
        continue;
      }

#ifdef KOBO
      if (event.GetKeyCode() == KEY_POWER) {
        /* the Kobo power button closes the modal dialog */
        modal_result = mrCancel;
        continue;
      }
#endif
    }

    if (character_function && (event.GetCharacterCount() > 0)) {
      bool handled = false;
      for (size_t i = 0; i < event.GetCharacterCount(); ++i)
        handled = character_function(event.GetCharacter(i)) || handled;
      if (handled)
        continue;
    }

    loop.Dispatch(event);
  } // End Modal Loop

  main_window.RemoveDialog(this);

  if (old_focus_reference.Defined()) {
    Window *old_focus = old_focus_reference.Get(*root);
    if (old_focus != nullptr)
      old_focus->SetFocus();
  }

  return modal_result;
}

void
WndForm::OnPaint(Canvas &canvas) noexcept
{
  const SingleWindow &main_window = GetMainWindow();
  [[maybe_unused]] const bool is_active = main_window.IsTopDialog(*this);

#ifdef ENABLE_OPENGL
  if (!IsDithered() && !IsMaximised() && is_active)
    /* draw a soft shadow around the current dialog to emphasise it */
    DrawBoxShadow(GetClientRect(), BoxShadowStyle::DIALOG);
#endif

  ContainerWindow::OnPaint(canvas);

  // Get window coordinates
  PixelRect rcClient = GetClientRect();

  // Draw the borders
  if (!IsMaximised()) {
    if (IsDithered())
      canvas.DrawOutlineRectangle(rcClient, COLOR_BLACK);
    else
      canvas.DrawRaisedEdge(rcClient);
  }

  if (!caption.empty()) {
    // Set the colors
    canvas.SetTextColor(COLOR_WHITE);

    // Set the titlebar font and font-size
    canvas.Select(*look.caption.font);

    // JMW todo add here icons?

#ifdef EYE_CANDY
    if (!IsDithered() && is_active) {
      canvas.SetBackgroundTransparent();
      canvas.Stretch(title_rect.GetTopLeft(), title_rect.GetSize(),
                     look.caption.background_bitmap);

      // Draw titlebar text
      canvas.DrawText(title_rect.GetTopLeft().At(Layout::GetTextPadding(), 0),
                      caption.c_str());
    } else {
#endif
      canvas.SetBackgroundColor(is_active
                                ? look.caption.background_color
                                : look.caption.inactive_background_color);
      canvas.DrawOpaqueText(title_rect.GetTopLeft().At(Layout::GetTextPadding(), 0),
                            title_rect, caption.c_str());
#ifdef EYE_CANDY
    }
#endif
  }

  if (dragging) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
    canvas.Clear(COLOR_YELLOW.WithAlpha(80));
#else
    canvas.InvertRectangle(GetClientRect());
#endif
  }
}

void
WndForm::SetCaption(const char *_caption)
{
  if (_caption == nullptr)
    _caption = "";

  if (caption != _caption) {
    caption = _caption;
    UpdateLayout();
    client_area.Move(client_rect);
    Invalidate(title_rect);
  }
}

void
WndForm::ReinitialiseLayout(const PixelRect &rc) noexcept
{
  /* edge-to-edge dialogs (Fly/Simulator) paint into the unsafe area
     and are not shortened for a warning banner */
  if (FillsClient())
    return;

  const bool was_reinitialising = std::exchange(reinitialising_layout, true);
  AtScopeExit(this, was_reinitialising) {
    reinitialising_layout = was_reinitialising;
  };

  if (full_screen) {
    Move(rc);
    return;
  }

  const PixelSize size{
    std::min(preferred_size.width, rc.GetWidth()),
    std::min(preferred_size.height, rc.GetHeight()),
  };
  const auto position = GetPosition();
  const int max_left = rc.right - int(size.width);
  const int max_top = rc.bottom - int(size.height);
  const PixelPoint origin{
    std::clamp(position.left, rc.left, std::max(rc.left, max_left)),
    std::clamp(position.top, rc.top, std::max(rc.top, max_top)),
  };
  Move(origin, size);
}

void
WndForm::SetDefaultFocus() noexcept
{
  SetFocus();
  client_area.FocusFirstControl();
}

bool
WndForm::OnAnyKeyDown(unsigned key_code) noexcept
{
  return key_down_function && key_down_function(key_code);
}
