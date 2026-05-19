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
#include <stdexcept>
#include <string>

#if defined(KOBO)
#include "Kobo/PlatformWifiBackend.hpp"
#include "Kobo/System.hpp"
#include "net/wifi/WifiError.hpp"
#endif

#if defined(HAVE_LINUX_NET_WIFI)
#include "net/State.hpp"
#include "net/wifi/LinuxWifiBackend.hpp"
#include "net/wifi/WifiError.hpp"
#endif

#if defined(KOBO)

class KoboNetworkConfigWidget final
  : public RowFormWidget, public DataFieldListener {
  unsigned row_status{0}, row_ip{0}, row_backend{0}, row_radio{0};

public:
  KoboNetworkConfigWidget()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void OnRefresh() noexcept;
  void OnModified(DataField &df) noexcept override;
};

void
KoboNetworkConfigWidget::Prepare(ContainerWindow &parent,
                                 const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  unsigned n = 0U;
  row_status = n++;
  AddReadOnly(
    _("Status"),
    _("This page shows Kobo WiFi status. Open WiFi list to scan and connect."),
    _("Checking WiFi..."));
  row_ip = n++;
  AddReadOnly(
    _("IP address"),
    _("IPv4 address of the active WiFi interface."),
    _("Unknown"));
  row_backend = n++;
  AddReadOnly(
    _("Backend"),
    _("WiFi on Kobo is managed by wpa_supplicant."),
    "wpa_supplicant");
  row_radio = n++;
  AddBoolean(
    _("WiFi Enabled"),
    _("Turns the Kobo WiFi interface on or off."),
    IsKoboWifiOn(), this);

  AddButton(_("WiFi List"), [this]() {
    try {
      auto backend = CreatePlatformWifiBackend();
      if (backend == nullptr) {
        ShowMessageBox(
          _("WiFi backend is not available in this Kobo build."),
          _("Network"), MB_OK);
        return;
      }

      ShowWifiDialog(std::move(backend));
      OnRefresh();
    } catch (...) {
      const auto message = WifiError::Format(std::current_exception());
      ShowMessageBox(message.c_str(), _("Network"), MB_OK);
    }
  });

  OnRefresh();
}

void
KoboNetworkConfigWidget::OnRefresh() noexcept
{
  SetText(row_backend, "wpa_supplicant");
  LoadValue(row_radio, IsKoboWifiOn());

  if (!IsKoboWifiOn()) {
    SetText(row_status, _("Disabled"));
    SetText(row_ip, _("Unknown"));
    return;
  }

  try {
    auto backend = CreatePlatformWifiBackend();
    if (backend == nullptr) {
      SetText(row_status, _("WiFi backend is not available in this Kobo build."));
      SetText(row_ip, _("Unknown"));
      return;
    }

    const auto status = backend->GetBackendStatus();
    SetText(row_status, WifiBackendStatus::Format(status));
    SetText(row_ip, WifiBackendStatus::FormatIpAddress(status));
  } catch (...) {
    const auto message = WifiError::Format(std::current_exception());
    SetText(row_status, message.c_str());
    SetText(row_ip, _("Unknown"));
  }
}

void
KoboNetworkConfigWidget::OnModified(DataField &df) noexcept
{
  if (!IsDataField(row_radio, df))
    return;

  try {
    const bool enabled = GetValueBoolean(row_radio);
    const bool success = enabled ? KoboWifiOn() : KoboWifiOff();
    if (!success)
      throw std::runtime_error{enabled
        ? _("Failed to enable WiFi.")
        : _("Failed to disable WiFi.")};

    OnRefresh();
  } catch (...) {
    const auto message = WifiError::Format(std::current_exception());
    ShowMessageBox(message.c_str(), _("Network"), MB_OK);
    OnRefresh();
  }
}

bool
KoboNetworkConfigWidget::Save(bool &changed) noexcept
{
  (void)changed;
  return true;
}

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

  return _("Unknown");
}

class NetworkConfigWidget final
  : public RowFormWidget, public DataFieldListener {
  LinuxWifiBackendKind backend_kind = LinuxWifiBackendKind::None;

  unsigned row_status{0}, row_connectivity{0}, row_ip{0};
  unsigned row_backend{0}, row_radio{0};
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
  row_connectivity = n++;
  AddReadOnly(
    _("Connectivity"),
    _("Current network connectivity state."),
    NetStateText::ToString(NetState::UNKNOWN));
  row_ip = n++;
  AddReadOnly(
    _("IP address"),
    _("IPv4 address of the active WiFi interface."),
    _("Unknown"));
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
    } catch (...) {
      const auto message = WifiError::Format(std::current_exception());
      ShowMessageBox(message.c_str(), _("Network"), MB_OK);
    }
  });

  OnRefresh();
}

void
NetworkConfigWidget::OnRefresh() noexcept
{
  const auto set_error_state = [this](const char *status) {
    SetText(row_status, status);
    SetText(row_connectivity, NetStateText::ToString(NetState::UNKNOWN));
    SetText(row_ip, _("Unknown"));
    SetText(row_backend, _("Unknown"));
  };

  try {
    backend_kind = QueryLinuxWifiBackendKind();
    SetText(row_backend, BackendName(backend_kind));
    SetText(row_connectivity, NetStateText::ToString(GetNetState()));

    auto backend = CreateLinuxWifiBackend(backend_kind);
    if (backend == nullptr) {
      SetText(row_status,
              _("No network service (NetworkManager or ConnMan) found on D-Bus."));
      SetText(row_ip, _("Unknown"));
    } else {
      const auto status = backend->GetBackendStatus();
      SetText(row_status, WifiBackendStatus::Format(status));
      SetText(row_ip, WifiBackendStatus::FormatIpAddress(status));
    }

    if (have_radio)
      LoadValue(row_radio, GetLinuxWifiRadioEnabled(backend_kind));
  } catch (...) {
    const auto message = WifiError::Format(std::current_exception());
    set_error_state(message.c_str());
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
    } catch (...) {
      const auto message = WifiError::Format(std::current_exception());
      ShowMessageBox(message.c_str(), _("Network"), MB_OK);
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

#if !defined(HAVE_LINUX_NET_WIFI) && !defined(KOBO)
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
#if defined(KOBO)
  return std::make_unique<KoboNetworkConfigWidget>();
#elif defined(HAVE_LINUX_NET_WIFI)
  return std::make_unique<NetworkConfigWidget>();
#else
  return std::make_unique<NetworkConfigStub>();
#endif
}
