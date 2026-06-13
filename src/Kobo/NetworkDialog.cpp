// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetworkDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/WifiDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Kobo/PlatformWifiBackend.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "net/wifi/WifiError.hpp"
#include "System.hpp"

[[gnu::pure]]
static const char *
GetWifiToggleCaption() noexcept
{
  return IsKoboWifiOn() ? _("WiFi Off") : _("WiFi On");
}

class NetworkWidget final
  : public RowFormWidget {

  enum Buttons {
    TOGGLE_WIFI,
    WIFI,
    TELNET,
    FTP,
  };

  Button *toggle_wifi_button{nullptr};

public:
  NetworkWidget(const DialogLook &look):RowFormWidget(look) {}

  void UpdateButtons() noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

private:
  void ToggleWifi() noexcept;
};

void
NetworkWidget::UpdateButtons() noexcept
{
  if (toggle_wifi_button != nullptr)
    toggle_wifi_button->SetCaption(GetWifiToggleCaption());
}

void
NetworkWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  toggle_wifi_button = AddButton(GetWifiToggleCaption(), [this]() {
    ToggleWifi();
  });

  AddButton(_("WiFi"), []() {
    try {
      auto backend = CreatePlatformWifiBackend();
      if (backend == nullptr) {
        ShowMessageBox(_("WiFi service is not available."), _("WiFi"), MB_OK);
        return;
      }

      ShowWifiDialog(std::move(backend));
    } catch (...) {
      const auto message = WifiError::Format(std::current_exception());
      ShowMessageBox(message.c_str(), _("WiFi"), MB_OK);
    }
  });

  AddButton("Telnet server", [](){ KoboRunTelnetd(); });

  AddButton("Ftp server", [](){ KoboRunFtpd(); });

  UpdateButtons();
}

void
NetworkWidget::ToggleWifi() noexcept
{
  if (!IsKoboWifiOn())
    KoboWifiOn();
  else
    KoboWifiOff();

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
