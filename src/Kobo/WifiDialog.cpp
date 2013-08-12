/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "WifiDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "UIGlobals.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Form/ActionListener.hpp"
#include "Widget/ListWidget.hpp"
#include "WPASupplicant.hpp"

class WifiListWidget final
  : public ListWidget, ActionListener, Timer {
  enum Buttons {
    SCAN,
    CONNECT,
  };

  WndButton *connect_button;

  WifiStatus status;
  TrivialArray<WifiNetworkInfo, 64> networks;

  WPASupplicant wpa_supplicant;

public:
  void CreateButtons(WidgetDialog &dialog) {
    dialog.AddButton(_("Scan"), *this, SCAN);
    connect_button = dialog.AddButton(_("Connect"), *this, CONNECT);
  }

  void UpdateButtons() {
    connect_button->SetEnabled(!networks.empty());
  }

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override {
    const DialogLook &look = UIGlobals::GetDialogLook();
    CreateList(parent, look, rc, Layout::GetMaximumControlHeight());
    UpdateList();
    Timer::Schedule(1000);
  }

  virtual void Unprepare() override {
    Timer::Cancel();
    DeleteWindow();
  }

  /* virtual methods from class ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;

  /* virtual methods from class ListCursorHandler */
  virtual void OnCursorMoved(unsigned index) {
    UpdateButtons();
  }

  /* virtual methods from class Timer */
  virtual void OnTimer() {
    UpdateList();
  }

private:
  /**
   * Ensure that we're connected to wpa_supplicant.
   */
  bool EnsureConnected();
  void UpdateList();

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;
};

void
WifiListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                            unsigned idx)
{
  const WifiNetworkInfo &info = networks[idx];
  const unsigned padding = Layout::GetTextPadding();

  canvas.DrawText(rc.left + padding, rc.top + padding, info.ssid);

  if (StringIsEqual(info.bssid, status.bssid)) {
    const TCHAR *text = _("Connected");
    unsigned width = canvas.CalcTextWidth(text);
    canvas.DrawText(rc.right - padding - width, rc.top + padding, text);
  }
}

void
WifiListWidget::OnAction(int id)
{
  switch (id) {
  case SCAN:
    if (EnsureConnected() && wpa_supplicant.Scan())
      UpdateList();
    break;

  case CONNECT:
    // TODO
    ShowMessageBox(_T("Not yet implemented"), _("Connect"), MB_OK);
    break;
  }
}

bool
WifiListWidget::EnsureConnected()
{
  return wpa_supplicant.IsConnected() ||
    wpa_supplicant.Connect("/var/run/wpa_supplicant/eth0");
}

void
WifiListWidget::UpdateList()
{
  status.Clear();
  networks.clear();

  if (EnsureConnected()) {
    wpa_supplicant.Status(status);

    int n = wpa_supplicant.ScanResults(networks.begin(), networks.capacity());
    if (n >= 0)
      networks.resize(n);
  }

  GetList().SetLength(networks.size());

  UpdateButtons();
}

void
ShowWifiDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WifiListWidget widget;
  WidgetDialog dialog(look);
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Wifi"), &widget);
  widget.CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  dialog.StealWidget();
}
