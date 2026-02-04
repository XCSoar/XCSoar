// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TargetPickerDialog.hpp"

#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "system/Path.hpp"
#include "Storage/PlatformStorageMonitor.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "system/FileUtil.hpp"
#include "util/ConvertString.hpp"
#include "Language/Language.hpp"
#include "Formatter/ByteSizeFormatter.hpp"

#include <optional>
#include <limits>
#include <string>
#include <vector>
#include <cstdint>

namespace {
  static AllocatedPath g_last_target;

  struct DeviceEntry {
    std::string device_utf8; /* primary row (UTF-8) */
    AllocatedPath mount;     /* mount path (UTF-8) */
    std::string device_id;   /* device node, e.g. /dev/sda1 (UTF-8) */
    std::optional<StorageDevice::Space> space; /* optional space info */
  };

  static std::string
  FormatSpaceString(const std::optional<StorageDevice::Space> &space)
  {
    std::string right_first;
    if (!space)
      return right_first;

    char free_buf[64];
    char total_buf[64];
    const uint64_t fv = space->free_bytes;
    const uint64_t tv = space->total_bytes;
    FormatByteSize(free_buf, sizeof(free_buf)/sizeof(free_buf[0]), fv, false);
    FormatByteSize(total_buf, sizeof(total_buf)/sizeof(total_buf[0]), tv, false);
    right_first = free_buf;
    right_first.append(" / ");
    right_first.append(total_buf);
    return right_first;
  }

  struct Renderer final : public ListItemRenderer {
    TwoTextRowsRenderer &r;
    const std::vector<DeviceEntry> &opts;

    Renderer(TwoTextRowsRenderer &rr, const std::vector<DeviceEntry> &o)
      : r(rr), opts(o) {}

    void OnPaintItem(Canvas &canvas, const PixelRect rc,
                     unsigned idx) noexcept override {
      if (idx >= opts.size())
        return;
      const auto &it = opts[idx];
      /* First row: volume label or name */
      const char *first = it.device_utf8.c_str();

      /* Second row: prefer device id (e.g. /dev/sda1), fall back to mount */
      const char *second_base = "";
      if (!it.device_id.empty()) {
        second_base = it.device_id.c_str();
      }
      if (second_base[0] == '\0')
        second_base = it.mount.c_str();

      // Right-aligned free/total on first row if available
      const auto right_first = FormatSpaceString(it.space);

      r.DrawFirstRow(canvas, rc, first);
      if (!right_first.empty())
        r.DrawRightFirstRow(canvas, rc, right_first.c_str());

      r.DrawSecondRow(canvas, rc, second_base);
    }
  };

  static int
  ShowTargetPicker(const DialogLook &look,
                   const std::vector<DeviceEntry> &options) noexcept
  {
    TwoTextRowsRenderer row_renderer;
    const unsigned row_height =
      row_renderer.CalculateLayout(look.text_font, look.small_font);
    Renderer renderer(row_renderer, options);

    return ListPicker(_("Choose target"), (unsigned)options.size(), 0,
                      row_height, renderer, false, nullptr, nullptr,
                      _("Scan"));
  }


  static std::vector<DeviceEntry> DiscoverTargets() noexcept {
    std::vector<DeviceEntry> result;

    auto monitor = CreatePlatformStorageMonitor();
    if (monitor) {
      for (const auto &dev : monitor->enumerate()) {
        if (dev == nullptr)
          continue;
        if (!dev->isWritable())
          continue;

        const std::string dev_id = dev->id();
        const std::string dev_name = dev->label();
        const std::string mount = dev->name();
        if (mount.empty())
          continue;

        UTF8ToWideConverter conv(mount.c_str());
        if (!conv.IsValid())
          continue;

        DeviceEntry e;
        /* Use the storage layer's label() (e.g. volume label) */
        e.device_utf8 = dev_name.empty() ? dev_id : dev_name;
        e.mount = AllocatedPath(conv.c_str());
        e.device_id = dev_id;
        if (auto s = dev->space())
          e.space = *s;
        result.push_back(std::move(e));
      }
    }

    return result;
  }
}

TargetPickerDialog::TargetPickerDialog(UI::SingleWindow &parent, const DialogLook &look) noexcept
  : parent_window(parent), dialog_look(look) {}

const AllocatedPath &TargetPickerDialog::GetLastTarget() noexcept {
  return g_last_target;
}

AllocatedPath
TargetPickerDialog::GetDefaultTarget() noexcept {
  auto options = DiscoverTargets();
  if (!options.empty())
    return AllocatedPath(options[0].mount.c_str());
  return AllocatedPath();
}

void TargetPickerDialog::SetLastTarget(const AllocatedPath &p) noexcept {
  g_last_target = AllocatedPath(p.c_str());
}

AllocatedPath 
TargetPickerDialog::ShowModal() noexcept {
  while (true) {
    auto options = DiscoverTargets();
    if (options.empty()) {
      ShowMessageBox(_("No writable destinations found."), _("Choose target"), MB_OK | MB_ICONERROR);
      return AllocatedPath();
    }

    int picked = ShowTargetPicker(dialog_look, options);
    if (picked == -2)
      continue;
    if (picked < 0)
      return AllocatedPath();

    const unsigned sel = (unsigned)picked;
    if (sel >= options.size())
      return AllocatedPath();

    const AllocatedPath &chosen = options[sel].mount;
    g_last_target = AllocatedPath(chosen.c_str());
    return AllocatedPath(chosen.c_str());
  }
}
