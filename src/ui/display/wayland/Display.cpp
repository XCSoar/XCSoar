// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "Scale.hpp"
#include "ui/dim/Size.hpp"
#include "util/StringAPI.hxx"

#ifdef USE_EGL
#include "ui/egl/System.hpp"
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
               struct wl_output *wl_output,
               [[maybe_unused]] int32_t x,
               [[maybe_unused]] int32_t y,
               int32_t physical_width, int32_t physical_height,
               [[maybe_unused]] int32_t subpixel,
               [[maybe_unused]] const char *make,
               [[maybe_unused]] const char *model,
               int32_t transform) noexcept
{
  auto &display = *static_cast<Display *>(data);
  display.OutputGeometry(wl_output, physical_width, physical_height,
                         transform);
}

static void
OutputMode(void *data,
           struct wl_output *wl_output,
           uint32_t flags, int32_t width, int32_t height,
           [[maybe_unused]] int32_t refresh) noexcept
{
  auto &display = *static_cast<Display *>(data);
  display.OutputMode(wl_output, flags, width, height);
}

static void
OutputDone([[maybe_unused]] void *data,
           [[maybe_unused]] struct wl_output *wl_output) noexcept
{
}

static void
OutputScale(void *data,
            struct wl_output *wl_output,
            int32_t factor) noexcept
{
  auto &display = *static_cast<Display *>(data);
  display.OutputScale(wl_output, factor);
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
                     struct zxdg_output_v1 *xdg_output,
                     int32_t width, int32_t height) noexcept
{
  auto &display = *static_cast<Display *>(data);
  display.OutputLogicalSize(xdg_output, width, height);
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
  for (auto &o : outputs) {
    if (o.xdg_output != nullptr)
      zxdg_output_v1_destroy(o.xdg_output);
    if (o.output != nullptr)
      wl_output_destroy(o.output);
  }
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

const Display::Output *
Display::FindOutput(struct wl_output *wl_output) const noexcept
{
  if (wl_output == nullptr)
    return nullptr;

  for (const auto &o : outputs)
    if (o.output == wl_output)
      return &o;

  return nullptr;
}

Display::Output *
Display::FindOutput(struct wl_output *wl_output) noexcept
{
  return const_cast<Output *>(
    static_cast<const Display *>(this)->FindOutput(wl_output));
}

Display::Output *
Display::FindOutputByXdg(struct zxdg_output_v1 *xdg_output) noexcept
{
  if (xdg_output == nullptr)
    return nullptr;

  for (auto &o : outputs)
    if (o.xdg_output == xdg_output)
      return &o;

  return nullptr;
}

const Display::Output *
Display::Active() const noexcept
{
  if (const auto *o = FindOutput(active_output); o != nullptr)
    return o;

  return outputs.empty() ? nullptr : &outputs.front();
}

PixelSize
Display::GetSize() const noexcept
{
  const auto *o = Active();
  if (o == nullptr)
    return {0, 0};

  if (o->logical_width > 0 && o->logical_height > 0)
    return {o->logical_width, o->logical_height};

  unsigned w = o->width, h = o->height;
  if (OutputAxesSwapped(o->transform)) {
    w = o->height;
    h = o->width;
  }

  if (w > 0 && h > 0 && o->scale > 1)
    return {w / o->scale, h / o->scale};

  return {w, h};
}

PixelSize
Display::GetHardwareSize() const noexcept
{
  const auto *o = Active();
  return o != nullptr ? PixelSize{o->width, o->height} : PixelSize{0, 0};
}

unsigned
Display::GetScale120() const noexcept
{
  const auto *o = Active();
  if (o == nullptr)
    return SCALE_100;

  const unsigned physical_width = OutputAxesSwapped(o->transform)
    ? o->height
    : o->width;

  if (o->logical_width > 0 && physical_width > 0)
    return ToScale120ths(physical_width, o->logical_width);

  if (o->scale > 1)
    return FromIntegerScale(o->scale);

  return SCALE_100;
}

PixelSize
Display::GetSizeMM() const noexcept
{
  const auto *o = Active();
  return o != nullptr
    ? PixelSize{o->mm_width, o->mm_height}
    : PixelSize{0, 0};
}

void
Display::SetSurfaceOutput(struct wl_output *output, bool entered) noexcept
{
  if (output == nullptr)
    return;

  if (entered) {
    active_output = output;
    return;
  }

  if (active_output != output)
    return;

  active_output = outputs.empty() ? nullptr : outputs.front().output;
}

void
Display::BindXdgOutput(Output &o) noexcept
{
  if (o.xdg_output != nullptr ||
      xdg_output_manager == nullptr ||
      o.output == nullptr)
    return;

  o.xdg_output = zxdg_output_manager_v1_get_xdg_output(xdg_output_manager,
                                                       o.output);
  if (o.xdg_output != nullptr)
    zxdg_output_v1_add_listener(o.xdg_output, &xdg_output_listener, this);
}

void
Display::RegistryHandler(struct wl_registry *registry, uint32_t id,
                         const char *interface, uint32_t version) noexcept
{
  if (interface == nullptr || *interface == '\0')
    return;

  if (StringIsEqual(interface, "wl_output") && !outputs.full()) {
    const uint32_t bind_version = version >= 2 ? 2 : 1;
    auto *wl = (struct wl_output *)
      wl_registry_bind(registry, id, &wl_output_interface, bind_version);
    if (wl == nullptr)
      return;

    Output o;
    o.output = wl;
    outputs.push_back(o);
    wl_output_add_listener(outputs.back().output, &output_listener, this);
    BindXdgOutput(outputs.back());
    if (active_output == nullptr)
      active_output = outputs.back().output;
    return;
  }

  if (StringIsEqual(interface, zxdg_output_manager_v1_interface.name) &&
      xdg_output_manager == nullptr && version >= 1) {
    const uint32_t bind_version = version >= 3 ? 3 : version;
    xdg_output_manager = (struct zxdg_output_manager_v1 *)
      wl_registry_bind(registry, id,
                       &zxdg_output_manager_v1_interface, bind_version);
    for (auto &o : outputs)
      BindXdgOutput(o);
  }
}

void
Display::OutputGeometry(struct wl_output *wl_output,
                        int32_t physical_width, int32_t physical_height,
                        int32_t transform) noexcept
{
  auto *o = FindOutput(wl_output);
  if (o == nullptr)
    return;

  o->transform = transform;
  if (physical_width > 0 && physical_height > 0) {
    o->mm_width = (unsigned)physical_width;
    o->mm_height = (unsigned)physical_height;
  }
}

void
Display::OutputMode(struct wl_output *wl_output,
                    uint32_t flags, int32_t width, int32_t height) noexcept
{
  if ((flags & WL_OUTPUT_MODE_CURRENT) == 0)
    return;

  auto *o = FindOutput(wl_output);
  if (o == nullptr)
    return;

  if (width > 0 && height > 0) {
    o->width = (unsigned)width;
    o->height = (unsigned)height;
  }
}

void
Display::OutputScale(struct wl_output *wl_output, int32_t factor) noexcept
{
  auto *o = FindOutput(wl_output);
  if (o == nullptr)
    return;

  if (factor > 0)
    o->scale = (unsigned)factor;
}

void
Display::OutputLogicalSize(struct zxdg_output_v1 *xdg_output,
                           int32_t width, int32_t height) noexcept
{
  auto *o = FindOutputByXdg(xdg_output);
  if (o == nullptr)
    return;

  if (width > 0 && height > 0) {
    o->logical_width = (unsigned)width;
    o->logical_height = (unsigned)height;
  }
}

} // namespace Wayland
