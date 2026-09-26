// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/Form.hpp"
#include "time/PeriodClock.hpp"
#include "Asset.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/BoxShadowRenderer.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "ui/event/Globals.hpp"
#include "ui/window/custom/Reference.hpp"

#include <optional>

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#include "ui/canvas/opengl/Scissor.hpp"
#include "ui/opengl/System.hpp"

/**
 * While this is alive, drawing is limited to a rounded rectangle.
 * The pixels outside the curve keep whatever was painted there
 * before, which is the map and the dialog's shadow.
 */
class DialogRoundClip final {
  bool active = false;

public:
  DialogRoundClip(Canvas &canvas, const PixelRect &rc,
                  unsigned diameter) noexcept {
    if (diameter < 2 || rc.left >= rc.right || rc.top >= rc.bottom)
      return;

    active = true;
    glEnable(GL_STENCIL_TEST);

    {
      const GLCanvasScissor scissor(rc);
      glStencilMask(~0u);
      glClear(GL_STENCIL_BUFFER_BIT);
    }

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    canvas.SelectNullPen();
    {
      const Brush brush{COLOR_WHITE};
      canvas.Select(brush);
      canvas.DrawRoundRectangle(rc, PixelSize{diameter});
      canvas.SelectHollowBrush();
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  }

  ~DialogRoundClip() noexcept {
    if (!active)
      return;

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_ALWAYS, 0, ~0u);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(~0u);
    glDisable(GL_STENCIL_TEST);
  }

  DialogRoundClip(const DialogRoundClip &) = delete;
  DialogRoundClip &operator=(const DialogRoundClip &) = delete;
};
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
  :WndForm(main_window, _look, main_window.GetClientRect(), caption, style)
{
}

void
WndForm::Create(SingleWindow &main_window, const PixelRect &rc,
                const char *_caption, const WindowStyle style)
{
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
  Create(main_window, main_window.GetClientRect(), _caption, style);
}

SingleWindow &
WndForm::GetMainWindow()
{
  return *(SingleWindow *)GetRootOwner();
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

  /* from the inset top, so the border does not eat the tail
     below the baseline.  The line height is that tail plus the
     ascent. */
  title_rect.bottom = title_rect.top;
  if (!caption.empty())
    title_rect.bottom += (int)look.caption.font->GetHeight();

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
}

void
WndForm::OnResize(PixelSize new_size) noexcept
{
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

    PixelRect parent = GetParentClientRect();
    parent.Grow(-client_rect.top);

    PixelRect new_position = position;
    new_position.Offset(dx, dy);

    if (new_position.right < parent.left)
      new_position.Offset(parent.left - new_position.right, 0);

    if (new_position.left > parent.right)
      new_position.Offset(parent.right - new_position.left, 0);

    if (new_position.top > parent.bottom)
      new_position.Offset(0, parent.bottom - new_position.top);

    if (new_position.top < 0)
      new_position.Offset(0, -new_position.top);

#ifdef USE_MEMORY_CANVAS
    /* the RasterCanvas class doesn't clip negative window positions
       properly, therefore we avoid this problem at this stage */
    if (new_position.left < 0)
      new_position.left = 0;

    if (new_position.top < 0)
      new_position.top = 0;
#endif

    Move(new_position.GetTopLeft());

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
    if (!main_window.FilterEvent(event, this)) {
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

  const PixelRect rc_client = GetClientRect();
  const unsigned corner_diameter =
    ButtonFrameRenderer::GetCornerDiameter(rc_client);

  /* same corner curve as a button; a full-screen dialog stays
     square, and e-paper keeps its hard rectangular outline */
  bool round = !IsMaximised() && !IsDithered() && corner_diameter >= 2;

#ifdef ENABLE_OPENGL
  if (round) {
    GLint stencil_bits = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);
    if (stencil_bits <= 0)
      round = false;
  }

  if (!IsDithered() && !IsMaximised() && is_active)
    /* the shadow follows the same curve, so it does not square
       off the corners */
    DrawBoxShadow(rc_client, BoxShadowStyle::DIALOG,
                  round ? corner_diameter / 2 : 0);

  const DialogRoundClip clip(canvas, round ? rc_client : PixelRect{},
                             round ? corner_diameter : 0);
#else
  std::optional<Canvas::RoundCornerGuard> round_guard;
  if (round)
    round_guard.emplace(canvas, rc_client, corner_diameter / 2);
#endif

  if (round)
    /* the page colour under the client, including the bottom
       corners the client window does not cover */
    canvas.DrawFilledRectangle(rc_client, look.background_color);

  ContainerWindow::OnPaint(canvas);

  // Draw the borders
  if (!IsMaximised() && !round) {
    if (IsDithered())
      canvas.DrawOutlineRectangle(rc_client, COLOR_BLACK);
    else {
      PixelRect edge = rc_client;
      canvas.DrawRaisedEdge(edge);
    }
  }

  if (!caption.empty()) {
    // Set the colors
    canvas.SetTextColor(COLOR_WHITE);

    // Set the titlebar font and font-size
    canvas.Select(*look.caption.font);

    /* top of the bitmap is the top of the line; the tail below
       the baseline then sits inside the bar */
    const PixelPoint text_at =
      title_rect.GetTopLeft().At(Layout::GetTextPadding(), 0);

    // JMW todo add here icons?

    /* the band reaches the window edge so the rounded top corners
       are the title, not the page colour underneath */
    const PixelRect title_band = round
      ? PixelRect{rc_client.left, rc_client.top,
                  rc_client.right, title_rect.bottom}
      : title_rect;

#ifdef EYE_CANDY
    if (!IsDithered() && is_active) {
      canvas.SetBackgroundTransparent();
      canvas.Stretch(title_band.GetTopLeft(), title_band.GetSize(),
                     look.caption.background_bitmap);

      canvas.DrawText(text_at, caption.c_str());
    } else {
#endif
      canvas.SetBackgroundColor(is_active
                                ? look.caption.background_color
                                : look.caption.inactive_background_color);
      if (round)
        canvas.DrawFilledRectangle(title_band,
                                   canvas.GetBackgroundColor());
      canvas.DrawOpaqueText(text_at, title_rect, caption.c_str());
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
WndForm::ReinitialiseLayout(const PixelRect &parent_rc) noexcept
{
  const unsigned parent_width = parent_rc.GetWidth();
  const unsigned parent_height = parent_rc.GetHeight();

  if (parent_width < GetSize().width || parent_height < GetSize().height) {
  } else {
    // reposition dialog to fit into TopWindow
    PixelRect rc = GetPosition();

    if (rc.right > (int)parent_width)
      rc.left = parent_width - rc.GetWidth();
    if (rc.bottom > (int)parent_height)
      rc.top = parent_height - rc.GetHeight();

#ifdef USE_MEMORY_CANVAS
    /* the RasterCanvas class doesn't clip negative window positions
       properly, therefore we avoid this problem at this stage */
    if (rc.left < 0)
      rc.left = 0;
    if (rc.top < 0)
      rc.top = 0;
#endif

    Move(rc.GetTopLeft());
  }
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
