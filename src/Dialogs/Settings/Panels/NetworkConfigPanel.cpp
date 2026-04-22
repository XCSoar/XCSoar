// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetworkConfigPanel.hpp"
#include "UIGlobals.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "Dialogs/Message.hpp"
#include "Form/DataField/Listener.hpp"

#include <memory>
#include <string>

#if defined(HAVE_LINUX_NET_WIFI)
#include "net/wifi/LinuxWifiDialog.hpp"
#include "net/wifi/WifiService.hpp"
#endif

#if defined(HAVE_LINUX_NET_WIFI)

static const char *
BackendName(WifiService::Backend b) noexcept
{
  switch (b) {
  case WifiService::Backend::None:
    return _("None");
  case WifiService::Backend::NetworkManager:
    return _("NetworkManager");
  case WifiService::Backend::ConnMan:
    return _("ConnMan");
  }
  return "";
}

class NetworkConfigWidget final
  : public RowFormWidget, public DataFieldListener {
  std::unique_ptr<WifiService> service;

  unsigned row_status{0}, row_backend{0}, row_radio{0};
  bool have_radio{false};

public:
  NetworkConfigWidget()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void OnRefresh() noexcept;

  /* DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
NetworkConfigWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  service = std::make_unique<WifiService>();
  if (!service->Detect()) {
    AddReadOnly(
      _("Status"),
      _("This page shows WiFi status when NetworkManager or ConnMan is on "
        "D-Bus (e.g. Linux with Wayland or KMS). Open WiFi list to scan and "
        "connect when a service is available."),
      _("No network service (NetworkManager or ConnMan) found on D-Bus."));
    AddReadOnly(_("Backend"), nullptr, BackendName(WifiService::Backend::None));
    return;
  }

  have_radio = (service->GetBackend() == WifiService::Backend::NetworkManager);
  unsigned n = 0U;
  row_status = n++;
  AddReadOnly(_("Status"), nullptr, service->GetStatusLine().c_str());
  row_backend = n++;
  AddReadOnly(
    _("Backend"),
    _("D-Bus provider used for WiFi (see WiFi list to manage networks)."),
    BackendName(service->GetBackend()));
  if (have_radio) {
    row_radio = n++;
    const bool w = true;
    AddBoolean(_("WiFi enabled"), nullptr, w, this);
  }
  AddButton(_("WiFi list"), [this]() {
    if (service) {
      ShowLinuxWifiDialog(*service);
      OnRefresh();
    }
  });
}

void
NetworkConfigWidget::OnRefresh() noexcept
{
  if (service == nullptr) {
    return;
  }
  try {
    SetText(row_status, service->GetStatusLine().c_str());
    SetText(row_backend, BackendName(service->GetBackend()));
  } catch (...) {
  }
}

void
NetworkConfigWidget::OnModified(DataField &df) noexcept
{
  if (service == nullptr) {
    return;
  }
  if (have_radio && IsDataField(row_radio, df) &&
      service->GetBackend() == WifiService::Backend::NetworkManager) {
    try {
      const bool o = GetValueBoolean(row_radio);
      service->SetRadio(o);
      SetText(row_status, service->GetStatusLine().c_str());
    } catch (const std::exception &e) {
      ShowMessageBox(e.what(), _("Network"), MB_OK);
    } catch (...) {
    }
  } else {
    (void)df;
  }
}

bool
NetworkConfigWidget::Save(bool &changed) noexcept
{
  (void)changed;
  return false;
}

#endif /* HAVE_LINUX_NET_WIFI */

#if !defined(HAVE_LINUX_NET_WIFI)
class NetworkConfigStub final : public RowFormWidget {
public:
  explicit NetworkConfigStub()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override
  {
    RowFormWidget::Prepare(parent, rc);
    AddReadOnly(
      _("Status"),
      nullptr,
      _("In-app network settings are not available in this build."));
  }
};
#endif

std::unique_ptr<Widget>
CreateNetworkConfigPanel()
{
#if defined(HAVE_LINUX_NET_WIFI)
  return std::make_unique<NetworkConfigWidget>();
#else
  return std::make_unique<NetworkConfigStub>();
#endif
}
