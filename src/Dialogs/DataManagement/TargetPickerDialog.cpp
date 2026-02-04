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
#include "Language/Language.hpp"
#include "Formatter/ByteSizeFormatter.hpp"

#include <optional>
#include <limits>
#include <string>
#include <vector>
#include <span>
#include <cstdint>

namespace {

struct DeviceEntry {
  std::string device_label; /* primary row (UTF-8) */
  AllocatedPath mount;     /* mount path (UTF-8) */
  std::string device_id;   /* device node, e.g. /dev/sda1 (UTF-8) */
  std::optional<StorageDevice::Space> space; /* optional space info */
};

std::string
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
  std::span<const DeviceEntry> opts;

  Renderer(TwoTextRowsRenderer &rr, std::span<const DeviceEntry> o)
    : r(rr), opts(o) {}

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    if (idx >= opts.size())
      return;
    const auto &it = opts[idx];
    /* First row: volume label or name */
    const char *first = it.device_label.c_str();

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

int
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

std::vector<DeviceEntry>
DiscoverTargets() noexcept
{
  std::vector<DeviceEntry> result;

  auto monitor = CreatePlatformStorageMonitor();
  if (monitor) {
    for (const auto &dev : monitor->Enumerate()) {
      if (dev == nullptr)
        continue;
      if (!dev->IsWritable())
        continue;

      const std::string dev_id = dev->Id();
      const std::string dev_name = dev->Label();
      const std::string mount = dev->Name();
      if (mount.empty())
        continue;

      DeviceEntry e;
      /* Use the storage layer's label() (e.g. volume label) */
      e.device_label = dev_name.empty() ? dev_id : dev_name;
      e.mount = AllocatedPath(mount.c_str());
      e.device_id = dev_id;
      if (auto s = dev->GetSpace())
        e.space = *s;
      result.push_back(std::move(e));
    }
  }

  return result;
}
}

TargetPickerDialog::TargetPickerDialog(const DialogLook &look) noexcept
  : dialog_look(look) {}

AllocatedPath PickTarget(const DialogLook &look) noexcept;

AllocatedPath TargetPickerDialog::last_target;

const AllocatedPath &TargetPickerDialog::GetLastTarget() noexcept {
  return TargetPickerDialog::last_target;
}

AllocatedPath
TargetPickerDialog::GetDefaultTarget() noexcept {
  auto options = DiscoverTargets();
  if (!options.empty())
    return std::move(options[0].mount);
  return AllocatedPath();
}

void TargetPickerDialog::SetLastTarget(const AllocatedPath &p) noexcept {
  TargetPickerDialog::last_target = Path(p);
}

AllocatedPath 
TargetPickerDialog::ShowModal() noexcept {
  AllocatedPath chosen = PickTarget(dialog_look);
  if (chosen != nullptr)
    TargetPickerDialog::last_target = Path(chosen);
  return chosen;
}

AllocatedPath
PickTarget(const DialogLook &look) noexcept {
  // Re-scan devices when the extra "Scan" button is pressed.
  while (true) {
    auto options = DiscoverTargets();
    if (options.empty()) {
      ShowMessageBox(
        _("No writable destinations found."),
        _("Choose target"),
        MB_OK | MB_ICONERROR);
      return AllocatedPath();
    }

    int picked = ShowTargetPicker(look, options);
    /* ListPicker returns -2 when the extra button ("Scan") is pressed. */
    if (picked == -2)
      continue;
    if (picked < 0)
      return AllocatedPath();

    const unsigned sel = (unsigned)picked;
    if (sel >= options.size())
      return AllocatedPath();

    return std::move(options[sel].mount);
  }
}
