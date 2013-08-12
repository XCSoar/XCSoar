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

#include "NetworkDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Screen/Init.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "../test/src/Fonts.hpp"
#include "Language/Language.hpp"
#include "Form/Form.hpp"
#include "Form/ActionListener.hpp"
#include "Widget/RowFormWidget.hpp"
#include "System.hpp"

class NetworkWidget final
  : public RowFormWidget, ActionListener {
  enum Buttons {
    WIFI,
    TELNET,
  };

  WndButton *wifi_button;

public:
  NetworkWidget(const DialogLook &look):RowFormWidget(look) {}

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;

private:
  void ToggleWifi();

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;
};

void
NetworkWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  wifi_button = AddButton(IsKoboWifiOn() ? _T("Wifi OFF") : _T("Wifi ON"),
                          *this, WIFI);

  AddButton(_T("Telnet server"), *this, TELNET);
}

void
NetworkWidget::ToggleWifi()
{
  if (!IsKoboWifiOn()) {
    wifi_button->SetCaption(("Wifi OFF"));
    KoboWifiOn();
  } else {
    wifi_button->SetCaption(("Wifi ON"));
    KoboWifiOff();
  }
}

void
NetworkWidget::OnAction(int id)
{
  switch (id) {
  case WIFI:
    ToggleWifi();
    break;

  case TELNET:
    KoboRunTelnetd();
    break;
  }
}

void
ShowNetworkDialog(SingleWindow &main_window, const DialogLook &look)
{
  NetworkWidget widget(look);
  WidgetDialog dialog(look);
  dialog.CreateFull(main_window, _("Network"), &widget);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  dialog.StealWidget();
}
