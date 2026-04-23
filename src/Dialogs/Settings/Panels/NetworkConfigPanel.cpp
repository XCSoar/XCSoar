// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetworkConfigPanel.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/WifiDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "Dialogs/Message.hpp"
#include "Form/DataField/Listener.hpp"

#include <memory>
#include <string>

#if defined(HAVE_LINUX_NET_WIFI)
#include "net/wifi/LinuxWifiBackend.hpp"
#include "net/wifi/WifiFormat.hpp"
#endif

#if defined(HAVE_LINUX_NET_WIFI)

static const char *
BackendName(LinuxWifiBackendKind b) noexcept
{
  switch (b) {
  case LinuxWifiBackendKind::None:
    return _("None");
  case LinuxWifiBackendKind::NetworkManager:
    return _("NetworkManager");
  case LinuxWifiBackendKind::ConnMan:
    return _("ConnMan");
  }

  return "";
}

static StaticString<256>
FormatStatusLine(const WifiBackendStatus &status)
{
  StaticString<256> text;

  switch (status.state) {
  case WifiConnectionState::Connected:
    if (!status.ssid.empty())
      text.Format(_("Connected to %s"), status.ssid.c_str());
    else
      text = FormatWifiConnectionStateLabel(status.state);
    break;

  case WifiConnectionState::Connecting:
  case WifiConnectionState::Disconnected:
  case WifiConnectionState::Unknown:
    text = FormatWifiConnectionStateLabel(status.state);
    break;
  }

  return text;
}

class NetworkConfigWidget final
  : public RowFormWidget, public DataFieldListener {
  LinuxWifiBackendKind backend_kind = LinuxWifiBackendKind::None;

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

  unsigned n = 0U;
  row_status = n++;
  AddReadOnly(
    _("Status"),
    _("This page shows WiFi status when NetworkManager or ConnMan is on "
      "D-Bus (e.g. Linux with Wayland or KMS). Open WiFi list to scan and "
      "connect when a service is available."),
    _("Checking network services..."));
  row_backend = n++;
  AddReadOnly(
    _("Backend"),
    _("D-Bus provider used for WiFi (see WiFi list to manage networks)."),
    BackendName(LinuxWifiBackendKind::None));

  try {
    backend_kind = QueryLinuxWifiBackendKind();
    have_radio = HasLinuxWifiRadioToggle(backend_kind);
  } catch (...) {
    backend_kind = LinuxWifiBackendKind::None;
    have_radio = false;
  }

  if (have_radio) {
    row_radio = n++;
    AddBoolean(_("WiFi Enabled"), nullptr, false, this);
  }

  AddButton(_("WiFi List"), [this]() {
    try {
      auto backend = CreateLinuxWifiBackend();
      if (backend == nullptr) {
        ShowMessageBox(
          _("No network service (NetworkManager or ConnMan) found on D-Bus."),
          _("Network"), MB_OK);
        return;
      }

      ShowWifiDialog(std::move(backend));
      OnRefresh();
    } catch (const std::exception &e) {
      ShowMessageBox(e.what(), _("Network"), MB_OK);
    } catch (...) {
      ShowMessageBox(_("The operation failed."), _("Network"), MB_OK);
    }
  });

  OnRefresh();
}

void
NetworkConfigWidget::OnRefresh() noexcept
{
  try {
    backend_kind = QueryLinuxWifiBackendKind();
    SetText(row_backend, BackendName(backend_kind));

    auto backend = CreateLinuxWifiBackend();
    if (backend == nullptr) {
      SetText(row_status,
              _("No network service (NetworkManager or ConnMan) found on D-Bus."));
    } else {
      SetText(row_status, FormatStatusLine(backend->GetBackendStatus()));
    }

    if (have_radio)
      LoadValue(row_radio, GetLinuxWifiRadioEnabled(backend_kind));
  } catch (...) {
  }
}

void
NetworkConfigWidget::OnModified(DataField &df) noexcept
{
  if (have_radio && IsDataField(row_radio, df) &&
      backend_kind != LinuxWifiBackendKind::None) {
    try {
      SetLinuxWifiRadioEnabled(backend_kind, GetValueBoolean(row_radio));
      OnRefresh();
    } catch (const std::exception &e) {
      ShowMessageBox(e.what(), _("Network"), MB_OK);
    } catch (...) {
    }
  }
}

bool
NetworkConfigWidget::Save(bool &changed) noexcept
{
  (void)changed;
  return true;
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
