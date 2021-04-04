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
#include "Android/LeScanCallback.hpp"
#include "Android/BluetoothHelper.hpp"
#include "Language/Language.hpp"
#include "thread/Mutex.hxx"
#include "ui/event/Notify.hpp"
#include "util/StringCompare.hxx"

#include <vector>
#include <forward_list>
#include <set>

class ScanBluetoothLeWidget final
  : public ListWidget, public LeScanCallback {

  struct Item {
    std::string address;
    std::string name;

    Item(const char *_address, const char *_name)
      :address(_address), name(_name) {}
  };

  WidgetDialog &dialog;

  UI::Notify le_scan_notify{[this]{ OnLeScanNotification(); }};

  std::vector<Item> items;

  TextRowRenderer row_renderer;

  Mutex mutex;
  std::set<std::string> addresses;
  std::forward_list<Item> new_items;

  Button *select_button;

public:
  explicit ScanBluetoothLeWidget(WidgetDialog &_dialog)
    :dialog(_dialog) {}

  gcc_pure
  const auto &GetSelectedAddress() const {
    return items[GetList().GetCursorIndex()].address;
  }

  void CreateButtons() {
    select_button = dialog.AddButton(_("Select"), mrOK);
    select_button->SetEnabled(false);
  }

  void UpdateButtons() {
    select_button->SetEnabled(!items.empty());
  }

private:
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

  /* virtual methods from class LeScanCallback */
  void OnLeScan(const char *address, const char *name) override {
    {
      std::string address2(address);
      if (addresses.find(address2) != addresses.end())
        /* already in the list */
        return;

      addresses.emplace(std::move(address2));
    }

    {
      const std::lock_guard<Mutex> lock(mutex);
      new_items.emplace_front(address, name);
    }

    le_scan_notify.SendNotification();
  };

  void OnLeScanNotification() noexcept {
    const bool was_empty = items.empty();

    {
      const std::lock_guard<Mutex> lock(mutex);

      if (new_items.empty())
        return;

      do {
        items.emplace_back(std::move(new_items.front()));
        new_items.pop_front();
      } while (!new_items.empty());
    }

    GetList().SetLength(items.size());

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
}

std::string
ScanBluetoothLeDialog() noexcept
{
  TWidgetDialog<ScanBluetoothLeWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Bluetooth LE"));
  dialog.SetWidget(dialog);

  const auto env = Java::GetEnv();
  const auto callback = BluetoothHelper::StartLeScan(env, dialog.GetWidget());
  if (callback == nullptr) {
    const TCHAR *message =
      _("Bluetooth LE is not available on this device.");
    ShowMessageBox(message, _("Bluetooth LE"), MB_OK);
    return {};
  }

  dialog.GetWidget().CreateButtons();
  dialog.AddButton(_("Cancel"), mrCancel);

  int result = dialog.ShowModal();
  BluetoothHelper::StopLeScan(env, callback);

  if (result != mrOK)
    return {};

  return dialog.GetWidget().GetSelectedAddress();
}
