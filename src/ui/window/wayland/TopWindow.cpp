// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../TopWindow.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/poll/Queue.hpp"
#include "ui/display/Display.hpp"
#include "ui/display/wayland/Scale.hpp"
#include "Asset.hpp"
#include "LogFile.hpp"
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "fractional-scale-v1-client-protocol.h"
#include "pointer-constraints-unstable-v1-client-protocol.h"
#ifdef SOFTWARE_ROTATE_DISPLAY
#include "DisplayOrientation.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#endif

#include <wayland-client.h>
#include <wayland-egl.h>

#include <stdexcept>
#include <chrono>

namespace UI {

/* OnEvent is defined in poll/TopWindow.cpp.  That object has no other
   Wayland symbols (OnResize is in this file), so keep a used
   reference here to extract it from the screen archive. */
static auto const link_poll_on_event [[gnu::used]] =
  &TopWindow::OnEvent;

static void
handle_ping([[maybe_unused]] void *data,
            struct wl_shell_surface *shell_surface,
            uint32_t serial) noexcept
{
  wl_shell_surface_pong(shell_surface, serial);
}

static void
handle_configure(void *data,
                 [[maybe_unused]] struct wl_shell_surface *shell_surface,
                 [[maybe_unused]] uint32_t edges,
                 int32_t width,
                 int32_t height) noexcept
{
  if (width > 0 && height > 0) {
    auto *window = static_cast<TopWindow *>(data);
    window->OnNativeConfigure(PixelSize(width, height));
  }
}

static void
handle_popup_done([[maybe_unused]] void *data,
                  [[maybe_unused]] struct wl_shell_surface *shell_surface) noexcept
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
  .ping = handle_ping,
  .configure = handle_configure,
  .popup_done = handle_popup_done
};

static void
handle_wm_base_ping([[maybe_unused]] void *data,
                    struct xdg_wm_base *xdg_wm_base,
                    uint32_t serial) noexcept
{
  xdg_wm_base_pong(xdg_wm_base, serial);
}

static constexpr struct xdg_wm_base_listener wm_base_listener = {
  .ping = handle_wm_base_ping,
};

static void
handle_surface_configure([[maybe_unused]] void *data,
                         struct xdg_surface *xdg_surface,
                         uint32_t serial) noexcept
{
  xdg_surface_ack_configure(xdg_surface, serial);
}

static const struct xdg_surface_listener surface_listener = {
  .configure = handle_surface_configure,
};

static void
handle_toplevel_configure(void *data,
                          [[maybe_unused]] struct xdg_toplevel *xdg_toplevel,
                          int32_t width,
                          int32_t height,
                          struct wl_array *states) noexcept
{
  auto *window = static_cast<TopWindow *>(data);

  bool suspended = false;
  const auto *state = static_cast<const uint32_t *>(states->data);
  const auto *const states_end =
    state + states->size / sizeof(uint32_t);
  for (; state < states_end; ++state) {
    switch (*state) {
#ifdef XDG_TOPLEVEL_STATE_SUSPENDED_SINCE_VERSION
    case XDG_TOPLEVEL_STATE_SUSPENDED:
      suspended = true;
      break;
#endif
    default:
      break;
    }
  }

  if (event_queue != nullptr)
    event_queue->SetToplevelState(suspended);

  window->OnToplevelConfigureSize(width, height);
}

static void
handle_toplevel_close([[maybe_unused]] void *data,
                      [[maybe_unused]] struct xdg_toplevel *xdg_toplevel) noexcept
{
  if (event_queue != nullptr) {
    // Inject CLOSE event into the event queue
    event_queue->Inject(Event::CLOSE);
  }
}

static void
handle_toplevel_configure_bounds([[maybe_unused]] void *data,
                                 [[maybe_unused]] struct xdg_toplevel *xdg_toplevel,
                                 [[maybe_unused]] int32_t width,
                                 [[maybe_unused]] int32_t height) noexcept
{
}

static void
handle_toplevel_wm_capabilities([[maybe_unused]] void *data,
                                [[maybe_unused]] struct xdg_toplevel *xdg_toplevel,
                                [[maybe_unused]] struct wl_array *capabilities) noexcept
{
}

static const struct xdg_toplevel_listener toplevel_listener = {
  .configure = handle_toplevel_configure,
  .close = handle_toplevel_close,
  .configure_bounds = handle_toplevel_configure_bounds,
  .wm_capabilities = handle_toplevel_wm_capabilities,
};

static void
handle_fractional_preferred_scale(void *data,
                                  [[maybe_unused]] struct wp_fractional_scale_v1 *fs,
                                  uint32_t scale) noexcept
{
  static_cast<TopWindow *>(data)->OnFractionalPreferredScale(scale);
}

static const struct wp_fractional_scale_v1_listener
fractional_scale_listener = {
  .preferred_scale = handle_fractional_preferred_scale,
};

static void
handle_surface_enter(void *data,
                      [[maybe_unused]] struct wl_surface *surface,
                      struct wl_output *output) noexcept
{
  static_cast<TopWindow *>(data)->OnSurfaceOutput(output, true);
}

static void
handle_surface_leave(void *data,
                      [[maybe_unused]] struct wl_surface *surface,
                      struct wl_output *output) noexcept
{
  static_cast<TopWindow *>(data)->OnSurfaceOutput(output, false);
}

#ifdef WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION
static void
handle_preferred_buffer_scale([[maybe_unused]] void *data,
                              [[maybe_unused]] struct wl_surface *surface,
                              [[maybe_unused]] int32_t factor) noexcept
{
}

static void
handle_preferred_buffer_transform([[maybe_unused]] void *data,
                                   [[maybe_unused]] struct wl_surface *surface,
                                   [[maybe_unused]] uint32_t transform) noexcept
{
}
#endif

static const struct wl_surface_listener wl_surface_output_listener = {
  .enter = handle_surface_enter,
  .leave = handle_surface_leave,
#ifdef WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION
  .preferred_buffer_scale = handle_preferred_buffer_scale,
  .preferred_buffer_transform = handle_preferred_buffer_transform,
#endif
};

static void
handle_pointer_confined([[maybe_unused]] void *data,
                        [[maybe_unused]] struct zwp_confined_pointer_v1 *p) noexcept
{
}

static void
handle_pointer_unconfined([[maybe_unused]] void *data,
                          [[maybe_unused]] struct zwp_confined_pointer_v1 *p) noexcept
{
}

static const struct zwp_confined_pointer_v1_listener
confined_pointer_listener = {
  .confined = handle_pointer_confined,
  .unconfined = handle_pointer_unconfined,
};

static void
SetBufferScale(struct wl_surface *surface, int32_t n) noexcept
{
  if (wl_proxy_get_version((struct wl_proxy *)surface) >= 3)
    wl_surface_set_buffer_scale(surface, n);
}

static void
SetOpaqueRegion(struct wl_compositor *compositor,
                struct wl_surface *surface,
                PixelSize logical) noexcept
{
  if (compositor == nullptr || surface == nullptr)
    return;

  const auto region = wl_compositor_create_region(compositor);
  wl_region_add(region, 0, 0,
                (int32_t)logical.width, (int32_t)logical.height);
  wl_surface_set_opaque_region(surface, region);
  wl_region_destroy(region);
}

void
TopWindow::CreateNative(const char *text, PixelSize size,
                        TopWindowStyle style)
{
  initial_requested_size = size;
  received_first_configure = false;
  last_resize_flush_time = std::chrono::steady_clock::now();
  scale_120 = display.GetScale120();

  auto compositor = event_queue->GetCompositor();

  wl_surface = wl_compositor_create_surface(compositor);
  if (wl_surface == nullptr)
    throw std::runtime_error("Failed to create Wayland surface");

  wl_surface_add_listener(wl_surface, &wl_surface_output_listener, this);

  auto *const fractional_manager = event_queue->GetFractionalScaleManager();
  auto *const viewporter = event_queue->GetViewporter();
  if (fractional_manager != nullptr && viewporter != nullptr) {
    viewport = wp_viewporter_get_viewport(viewporter, wl_surface);
    fractional_scale =
      wp_fractional_scale_manager_v1_get_fractional_scale(fractional_manager,
                                                          wl_surface);
    if (fractional_scale != nullptr)
      wp_fractional_scale_v1_add_listener(fractional_scale,
                                          &fractional_scale_listener, this);
  }

  if (auto wm_base = event_queue->GetWmBase()) {
    xdg_wm_base_add_listener(wm_base, &wm_base_listener, nullptr);

    xdg_surface = xdg_wm_base_get_xdg_surface(wm_base, wl_surface);
    xdg_surface_add_listener(xdg_surface, &surface_listener, this);

    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(xdg_toplevel, &toplevel_listener, this);
    xdg_toplevel_set_title(xdg_toplevel, text);
    xdg_toplevel_set_app_id(xdg_toplevel, "xcsoar");

    if (auto decoration_manager = event_queue->GetDecorationManager()) {
      xdg_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
        decoration_manager, xdg_toplevel);
      if (xdg_decoration != nullptr)
        zxdg_toplevel_decoration_v1_set_mode(
          xdg_decoration,
          ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }

    if (style.GetFullScreen())
      xdg_toplevel_set_fullscreen(xdg_toplevel, nullptr);

    wl_surface_commit(wl_surface);
    wl_display_roundtrip(display.GetWaylandDisplay());
  } else if (auto shell = event_queue->GetShell()) {
    auto shell_surface = wl_shell_get_shell_surface(shell, wl_surface);
    wl_shell_surface_add_listener(shell_surface,
                                  &shell_surface_listener, this);
    wl_shell_surface_set_toplevel(shell_surface);
    wl_shell_surface_set_title(shell_surface, text);

    if (style.GetFullScreen())
      wl_shell_surface_set_fullscreen(shell_surface,
                                      WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
                                      0, nullptr);
  }

  native_window = wl_egl_window_create(wl_surface, 1, 1);
  if (native_window == nullptr)
    throw std::runtime_error("Failed to create Wayland EGL window");

  ApplySurfaceScale(initial_requested_size);
}

bool
TopWindow::IsVisible() const noexcept
{
  return event_queue->IsVisible();
}

void
TopWindow::EnableCapture() noexcept
{
  if (confined_pointer != nullptr ||
      event_queue == nullptr ||
      wl_surface == nullptr)
    return;

  auto *constraints = event_queue->GetPointerConstraints();
  auto *pointer = event_queue->GetPointer();
  if (constraints == nullptr || pointer == nullptr)
    return;

  confined_pointer = zwp_pointer_constraints_v1_confine_pointer(
    constraints, wl_surface, pointer, nullptr,
    ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
  if (confined_pointer != nullptr)
    zwp_confined_pointer_v1_add_listener(confined_pointer,
                                         &confined_pointer_listener,
                                         this);
}

void
TopWindow::DisableCapture() noexcept
{
  if (confined_pointer == nullptr)
    return;

  zwp_confined_pointer_v1_destroy(confined_pointer);
  confined_pointer = nullptr;
}

void
TopWindow::DestroyNative() noexcept
{
  if (confined_pointer != nullptr) {
    zwp_confined_pointer_v1_destroy(confined_pointer);
    confined_pointer = nullptr;
  }
  if (native_window != nullptr) {
    wl_egl_window_destroy(native_window);
    native_window = nullptr;
  }
  if (fractional_scale != nullptr) {
    wp_fractional_scale_v1_destroy(fractional_scale);
    fractional_scale = nullptr;
  }
  if (viewport != nullptr) {
    wp_viewport_destroy(viewport);
    viewport = nullptr;
  }
  if (xdg_decoration != nullptr) {
    zxdg_toplevel_decoration_v1_destroy(xdg_decoration);
    xdg_decoration = nullptr;
  }
  if (xdg_toplevel != nullptr) {
    xdg_toplevel_destroy(xdg_toplevel);
    xdg_toplevel = nullptr;
  }
  if (xdg_surface != nullptr) {
    xdg_surface_destroy(xdg_surface);
    xdg_surface = nullptr;
  }
  if (wl_surface != nullptr) {
    wl_surface_destroy(wl_surface);
    wl_surface = nullptr;
  }
}

unsigned
TopWindow::EffectiveScale120() const noexcept
{
  if (viewport != nullptr && fractional_scale != nullptr)
    return scale_120 > 0 ? scale_120 : Wayland::SCALE_100;

  return display.GetScale120();
}

PixelSize
TopWindow::CompositorLogicalSize() const noexcept
{
  if (compositor_size.width > 0 && compositor_size.height > 0)
    return compositor_size;

  PixelSize size = GetSize();
#ifdef SOFTWARE_ROTATE_DISPLAY
  if (AreAxesSwapped(OpenGL::display_orientation))
    size = PixelSize(size.height, size.width);
#endif
  return Wayland::ToLogicalSize(size, EffectiveScale120());
}

void
TopWindow::RefreshSurfaceScale() noexcept
{
  if (!IsDefined())
    return;

  ApplySurfaceScale(CompositorLogicalSize());
  if (screen != nullptr) {
    const PixelSize physical = screen->GetSize();
    if (physical != GetSize())
      Resize(physical);
  }
  Invalidate();
}

void
TopWindow::ApplySurfaceScale(PixelSize logical_size) noexcept
{
  if (logical_size.width == 0 || logical_size.height == 0 ||
      wl_surface == nullptr)
    return;

  const unsigned s = EffectiveScale120();
  const bool fractional = viewport != nullptr && fractional_scale != nullptr;
  const auto buffer = Wayland::ChooseBuffer(logical_size, s, fractional);

  compositor_size = logical_size;

  if (screen == nullptr)
    LogFmt("Wayland: scale={:.2f} ({}) buffer={}x{} logical={}x{}",
           s / (double)Wayland::SCALE_100,
           fractional ? "fractional" : "integer",
           buffer.size.width, buffer.size.height,
           logical_size.width, logical_size.height);

  SetBufferScale(wl_surface, (int32_t)buffer.buffer_scale);
  if (fractional && viewport != nullptr)
    wp_viewport_set_destination(viewport,
                                (int32_t)logical_size.width,
                                (int32_t)logical_size.height);

  if (native_window != nullptr)
    wl_egl_window_resize(native_window,
                         (int)buffer.size.width, (int)buffer.size.height,
                         0, 0);

  if (event_queue != nullptr) {
    event_queue->SetSurfaceScale120(s);
    SetOpaqueRegion(event_queue->GetCompositor(), wl_surface, logical_size);
  }

  if (screen != nullptr)
    screen->CheckResize(buffer.size);
}

void
TopWindow::OnFractionalPreferredScale(unsigned new_scale_120) noexcept
{
  if (new_scale_120 == 0 || scale_120 == new_scale_120)
    return;

  scale_120 = new_scale_120;
  RefreshSurfaceScale();
}

void
TopWindow::OnSurfaceOutput(struct wl_output *output, bool entered) noexcept
{
  display.SetSurfaceOutput(output, entered);

  if (viewport == nullptr || fractional_scale == nullptr)
    scale_120 = display.GetScale120();

  RefreshSurfaceScale();
}

void
TopWindow::OnToplevelConfigureSize(int32_t width, int32_t height) noexcept
{
  /* width=0 or height=0 means "client decides". */
  if (width > 0 && height > 0)
    OnNativeConfigure(PixelSize(width, height));
  else if (initial_requested_size.width > 0 &&
           initial_requested_size.height > 0)
    OnNativeConfigure(initial_requested_size);
  else
    CommitNativeSurface();
}

void
TopWindow::OnNativeConfigure(PixelSize new_native_size) noexcept
{
  MarkFirstConfigureReceived();
  initial_requested_size = new_native_size;

  /* The first configure arrives during CreateNative(), before
     ContainerWindow::Create() has set size.  Resize() asserts
     IsDefined(). */
  if (!IsDefined())
    return;

  ApplySurfaceScale(new_native_size);

  const PixelSize physical_size = screen != nullptr
    ? screen->GetSize()
    : Wayland::ToPhysicalSize(new_native_size, EffectiveScale120());
  if (physical_size == GetSize())
    BumpRenderStateToken();

  Resize(physical_size);
}

void
TopWindow::CommitNativeSurface() noexcept
{
  if (wl_surface != nullptr)
    wl_surface_commit(wl_surface);
}

void
TopWindow::OnResize(PixelSize new_size) noexcept
{
  /* Skip the Create() resize until the compositor has configured us. */
  if (!received_first_configure)
    return;

  BumpRenderStateToken();

  event_queue->SetScreenSize(new_size);
  ApplySurfaceScale(CompositorLogicalSize());

  if (screen != nullptr) {
    Invalidate();
    if (screen->IsReady() && IsVisible()) {
      Expose();
      const auto now = std::chrono::steady_clock::now();
      const auto time_since_last_flush =
        std::chrono::duration_cast<std::chrono::milliseconds>(
          now - last_resize_flush_time).count();
      const int throttle_ms = HasEPaper() ? 100 : 16;
      if (time_since_last_flush >= throttle_ms) {
        wl_display_flush(display.GetWaylandDisplay());
        last_resize_flush_time = now;
      }
    } else if (wl_surface != nullptr) {
      wl_surface_commit(wl_surface);
    }
  }

  ContainerWindow::OnResize(new_size);

#ifdef USE_MEMORY_CANVAS
  if (screen != nullptr)
    screen->RequestResize(new_size);
#endif
}

} // namespace UI
