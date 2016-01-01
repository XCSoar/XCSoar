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

#include "NetworkDialog.hpp"
#include "WifiDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Event/KeyCode.hpp"
#include "Language/Language.hpp"
#include "Form/Form.hpp"
#include "Form/ActionListener.hpp"
#include "Widget/RowFormWidget.hpp"
#include "System.hpp"

gcc_pure
static const TCHAR *
GetWifiToggleCaption()
{
  return IsKoboWifiOn() ? _T("Wifi OFF") : _T("Wifi ON");
}

class NetworkWidget final
  : public RowFormWidget, ActionListener {
  enum Buttons {
    TOGGLE_WIFI,
    WIFI,
    TELNET,
    FTP,
  };

  Button *toggle_wifi_button, *wifi_button;

public:
  NetworkWidget(const DialogLook &look):RowFormWidget(look) {}

  void UpdateButtons();

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;

private:
  void ToggleWifi();

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;
};

void
NetworkWidget::UpdateButtons()
{
  toggle_wifi_button->SetCaption(GetWifiToggleCaption());
  wifi_button->SetEnabled(IsKoboWifiOn());
}

void
NetworkWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  toggle_wifi_button = AddButton(GetWifiToggleCaption(),
                                 *this, TOGGLE_WIFI);

  wifi_button = AddButton(_("Wifi"), *this, WIFI);

  AddButton(_T("Telnet server"), *this, TELNET);

  AddButton(_T("Ftp server"), *this, FTP);

  UpdateButtons();
}

void
NetworkWidget::ToggleWifi()
{
  if (!IsKoboWifiOn()) {
    KoboWifiOn();
  } else {
    KoboWifiOff();
  }

  UpdateButtons();
}

void
NetworkWidget::OnAction(int id)
{
  switch (id) {
  case TOGGLE_WIFI:
    ToggleWifi();
    break;

  case WIFI:
    ShowWifiDialog();
    break;

  case TELNET:
    KoboRunTelnetd();
    break;

  case FTP:
    KoboRunFtpd();
    break;
  }
}

void
ShowNetworkDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  NetworkWidget widget(look);
  WidgetDialog dialog(look);
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Network"), &widget);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  dialog.StealWidget();
}
