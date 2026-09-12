// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "Scale.hpp"
#include "ui/dim/Size.hpp"
#include "util/StringAPI.hxx"

#ifdef USE_EGL
#include "ui/egl/System.hpp"
#endif

#ifdef USE_GLX
#include "ui/glx/System.hpp"
#endif

#include "xdg-output-unstable-v1-client-protocol.h"

#include <wayland-client.h>

#include <cstdint>
#include <stdexcept>

namespace Wayland {

static void
RegistryGlobal(void *data, struct wl_registry *registry, uint32_t id,
               const char *interface, uint32_t version) noexcept
{
  auto &display = *static_cast<Display *>(data);
  display.RegistryHandler(registry, id, interface, version);
}

static void
RegistryGlobalRemove([[maybe_unused]] void *data,
                     [[maybe_unused]] struct wl_registry *registry,
                     [[maybe_unused]] uint32_t id) noexcept
{
}

static constexpr struct wl_registry_listener registry_listener = {
  .global = RegistryGlobal,
  .global_remove = RegistryGlobalRemove,
};

static void
OutputGeometry(void *data,
               [[maybe_unused]] struct wl_output *wl_output,
               [[maybe_unused]] int32_t x,
               [[maybe_unused]] int32_t y,
               int32_t physical_width, int32_t physical_height,
               [[maybe_unused]] int32_t subpixel,
               [[maybe_unused]] const char *make,
               [[maybe_unused]] const char *model,
               int32_t transform) noexcept
{
  auto &display = *static_cast<Display *>(data);
  display.OutputGeometry(physical_width, physical_height, transform);
}

static void
OutputMode(void *data,
           [[maybe_unused]] struct wl_output *wl_output,
           uint32_t flags, int32_t width, int32_t height,
           [[maybe_unused]] int32_t refresh) noexcept
{
  auto &display = *static_cast<Display *>(data);
  display.OutputMode(flags, width, height);
}

static void
OutputDone([[maybe_unused]] void *data,
           [[maybe_unused]] struct wl_output *wl_output) noexcept
{
}

static void
OutputScale(void *data,
            [[maybe_unused]] struct wl_output *wl_output,
            int32_t factor) noexcept
{
  auto &display = *static_cast<Display *>(data);
  display.OutputScale(factor);
}

static constexpr struct wl_output_listener output_listener = {
  .geometry = OutputGeometry,
  .mode = OutputMode,
  .done = OutputDone,
  .scale = OutputScale,
};

static void
XdgOutputLogicalPosition([[maybe_unused]] void *data,
                         [[maybe_unused]] struct zxdg_output_v1 *xdg_output,
                         [[maybe_unused]] int32_t x,
                         [[maybe_unused]] int32_t y) noexcept
{
}

static void
XdgOutputLogicalSize(void *data,
                     [[maybe_unused]] struct zxdg_output_v1 *xdg_output,
                     int32_t width, int32_t height) noexcept
{
  auto &display = *static_cast<Display *>(data);
  display.OutputLogicalSize(width, height);
}

static void
XdgOutputDone([[maybe_unused]] void *data,
              [[maybe_unused]] struct zxdg_output_v1 *xdg_output) noexcept
{
}

static void
XdgOutputName([[maybe_unused]] void *data,
              [[maybe_unused]] struct zxdg_output_v1 *xdg_output,
              [[maybe_unused]] const char *name) noexcept
{
}

static void
XdgOutputDescription([[maybe_unused]] void *data,
                     [[maybe_unused]] struct zxdg_output_v1 *xdg_output,
                     [[maybe_unused]] const char *description) noexcept
{
}

static constexpr struct zxdg_output_v1_listener xdg_output_listener = {
  .logical_position = XdgOutputLogicalPosition,
  .logical_size = XdgOutputLogicalSize,
  .done = XdgOutputDone,
  .name = XdgOutputName,
  .description = XdgOutputDescription,
};

Display::Display()
  :display(wl_display_connect(nullptr))
{
  if (display == nullptr)
    throw std::runtime_error("wl_display_connect() failed");

  struct wl_registry *registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registry_listener, this);
  wl_display_roundtrip(display);
  wl_display_roundtrip(display);
  wl_registry_destroy(registry);
}

Display::~Display() noexcept
{
  if (xdg_output != nullptr)
    zxdg_output_v1_destroy(xdg_output);
  if (output != nullptr)
    wl_output_destroy(output);
  if (xdg_output_manager != nullptr)
    zxdg_output_manager_v1_destroy(xdg_output_manager);

  wl_display_disconnect(display);
}

[[gnu::const]]
static bool
OutputAxesSwapped(int32_t transform) noexcept
{
  return transform == WL_OUTPUT_TRANSFORM_90 ||
         transform == WL_OUTPUT_TRANSFORM_270 ||
         transform == WL_OUTPUT_TRANSFORM_FLIPPED_90 ||
         transform == WL_OUTPUT_TRANSFORM_FLIPPED_270;
}

PixelSize
Display::GetSize() const noexcept
{
  if (logical_width > 0 && logical_height > 0)
    return {logical_width, logical_height};

  unsigned w = width, h = height;
  if (OutputAxesSwapped(transform)) {
    w = height;
    h = width;
  }

  if (w > 0 && h > 0 && scale > 1)
    return {w / scale, h / scale};

  return {w, h};
}

PixelSize
Display::GetHardwareSize() const noexcept
{
  return {width, height};
}

unsigned
Display::GetScale120() const noexcept
{
  const unsigned physical_width = OutputAxesSwapped(transform)
    ? height
    : width;

  if (logical_width > 0 && physical_width > 0)
    return ToScale120ths(physical_width, logical_width);

  if (scale > 1)
    return FromIntegerScale(scale);

  return SCALE_100;
}

PixelSize
Display::GetSizeMM() const noexcept
{
  return {mm_width, mm_height};
}

void
Display::BindXdgOutput() noexcept
{
  if (xdg_output != nullptr ||
      xdg_output_manager == nullptr ||
      output == nullptr)
    return;

  xdg_output = zxdg_output_manager_v1_get_xdg_output(xdg_output_manager,
                                                     output);
  if (xdg_output != nullptr)
    zxdg_output_v1_add_listener(xdg_output, &xdg_output_listener, this);
}

void
Display::RegistryHandler(struct wl_registry *registry, uint32_t id,
                         const char *interface, uint32_t version) noexcept
{
  if (StringIsEqual(interface, "wl_output") && output == nullptr) {
    const uint32_t bind_version = version >= 2 ? 2 : 1;
    output = (struct wl_output *)
      wl_registry_bind(registry, id, &wl_output_interface, bind_version);
    if (output != nullptr)
      wl_output_add_listener(output, &output_listener, this);
    BindXdgOutput();
    return;
  }

  if (StringIsEqual(interface, zxdg_output_manager_v1_interface.name) &&
      xdg_output_manager == nullptr && version >= 1) {
    const uint32_t bind_version = version >= 3 ? 3 : version;
    xdg_output_manager = (struct zxdg_output_manager_v1 *)
      wl_registry_bind(registry, id,
                       &zxdg_output_manager_v1_interface, bind_version);
    BindXdgOutput();
  }
}

void
Display::OutputGeometry(int32_t physical_width, int32_t physical_height,
                        int32_t transform) noexcept
{
  this->transform = transform;
  if (physical_width > 0 && physical_height > 0) {
    mm_width = (unsigned)physical_width;
    mm_height = (unsigned)physical_height;
  }
}

void
Display::OutputMode(uint32_t flags, int32_t width, int32_t height) noexcept
{
  if ((flags & WL_OUTPUT_MODE_CURRENT) == 0)
    return;
  if (width > 0 && height > 0) {
    this->width = (unsigned)width;
    this->height = (unsigned)height;
  }
}

void
Display::OutputScale(int32_t factor) noexcept
{
  if (factor > 0)
    scale = (unsigned)factor;
}

void
Display::OutputLogicalSize(int32_t width, int32_t height) noexcept
{
  if (width > 0 && height > 0) {
    logical_width = (unsigned)width;
    logical_height = (unsigned)height;
  }
}

} // namespace Wayland
