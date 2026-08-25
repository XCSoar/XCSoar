// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "ui/dim/Size.hpp"
#include "util/StringAPI.hxx"

#ifdef USE_EGL
#include "ui/egl/System.hpp"
#endif

#ifdef USE_GLX
#include "ui/glx/System.hpp"
#endif

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
               const char *make,
               const char *model,
               [[maybe_unused]] int32_t transform) noexcept
{
  auto &display = *static_cast<Display *>(data);
  display.OutputGeometry(physical_width, physical_height, make, model);
}

static void
OutputMode(void *data,
           [[maybe_unused]] struct wl_output *wl_output,
           uint32_t flags, int32_t width, int32_t height,
           int32_t refresh) noexcept
{
  auto &display = *static_cast<Display *>(data);
  display.OutputMode(flags, width, height, refresh);
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

Display::Display()
  :display(wl_display_connect(nullptr))
{
  if (display == nullptr)
    throw std::runtime_error("wl_display_connect() failed");

  make.clear();
  model.clear();

  struct wl_registry *registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registry_listener, this);
  wl_display_roundtrip(display);
  wl_display_roundtrip(display);
  wl_registry_destroy(registry);
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
  if (width > 0 && height > 0 && scale > 1)
    return {width / scale, height / scale};

  return {width, height};
}

PixelSize
Display::GetHardwareSize() const noexcept
{
  return {width, height};
}

PixelSize
Display::GetSizeMM() const noexcept
{
  return {mm_width, mm_height};
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
  }
}

void
Display::OutputGeometry(int32_t physical_width, int32_t physical_height,
                        const char *make_name,
                        const char *model_name) noexcept
{
  if (physical_width > 0 && physical_height > 0) {
    mm_width = (unsigned)physical_width;
    mm_height = (unsigned)physical_height;
  }

  if (make_name != nullptr)
    make.SetUTF8(make_name);
  if (model_name != nullptr)
    model.SetUTF8(model_name);
}

void
Display::OutputMode(uint32_t flags, int32_t width, int32_t height,
                    int32_t refresh) noexcept
{
  if ((flags & WL_OUTPUT_MODE_CURRENT) == 0)
    return;
  if (width > 0 && height > 0) {
    this->width = (unsigned)width;
    this->height = (unsigned)height;
  }
  if (refresh > 0)
    refresh_mhz = (unsigned)refresh;
}

void
Display::OutputScale(int32_t factor) noexcept
{
  if (factor > 0)
    scale = (unsigned)factor;
}

} // namespace Wayland
