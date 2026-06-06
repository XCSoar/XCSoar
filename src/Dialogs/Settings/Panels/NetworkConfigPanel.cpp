// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetworkConfigPanel.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/WifiDialog.hpp"
#include "Form/DataField/Listener.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Widget/RowFormWidget.hpp"
#include "net/State.hpp"
#include "system/OpenLink.hpp"
#include "util/StaticString.hxx"

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(KOBO)
#include "Kobo/PlatformWifiBackend.hpp"
#include "Kobo/System.hpp"
#include "net/wifi/WifiError.hpp"
#endif

#if defined(HAVE_LINUX_NET_WIFI)
#include "net/wifi/LinuxWifiBackend.hpp"
#include "net/wifi/WifiError.hpp"
#endif

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "java/Global.hxx"
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
#include "net/IPv4Address.hxx"
#endif

struct NetworkConfigRows {
  unsigned status{0}, connectivity{0}, ip{0}, backend{0}, radio{0},
    persist_wifi{0};
  bool have_radio{false};
  bool have_persist_wifi{false};
};

struct NetworkConfigState {
  NetState connectivity{NetState::UNKNOWN};
  StaticString<256> status{_("Unknown")};
  StaticString<64> ip{_("Unknown")};
  StaticString<64> backend{_("Unknown")};
  bool have_radio_enabled{false};
  bool radio_enabled{false};
  bool have_persist_wifi_enabled{false};
  bool persist_wifi_enabled{false};
};

#if defined(HAVE_LINUX_NET_WIFI)
static const char *
LinuxBackendName(LinuxWifiBackendKind backend_kind) noexcept
{
  switch (backend_kind) {
  case LinuxWifiBackendKind::None:
    return _("None");
  case LinuxWifiBackendKind::NetworkManager:
    return "NetworkManager";
  case LinuxWifiBackendKind::ConnMan:
    return "ConnMan";
  }

  return _("Unknown");
}
#endif

#if defined(ANDROID) || (defined(__APPLE__) && TARGET_OS_IPHONE)
static StaticString<64>
GetPlatformWifiIpAddress() noexcept
{
  StaticString<64> text;
  text.clear();

#ifdef ANDROID
  if (native_view == nullptr)
    return text;

  native_view->GetWifiIpAddress(Java::GetEnv(),
                                text.buffer(), text.capacity());
#else
  char buffer[64];
  const auto address = IPv4Address::GetDeviceAddress("en0");
  if (address.IsDefined() &&
      address.ToString(buffer, sizeof(buffer)) != nullptr)
    text = buffer;
#endif

  return text;
}
#endif

#if defined(KOBO) || defined(HAVE_LINUX_NET_WIFI)
static const char *
GetWifiServiceUnavailableText() noexcept
{
  return _("WiFi service is not available.");
}
#endif

#if defined(ANDROID) || defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_IPHONE)
static const char *
GetManagedBySystemSettingsText() noexcept
{
  return _("Managed by system settings.");
}
#endif

static const char *
GetStatusHelp() noexcept
{
#if defined(KOBO) || defined(HAVE_LINUX_NET_WIFI)
  return _("This page shows WiFi status. Use WiFi list to scan and connect.");
#elif defined(ANDROID) || defined(_WIN32)
  return _("WiFi is managed by the system settings. Use WiFi list to open them.");
#elif defined(__APPLE__) && TARGET_OS_IPHONE
  return _("WiFi is managed by the system settings. Use WiFi list for instructions.");
#else
  return _("Network details are not available in this build.");
#endif
}

static const char *
GetBackendHelp() noexcept
{
#if defined(KOBO) || defined(HAVE_LINUX_NET_WIFI) || defined(ANDROID) || defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_IPHONE)
  return _("WiFi service used by the device.");
#else
  return _("Platform/backend information is not available in this build.");
#endif
}

static const char *
GetInitialBackendName() noexcept
{
#if defined(KOBO)
  return "wpa_supplicant";
#elif defined(HAVE_LINUX_NET_WIFI)
  return LinuxBackendName(LinuxWifiBackendKind::None);
#elif defined(ANDROID)
  return "Android";
#elif defined(_WIN32)
  return "Windows";
#elif defined(__APPLE__) && TARGET_OS_IPHONE
  return "iOS";
#else
  return _("Unavailable");
#endif
}

static void
PreparePlatformRows(RowFormWidget &widget, unsigned &n, NetworkConfigRows &rows,
                    DataFieldListener &listener) noexcept
{
#if defined(KOBO)
  rows.radio = n++;
  rows.have_radio = true;
  widget.AddBoolean(_("WiFi Enabled"),
                    _("Turns the Kobo WiFi interface on or off."),
                    IsKoboWifiOn(), &listener);
  rows.persist_wifi = n++;
  rows.have_persist_wifi = true;
  widget.AddBoolean(_("Auto WiFi"),
                    _("Enable WiFi automatically at startup."),
                    IsKoboWifiAutoOn(), &listener);
#elif defined(HAVE_LINUX_NET_WIFI)
  try {
    const auto backend_kind = QueryLinuxWifiBackendKind();
    rows.have_radio = HasLinuxWifiRadioToggle(backend_kind);
  } catch (...) {
    rows.have_radio = false;
  }

  if (rows.have_radio) {
    rows.radio = n++;
    widget.AddBoolean(_("WiFi Enabled"), nullptr, false, &listener);
  }
#else
  (void)widget;
  (void)n;
  (void)rows;
  (void)listener;
#endif
}

static void
OpenPlatformWifiList(std::function<void()> refresh) noexcept
{
#if defined(KOBO)
  try {
    auto backend = CreatePlatformWifiBackend();
    if (backend == nullptr) {
      ShowMessageBox(GetWifiServiceUnavailableText(), _("Network"), MB_OK);
      return;
    }

    ShowWifiDialog(std::move(backend));
    refresh();
  } catch (...) {
    const auto message = WifiError::Format(std::current_exception());
    ShowMessageBox(message.c_str(), _("Network"), MB_OK);
  }
#elif defined(HAVE_LINUX_NET_WIFI)
  try {
    auto backend = CreateLinuxWifiBackend();
    if (backend == nullptr) {
      ShowMessageBox(GetWifiServiceUnavailableText(), _("Network"), MB_OK);
      return;
    }

    ShowWifiDialog(std::move(backend));
    refresh();
  } catch (...) {
    const auto message = WifiError::Format(std::current_exception());
    ShowMessageBox(message.c_str(), _("Network"), MB_OK);
  }
#elif defined(ANDROID)
  if (native_view != nullptr && native_view->OpenWifiSettings(Java::GetEnv()))
    return;

  ShowMessageBox(_("Failed to open system settings."),
                 _("Connectivity"), MB_OK);
#elif defined(_WIN32)
  if (OpenLink("ms-settings:network-wifi"))
    return;

  ShowMessageBox(_("Failed to open system settings."),
                 _("Connectivity"), MB_OK);
#elif defined(__APPLE__) && TARGET_OS_IPHONE
  ShowMessageBox(_("Open the Settings app, then go to Wi-Fi."),
                 _("Connectivity"), MB_OK);
#else
  (void)refresh;
  ShowMessageBox(_("WiFi management is not available in this build."),
                 _("Connectivity"), MB_OK);
#endif

#if defined(ANDROID) || defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_IPHONE)
  (void)refresh;
#endif
}

static void
BuildPlatformState(NetworkConfigState &state,
                   const NetworkConfigRows &rows) noexcept
{
#if defined(KOBO)
  state.connectivity = GetNetState();
  state.backend = "wpa_supplicant";
  state.have_radio_enabled = true;
  state.radio_enabled = IsKoboWifiOn();
  state.have_persist_wifi_enabled = true;
  state.persist_wifi_enabled = IsKoboWifiAutoOn();

  if (!state.radio_enabled) {
    state.status = _("Disabled");
    return;
  }

  try {
    auto backend = CreatePlatformWifiBackend();
    if (backend == nullptr) {
      state.status = GetWifiServiceUnavailableText();
      return;
    }

    const auto status = backend->GetBackendStatus();
    state.status = WifiBackendStatus::Format(status);
    state.ip = WifiBackendStatus::FormatIpAddress(status);
  } catch (...) {
    const auto message = WifiError::Format(std::current_exception());
    state.status = message.c_str();
  }
#elif defined(HAVE_LINUX_NET_WIFI)
  try {
    const auto backend_kind = QueryLinuxWifiBackendKind();
    state.connectivity = GetNetState();
    state.backend = LinuxBackendName(backend_kind);

    auto backend = CreateLinuxWifiBackend(backend_kind);
    if (backend == nullptr) {
      state.status = GetWifiServiceUnavailableText();
    } else {
      const auto status = backend->GetBackendStatus();
      state.status = WifiBackendStatus::Format(status);
      state.ip = WifiBackendStatus::FormatIpAddress(status);
    }

    if (rows.have_radio) {
      state.have_radio_enabled = true;
      state.radio_enabled = GetLinuxWifiRadioEnabled(backend_kind);
    }
  } catch (...) {
    const auto message = WifiError::Format(std::current_exception());
    state.status = message.c_str();
  }
#elif defined(ANDROID)
  state.connectivity = GetNetState();
  state.backend = "Android";
  state.status = GetManagedBySystemSettingsText();
  const auto android_ip = GetPlatformWifiIpAddress();
  if (!android_ip.empty())
    state.ip = android_ip;
#elif defined(_WIN32)
  state.connectivity = GetNetState();
  state.backend = "Windows";
  state.status = GetManagedBySystemSettingsText();
#elif defined(__APPLE__) && TARGET_OS_IPHONE
  state.connectivity = GetNetState();
  state.backend = "iOS";
  state.status = GetManagedBySystemSettingsText();
  const auto ios_ip = GetPlatformWifiIpAddress();
  if (!ios_ip.empty())
    state.ip = ios_ip;
#else
  (void)rows;
  state.status = _("In-app network settings are not available in this build.");
#endif

#if defined(KOBO) || defined(ANDROID) || defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_IPHONE)
  (void)rows;
#endif
}

static void
HandlePlatformModified(RowFormWidget &widget, const NetworkConfigRows &rows,
                       DataField &df,
                       std::function<void()> refresh) noexcept
{
#if defined(KOBO)
  if (rows.have_persist_wifi && widget.IsDataField(rows.persist_wifi, df)) {
    if (!SetKoboWifiAutoOn(widget.GetValueBoolean(rows.persist_wifi))) {
      ShowMessageBox(_("Failed to store the WiFi startup setting."),
                     _("Network"), MB_OK);
      refresh();
    }

    return;
  }

  if (!widget.IsDataField(rows.radio, df))
    return;

  try {
    const bool enabled = widget.GetValueBoolean(rows.radio);
    const bool success = enabled ? KoboWifiOn() : KoboWifiOff();
    if (!success)
      throw std::runtime_error{enabled
        ? _("Failed to enable WiFi.")
        : _("Failed to disable WiFi.")};

    refresh();
  } catch (...) {
    const auto message = WifiError::Format(std::current_exception());
    ShowMessageBox(message.c_str(), _("Network"), MB_OK);
    refresh();
  }
#elif defined(HAVE_LINUX_NET_WIFI)
  if (!rows.have_radio || !widget.IsDataField(rows.radio, df))
    return;

  try {
    const auto backend_kind = QueryLinuxWifiBackendKind();
    if (backend_kind == LinuxWifiBackendKind::None) {
      refresh();
      return;
    }

    SetLinuxWifiRadioEnabled(backend_kind, widget.GetValueBoolean(rows.radio));
    refresh();
  } catch (...) {
    const auto message = WifiError::Format(std::current_exception());
    ShowMessageBox(message.c_str(), _("Network"), MB_OK);
    refresh();
  }
#else
  (void)widget;
  (void)rows;
  (void)df;
  (void)refresh;
#endif
}

class NetworkConfigWidget final
  : public RowFormWidget, public DataFieldListener {
  NetworkConfigRows rows;

public:
  NetworkConfigWidget()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void OnRefresh() noexcept;
  void OnModified(DataField &df) noexcept override;
};

void
NetworkConfigWidget::Prepare(ContainerWindow &parent,
                             const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  unsigned n = 0U;
  rows.status = n++;
  AddReadOnly(_("Status"), GetStatusHelp(), _("Checking WiFi..."));

  rows.connectivity = n++;
  AddReadOnly(_("Connectivity"),
              _("Current network connectivity state."),
              NetStateText::ToString(NetState::UNKNOWN));
  rows.ip = n++;
  AddReadOnly(_("IP address"),
              _("IPv4 address of the active WiFi interface."),
              _("Unknown"));
  rows.backend = n++;
  AddReadOnly(_("Backend"),
              GetBackendHelp(),
              GetInitialBackendName());

  PreparePlatformRows(*this, n, rows, *this);

  AddButton(_("WiFi List"), [this]() {
    OpenPlatformWifiList([this]() { OnRefresh(); });
  });

  OnRefresh();
}

void
NetworkConfigWidget::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);
  OnRefresh();
}

void
NetworkConfigWidget::OnRefresh() noexcept
{
  NetworkConfigState state;
  BuildPlatformState(state, rows);

  SetText(rows.status, state.status.c_str());
  SetText(rows.connectivity, NetStateText::ToString(state.connectivity));
  SetText(rows.ip, state.ip.c_str());
  SetText(rows.backend, state.backend.c_str());

  if (rows.have_radio && state.have_radio_enabled)
    LoadValue(rows.radio, state.radio_enabled);

  if (rows.have_persist_wifi && state.have_persist_wifi_enabled)
    LoadValue(rows.persist_wifi, state.persist_wifi_enabled);
}

void
NetworkConfigWidget::OnModified(DataField &df) noexcept
{
  HandlePlatformModified(*this, rows, df, [this]() { OnRefresh(); });
}

bool
NetworkConfigWidget::Save(bool &changed) noexcept
{
  (void)changed;
  return true;
}

std::unique_ptr<Widget>
CreateNetworkConfigPanel()
{
  return std::make_unique<NetworkConfigWidget>();
}
