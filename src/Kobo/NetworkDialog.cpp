// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetworkDialog.hpp"
#include "WifiDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "ui/event/KeyCode.hpp"
#include "Language/Language.hpp"
#include "Form/Form.hpp"
#include "Widget/RowFormWidget.hpp"
#include "System.hpp"

[[gnu::pure]]
static const char *
GetWifiToggleCaption()
{
  return IsKoboWifiOn() ? _T("Wifi OFF") : _T("Wifi ON");
}

class NetworkWidget final
  : public RowFormWidget {

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
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

private:
  void ToggleWifi();
};

void
NetworkWidget::UpdateButtons()
{
  toggle_wifi_button->SetCaption(GetWifiToggleCaption());
  wifi_button->SetEnabled(IsKoboWifiOn());
}

void
NetworkWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  toggle_wifi_button = AddButton(GetWifiToggleCaption(),
                                 [this](){ ToggleWifi(); });

  wifi_button = AddButton(_("Wifi"), [](){ ShowWifiDialog(); });

  AddButton(_T("Telnet server"), [](){ KoboRunTelnetd(); });

  AddButton(_T("Ftp server"), [](){ KoboRunFtpd(); });

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
ShowNetworkDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<NetworkWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look, _("Network"));
  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget(look);
  dialog.ShowModal();
}
