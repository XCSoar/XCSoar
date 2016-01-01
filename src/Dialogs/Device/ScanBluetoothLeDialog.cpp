/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Screen/Layout.hpp"
#include "Screen/Canvas.hpp"
#include "Form/Button.hpp"
#include "Widget/ListWidget.hpp"
#include "Java/Global.hxx"
#include "Android/LeScanCallback.hpp"
#include "Android/BluetoothHelper.hpp"
#include "Language/Language.hpp"
#include "Thread/Mutex.hpp"
#include "Event/Notify.hpp"
#include "Util/StringCompare.hxx"

#include <string>
#include <vector>
#include <forward_list>
#include <set>

class ScanBluetoothLeWidget final
  : public ListWidget, public LeScanCallback, Notify {

  struct Item {
    std::string address;
    std::string name;

    Item(const char *_address, const char *_name)
      :address(_address), name(_name) {}
  };

  WidgetDialog &dialog;

  std::vector<Item> items;

  Mutex mutex;
  std::set<std::string> addresses;
  std::forward_list<Item> new_items;

  Button *select_button;

public:
  explicit ScanBluetoothLeWidget(WidgetDialog &_dialog)
    :dialog(_dialog) {}

  gcc_pure
  const char *GetSelectedAddress() const {
    return items[GetList().GetCursorIndex()].address.c_str();
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
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  void Unprepare() override;

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned idx) override;

  /* virtual methods from class ListCursorHandler */
  bool CanActivateItem(unsigned index) const override {
    return true;
  }

  void OnActivateItem(unsigned index) override {
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
      const ScopeLock protect(mutex);
      new_items.emplace_front(address, name);
    }

    Notify::SendNotification();
  };

  /* virtual methods from class Notify */
  void OnNotification() override {
    const bool was_empty = items.empty();

    {
      const ScopeLock protect(mutex);

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
ScanBluetoothLeWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  const unsigned margin = Layout::GetTextPadding();
  const unsigned font_height = look.list.font->GetHeight();

  unsigned row_height = std::max(2u * margin + font_height,
                                 Layout::GetMaximumControlHeight());
  CreateList(parent, look, rc, row_height);
}

void
ScanBluetoothLeWidget::Unprepare()
{
  Notify::ClearNotification();

  DeleteWindow();
}

void
ScanBluetoothLeWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                   unsigned i)
{
  const auto &item = items[i];

  const unsigned margin = Layout::GetTextPadding();

  const char *name = item.name.c_str();
  if (StringIsEmpty(name))
    name = item.address.c_str();

  canvas.DrawText(rc.left + margin, rc.top + margin, name);
}

bool
ScanBluetoothLeDialog(char *address, size_t address_size)
{
  WidgetDialog dialog(UIGlobals::GetDialogLook());
  ScanBluetoothLeWidget widget(dialog);

  const auto env = Java::GetEnv();
  const auto callback = BluetoothHelper::StartLeScan(env, widget);
  if (callback == nullptr) {
    const TCHAR *message =
      _("Bluetooth LE is not available on this device.");
    ShowMessageBox(message, _("Bluetooth LE"), MB_OK);
    return false;
  }

  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Bluetooth LE"), &widget);
  widget.CreateButtons();
  dialog.AddButton(_("Cancel"), mrCancel);

  int result = dialog.ShowModal();
  BluetoothHelper::StopLeScan(env, callback);

  dialog.StealWidget();

  if (result != mrOK)
    return false;

  CopyString(address, widget.GetSelectedAddress(), address_size);
  return true;
}
