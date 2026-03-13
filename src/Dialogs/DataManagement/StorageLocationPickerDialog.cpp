// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StorageLocationPickerDialog.hpp"

#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "system/Path.hpp"
#include "Storage/StorageEvents.hpp"
#include "Storage/StorageManager.hpp"
#include "Storage/StorageDevice.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Widget/ListWidget.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Language/Language.hpp"
#include "Formatter/ByteSizeFormatter.hpp"
#include "Storage/StorageUtil.hpp"
#include "util/StaticString.hxx"

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

namespace {

static AllocatedPath last_storage_target;

using DeviceList = std::vector<std::shared_ptr<StorageDevice>>;

static DeviceList
DiscoverTargets() noexcept
{
  DeviceList result;

  if (backend_components == nullptr ||
      backend_components->storage_manager == nullptr)
    return result;

  for (const auto &dev :
       backend_components->storage_manager->GetDevices()) {
    if (dev == nullptr)
      continue;
    if (!dev->IsWritable() && !dev->NeedsPermission())
      continue;
    if (dev->Name().empty())
      continue;

    result.push_back(dev);
  }

  std::stable_sort(result.begin(), result.end(),
                   [](const auto &a, const auto &b) {
                     return a->NeedsPermission() < b->NeedsPermission();
                   });

  return result;
}

/**
 * Custom list widget for the storage location picker.
 *
 * Paints device entries via TwoTextRowsRenderer and registers itself
 * as a StorageEventListener so the list refreshes automatically when
 * the OS hotplug monitor detects a device arrival or removal —
 * no need for the user to press "Scan".
 */
class StorageListWidget final : public ListWidget,
                                public StorageEventListener {
  TwoTextRowsRenderer row_renderer_;
  DeviceList options_;
  WndForm *dialog_ = nullptr;
  Button *select_button_ = nullptr;

  void SyncSelectButton() noexcept {
    if (select_button_ != nullptr)
      select_button_->SetEnabled(!options_.empty());
  }

public:
  void SetDialog(WndForm &d) noexcept { dialog_ = &d; }
  void SetSelectButton(Button &b) noexcept { select_button_ = &b; }

  [[nodiscard]] unsigned GetCursorIndex() const noexcept {
    return GetList().GetCursorIndex();
  }

  [[nodiscard]] DeviceList &GetOptions() noexcept {
    return options_;
  }

  void Refresh() noexcept {
    options_ = DiscoverTargets();
    GetList().SetLength(options_.size());
    GetList().Invalidate();
    SyncSelectButton();
  }

  void ScanAndReport() noexcept {
    const unsigned old_count = options_.size();
    Refresh();
    const unsigned new_count = options_.size();

    StaticString<128> msg;
    if (new_count > old_count)
      msg.Format(_("Found %u new device(s). %u total."),
                 new_count - old_count, new_count);
    else if (new_count < old_count)
      msg.Format(_("Removed %u device(s). %u remaining."),
                 old_count - new_count, new_count);
    else
      msg.Format(_("No changes. %u device(s)."), new_count);

    ShowMessageBox(msg, _("Scan"), MB_OK | MB_ICONINFORMATION);
  }

  /* StorageEventListener */
  void OnStorageEvent(const StorageEventInfo &) noexcept override {
    Refresh();
  }

  /* Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    const auto &look = UIGlobals::GetDialogLook();
    const unsigned row_height =
      row_renderer_.CalculateLayout(look.text_font, look.small_font);
    options_ = DiscoverTargets();
    CreateList(parent, look, rc, row_height).SetLength(options_.size());
    SyncSelectButton();

    if (backend_components != nullptr &&
        backend_components->storage_manager != nullptr)
      backend_components->storage_manager->AddEventListener(*this);
  }

  void Unprepare() noexcept override {
    if (backend_components != nullptr &&
        backend_components->storage_manager != nullptr)
      backend_components->storage_manager->RemoveEventListener(*this);
    ListWidget::Unprepare();
  }

  /* ListControl::Handler */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    if (idx >= options_.size())
      return;

    const auto &dev = *options_[idx];
    const std::string name = dev.Name();
    const std::string label = dev.Label();
    const std::string dev_id = dev.Id();

    std::string first_text = dev.NeedsPermission()
      ? label + " - " + _("Grant access")
      : (label.empty() ? name : label);

    std::string second_text;
    const AllocatedPath mount{name.c_str()};
    if (IsContentUri(mount)) {
      const auto subfolder = ExtractSafSubfolder(name);
      if (!subfolder.empty())
        second_text = subfolder;
    } else if (!dev_id.empty() && dev_id != first_text &&
               !IsSafDeviceId(dev_id))
      second_text = dev_id;
    else if (name != first_text)
      second_text = name;

    row_renderer_.DrawFirstRow(canvas, rc, first_text.c_str());

    if (const auto space = dev.GetSpace()) {
      char free_buf[64], total_buf[64];
      FormatByteSize(free_buf, sizeof(free_buf), space->free_bytes, false);
      FormatByteSize(total_buf, sizeof(total_buf), space->total_bytes, false);
      StaticString<128> space_text;
      space_text.Format("%s / %s", free_buf, total_buf);
      row_renderer_.DrawRightSecondRow(canvas, rc, space_text.c_str());
    }

    if (!second_text.empty())
      row_renderer_.DrawSecondRow(canvas, rc, second_text.c_str());
  }

  bool CanActivateItem([[maybe_unused]] unsigned) const noexcept override {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned) noexcept override {
    if (dialog_ != nullptr)
      dialog_->SetModalResult(mrOK);
  }
};

} // anonymous namespace

const AllocatedPath &
GetLastStorageTarget() noexcept
{
  return last_storage_target;
}

void
SetLastStorageTarget(const AllocatedPath &p) noexcept
{
  last_storage_target = Path(p);
}

AllocatedPath
PickStorageLocation() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  while (true) {
    auto *list_widget = new StorageListWidget();

    WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                        look, _("Choose location"));
    list_widget->SetDialog(dialog);

    auto *select_btn = dialog.AddButton(_("Select"), mrOK);
    list_widget->SetSelectButton(*select_btn);
#ifdef ANDROID
    static constexpr int mrChangeFolder = 100;
    dialog.AddButton(_("Change folder"), mrChangeFolder);
#endif
    /* "Scan" is kept for manual refresh; hotplug also triggers Refresh(). */
    dialog.AddButton(_("Scan"), [list_widget]{ list_widget->ScanAndReport(); });
    dialog.AddButton(_("Cancel"), mrCancel);
    dialog.EnableCursorSelection();
    dialog.FinishPreliminary(list_widget);

    const int result = dialog.ShowModal();

#ifdef ANDROID
    if (result == mrChangeFolder) {
      const unsigned sel = list_widget->GetCursorIndex();
      auto &opts = list_widget->GetOptions();
      if (sel < opts.size()) {
        const auto &dev = opts[sel];
        if (IsContentUri(AllocatedPath(dev->Name().c_str())))
          dev->RequestPermission();
      }
      continue;
    }
#endif

    if (result != mrOK)
      return AllocatedPath();

    const unsigned sel = list_widget->GetCursorIndex();
    auto &opts = list_widget->GetOptions();
    if (sel >= opts.size())
      return AllocatedPath();

    const auto &chosen = opts[sel];

    if (chosen->NeedsPermission()) {
      chosen->RequestPermission();
      continue;
    }

    AllocatedPath chosen_path{chosen->Name().c_str()};
    if (chosen_path != nullptr)
      last_storage_target = Path(chosen_path);
    return chosen_path;
  }
}
