// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StorageLocationPickerDialog.hpp"

#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "system/Path.hpp"
#include "Storage/PlatformStorageMonitor.hpp"
#include "Storage/StorageEvents.hpp"
#include "Storage/StorageManager.hpp"
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
#include <cstdint>

namespace {

static AllocatedPath last_storage_target;

using DeviceList = std::vector<std::shared_ptr<StorageDevice>>;

DeviceList
DiscoverTargets() noexcept
{
  DeviceList result;

  auto monitor = CreatePlatformStorageMonitor();
  if (!monitor)
    return result;

  for (auto &dev : monitor->Enumerate()) {
    if (dev && dev->IsWritable() && !dev->Name().empty())
      result.push_back(std::move(dev));
  }

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
    const std::string mount_text = dev.Name();
    const std::string label = dev.Label();

    const std::string &first_text = label.empty() ? mount_text : label;

    std::string second_text;
    const std::string dev_id = dev.Id();
    if (!dev_id.empty() && dev_id != first_text)
      second_text = dev_id;
    else if (mount_text != first_text)
      second_text = mount_text;

    row_renderer_.DrawFirstRow(canvas, rc, first_text.c_str());

    if (auto space = dev.GetSpace()) {
      char free_buf[16], total_buf[16];
      FormatByteSize(free_buf, sizeof(free_buf), space->free_bytes, false);
      FormatByteSize(total_buf, sizeof(total_buf), space->total_bytes, false);
      char space_buf[40];
      snprintf(space_buf, sizeof(space_buf), "%s / %s", free_buf, total_buf);
      row_renderer_.DrawRightFirstRow(canvas, rc, space_buf);
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
  auto *list_widget = new StorageListWidget();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Choose location"));
  list_widget->SetDialog(dialog);

  auto *select_btn = dialog.AddButton(_("Select"), mrOK);
  list_widget->SetSelectButton(*select_btn);
  dialog.AddButton(_("Scan"), [list_widget]{ list_widget->ScanAndReport(); });
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.EnableCursorSelection();
  dialog.FinishPreliminary(list_widget);

  const int result = dialog.ShowModal();

  if (result != mrOK)
    return AllocatedPath();

  const unsigned sel = list_widget->GetCursorIndex();
  auto &opts = list_widget->GetOptions();
  if (sel >= opts.size())
    return AllocatedPath();

  AllocatedPath chosen{opts[sel]->Name().c_str()};
  if (chosen != nullptr)
    last_storage_target = Path(chosen);
  return chosen;
}
