// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "Hardware/DisplayDPI.hpp"
#include "ui/dim/Size.hpp"
#include "util/StringAPI.hxx"

#ifdef USE_EGL
#include "ui/egl/System.hpp"
#endif

#ifdef USE_GLX
#include "ui/glx/System.hpp"
#endif

#include <wayland-client.h>

#include <cassert>
#include <stdexcept>

namespace Wayland {

void
OutputGeometry(void *data, struct wl_output *wl_output,
               [[maybe_unused]] int32_t x,
               [[maybe_unused]] int32_t y,
               int32_t physical_width,
               int32_t physical_height,
               [[maybe_unused]] int32_t subpixel,
               [[maybe_unused]] const char *make,
               [[maybe_unused]] const char *model,
               [[maybe_unused]] int32_t transform) noexcept
{
  auto *d = static_cast<Display *>(data);
  if (d->output == wl_output && physical_width > 0 && physical_height > 0) {
    d->size_mm = {static_cast<unsigned>(physical_width),
                   static_cast<unsigned>(physical_height)};
  }
}

void
OutputMode(void *data, struct wl_output *wl_output,
           [[maybe_unused]] uint32_t flags,
           int32_t width, int32_t height,
           [[maybe_unused]] int32_t refresh) noexcept
{
  auto *d = static_cast<Display *>(data);
  if (d->output == wl_output && width > 0 && height > 0) {
    /* Always prefer the current mode (flags & WL_OUTPUT_MODE_CURRENT).
       If no current mode has been received yet, use the first mode. */
    if ((flags & WL_OUTPUT_MODE_CURRENT) || d->size.width == 0) {
      d->size = {static_cast<unsigned>(width),
                  static_cast<unsigned>(height)};
    }
  }
}

void
OutputDone([[maybe_unused]] void *data,
           [[maybe_unused]] struct wl_output *wl_output) noexcept
{
}

void
OutputScale([[maybe_unused]] void *data,
            [[maybe_unused]] struct wl_output *wl_output,
            [[maybe_unused]] int32_t factor) noexcept
{
  /* Scale factor is provided by the compositor but we calculate DPI
     from physical dimensions, so we don't need to track it. */
}

static constexpr struct wl_output_listener output_listener = {
  .geometry = OutputGeometry,
  .mode = OutputMode,
  .done = OutputDone,
  .scale = OutputScale,
};

void
RegistryGlobal(void *data, struct wl_registry *registry, uint32_t id,
               const char *interface, [[maybe_unused]] uint32_t version)
{
  auto *d = static_cast<Display *>(data);
  if (StringIsEqual(interface, "wl_output") && d->output == nullptr) {
    d->output = static_cast<wl_output *>(
      wl_registry_bind(registry, id, &wl_output_interface, 2));
    if (d->output != nullptr)
      wl_output_add_listener(d->output, &output_listener, d);
  }
}

static constexpr struct wl_registry_listener registry_listener = {
  .global = RegistryGlobal,
  .global_remove = nullptr,
};

void
Display::InitOutput() const noexcept
{
  if (output_initialized)
    return;

  output_initialized = true;

  auto registry = wl_display_get_registry(display);
  if (registry == nullptr)
    return;

  wl_registry_add_listener(registry, &registry_listener,
                           const_cast<Display *>(this));

  /* First roundtrip: get registry events (output global) */
  wl_display_roundtrip(display);

  /* Second roundtrip: get output events (geometry, mode, done) */
  if (output != nullptr)
    wl_display_roundtrip(display);

  wl_registry_destroy(registry);
}

Display::Display()
  :display(wl_display_connect(nullptr))
{
  if (display == nullptr)
    throw std::runtime_error("wl_display_connect() failed");
}

Display::~Display() noexcept
{
  if (output != nullptr)
    wl_output_destroy(output);
  wl_display_disconnect(display);
}

PixelSize
Display::GetSize() const noexcept
{
  InitOutput();
  return size;
}

PixelSize
Display::GetSizeMM() const noexcept
{
  InitOutput();
  return size_mm;
}

} // namespace Wayland
