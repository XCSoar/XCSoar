/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "ScanBluetoothLeDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Form/Button.hpp"
#include "Widget/ListWidget.hpp"
#include "java/Global.hxx"
#include "Android/DetectDeviceListener.hpp"
#include "Android/BluetoothHelper.hpp"
#include "Language/Language.hpp"
#include "thread/Mutex.hxx"
#include "ui/event/Notify.hpp"
#include "util/StringCompare.hxx"

#include <vector>

class ScanBluetoothLeWidget final
  : public ListWidget, public DetectDeviceListener {

  struct Item {
    std::string address;
    std::string name;

    uint64_t features = 0;

    bool IsSame(const Item &other) const noexcept {
      return address == other.address;
    }

    void Update(Item &&src) noexcept {
      if (name.empty())
        name = std::move(src.name);

      features |= src.features;
    }
  };

  WidgetDialog &dialog;

  UI::Notify le_scan_notify{[this]{ OnLeScanNotification(); }};

  std::vector<Item> items;

  TextRowRenderer row_renderer;

  Mutex mutex;
  std::vector<Item> new_items;

  Button *select_button;

public:
  explicit ScanBluetoothLeWidget(WidgetDialog &_dialog)
    :dialog(_dialog) {}

  [[gnu::pure]]
  std::pair<std::string, bool> GetResult() && {
    auto &i = items[GetList().GetCursorIndex()];
    const bool is_hm10 = (i.features & DetectDeviceListener::FEATURE_HM10) != 0;
    return {std::move(i.address), is_hm10};
  }

  void CreateButtons() {
    select_button = dialog.AddButton(_("Select"), mrOK);
    select_button->SetEnabled(false);
  }

  void UpdateButtons() {
    select_button->SetEnabled(!items.empty());
  }

private:
  void UpdateItem(std::vector<Item> &v, Item &&new_item) noexcept {
    for (auto &i : v) {
      if (i.IsSame(new_item)) {
        i.Update(std::move(new_item));
        return;
      }
    }

    v.emplace_back(std::move(new_item));
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from class ListCursorHandler */
  bool CanActivateItem(unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override {
    dialog.SetModalResult(mrOK);
  }

  /* virtual methods from class DetectDeviceListener */
  void OnDeviceDetected(Type type, const char *address,
                        const char *name,
                        uint64_t features) noexcept override {
    if (name == nullptr)
      name = address;

    Item item{address, name, features};

    {
      const std::lock_guard<Mutex> lock(mutex);
      UpdateItem(new_items, std::move(item));
    }

    le_scan_notify.SendNotification();
  };

  void OnLeScanNotification() noexcept {
    const bool was_empty = items.empty();

    {
      const std::lock_guard<Mutex> lock(mutex);

      if (new_items.empty())
        return;

      for (auto &i : new_items)
        UpdateItem(items, std::move(i));

      new_items.clear();
    }

    GetList().SetLength(items.size());
    GetList().Invalidate();

    if (was_empty)
      /* the list has just become non-empty, so allow pressing the
         "select" button */
      select_button->SetEnabled(true);
  }
};

void
ScanBluetoothLeWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font));
}

void
ScanBluetoothLeWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                   unsigned i) noexcept
{
  const auto &item = items[i];

  const char *name = item.name.c_str();
  if (StringIsEmpty(name))
    name = item.address.c_str();

  row_renderer.DrawTextRow(canvas, rc, name);

  // TODO: properly render this, consider all feature flags
  if (item.features & DetectDeviceListener::FEATURE_HM10)
    row_renderer.DrawRightColumn(canvas, rc, _T("HM10"));
  else if (item.features & DetectDeviceListener::FEATURE_HEART_RATE)
    row_renderer.DrawRightColumn(canvas, rc, _T("HR"));
  else if (item.features & DetectDeviceListener::FEATURE_FLYTEC_SENSBOX)
    row_renderer.DrawRightColumn(canvas, rc, _T("Sensbox"));
  else
    row_renderer.DrawRightColumn(canvas, rc, _("Unsupported"));
}

std::pair<std::string, bool>
ScanBluetoothLeDialog(BluetoothHelper &bluetooth_helper) noexcept
{
  TWidgetDialog<ScanBluetoothLeWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Bluetooth LE"));
  dialog.SetWidget(dialog);

  const auto env = Java::GetEnv();
  const auto listener =
    bluetooth_helper.AddDetectDeviceListener(env, dialog.GetWidget());

  dialog.GetWidget().CreateButtons();
  dialog.AddButton(_("Cancel"), mrCancel);

  int result = dialog.ShowModal();
  bluetooth_helper.RemoveDetectDeviceListener(env, listener);

  if (result != mrOK)
    return {{}, false};

  return std::move(dialog.GetWidget()).GetResult();
}
