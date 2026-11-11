// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetworkConfigPanel.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/WifiDialog.hpp"
#include "Language/Language.hpp"
#include "Language/FormatText.hpp"
#include "UIGlobals.hpp"
#include "Widget/GroupedListWidget.hpp"
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

/** The switches the platform offers on this page. */
struct NetworkConfigToggles {
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
  static StaticString<128> message;
  FormatFeatureNotAvailableInThisBuild(message,
                                       _("Platform/backend information"));
  return message.c_str();
#endif
}

static NetworkConfigToggles
QueryPlatformToggles() noexcept
{
  NetworkConfigToggles toggles;

#if defined(KOBO)
  toggles.have_radio = true;
  toggles.have_persist_wifi = true;
#elif defined(HAVE_LINUX_NET_WIFI)
  try {
    const auto backend_kind = QueryLinuxWifiBackendKind();
    toggles.have_radio = HasLinuxWifiRadioToggle(backend_kind);
  } catch (...) {
    toggles.have_radio = false;
  }
#endif

  return toggles;
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
                 C_("Setting", "Connectivity"), MB_OK);
#elif defined(_WIN32)
  if (OpenLink("ms-settings:network-wifi"))
    return;

  ShowMessageBox(_("Failed to open system settings."),
                 C_("Setting", "Connectivity"), MB_OK);
#elif defined(__APPLE__) && TARGET_OS_IPHONE
  ShowMessageBox(_("Open the Settings app, then go to Wi-Fi."),
                 C_("Setting", "Connectivity"), MB_OK);
#else
  (void)refresh;
  {
    StaticString<128> message;
    FormatFeatureNotAvailableInThisBuild(message, C_("Setting", "WiFi management"));
    ShowMessageBox(message, C_("Setting", "Connectivity"), MB_OK);
  }
#endif

#if defined(ANDROID) || defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_IPHONE)
  (void)refresh;
#endif
}

static void
BuildPlatformState(NetworkConfigState &state,
                   const NetworkConfigToggles &toggles) noexcept
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

    if (toggles.have_radio) {
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
  (void)toggles;
  state.status = _("In-app network settings are not available in this build.");
#endif

#if defined(KOBO) || defined(ANDROID) || defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_IPHONE)
  (void)toggles;
#endif
}

/** Turn the WiFi radio on or off; errors are shown to the user. */
static void
SetPlatformRadio([[maybe_unused]] bool enabled) noexcept
{
#if defined(KOBO)
  try {
    const bool success = enabled ? KoboWifiOn() : KoboWifiOff();
    if (!success)
      throw std::runtime_error{enabled
        ? _("Failed to enable WiFi.")
        : _("Failed to disable WiFi.")};
  } catch (...) {
    const auto message = WifiError::Format(std::current_exception());
    ShowMessageBox(message.c_str(), _("Network"), MB_OK);
  }
#elif defined(HAVE_LINUX_NET_WIFI)
  try {
    const auto backend_kind = QueryLinuxWifiBackendKind();
    if (backend_kind == LinuxWifiBackendKind::None)
      return;

    SetLinuxWifiRadioEnabled(backend_kind, enabled);
  } catch (...) {
    const auto message = WifiError::Format(std::current_exception());
    ShowMessageBox(message.c_str(), _("Network"), MB_OK);
  }
#endif
}

/** Enable or disable WiFi at startup; errors are shown to the user. */
static void
SetPlatformAutoWifi([[maybe_unused]] bool enabled) noexcept
{
#if defined(KOBO)
  if (!SetKoboWifiAutoOn(enabled))
    ShowMessageBox(_("Failed to store the WiFi startup setting."),
                   _("Network"), MB_OK);
#endif
}

/**
 * The network of the device: what it is connected to, and the
 * switches of its WiFi.  The page is read again whenever it is
 * shown or a switch was flipped.
 */
class NetworkConfigWidget final : public GroupedListWidget {
  const NetworkConfigToggles toggles = QueryPlatformToggles();

public:
  NetworkConfigWidget() noexcept
    :GroupedListWidget(UIGlobals::GetDialogLook()) {}

private:
  void Fill() noexcept;
  void Refresh() noexcept;

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
};

void
NetworkConfigWidget::Fill() noexcept
{
  NetworkConfigState state;
  BuildPlatformState(state, toggles);

  AddGroup(_("Status"));

  AddItem(_("Status"), {.value = state.status.c_str(),
                        .value_below = true,
                        .help = GetStatusHelp()});
  AddItem(C_("Setting", "Connectivity"),
          {.value = NetStateText::ToString(state.connectivity),
           .help = _("Current network connectivity state.")});
  AddItem(_("IP address"),
          {.value = state.ip.c_str(),
           .help = _("IPv4 address of the active WiFi interface.")});
  AddItem(C_("Setting", "Backend"),
          {.value = state.backend.c_str(), .help = GetBackendHelp()});

  AddGroup();

  if (toggles.have_radio) {
    const unsigned item = GetItemCount();
    AddItem(C_("Setting", "WiFi Enabled"), [this, item](){
      SetPlatformRadio(IsItemChecked(item));
      Refresh();
    }, {.toggle = true,
        .checked = state.have_radio_enabled && state.radio_enabled,
#if defined(KOBO)
        .help = _("Turns the Kobo WiFi interface on or off."),
#endif
    });
  }

  if (toggles.have_persist_wifi) {
    const unsigned item = GetItemCount();
    AddItem(C_("Setting", "Auto WiFi"), [this, item](){
      SetPlatformAutoWifi(IsItemChecked(item));
      Refresh();
    }, {.toggle = true,
        .checked = state.have_persist_wifi_enabled &&
        state.persist_wifi_enabled,
        .help = _("Enable WiFi automatically at startup.")});
  }

  AddButton(C_("Button", "WiFi List"), [this](){
    OpenPlatformWifiList([this](){ Refresh(); });
  });
}

void
NetworkConfigWidget::Refresh() noexcept
{
  Clear();
  Fill();
  UpdateLayout();
}

void
NetworkConfigWidget::Prepare(ContainerWindow &parent,
                             const PixelRect &rc) noexcept
{
  Fill();

  GroupedListWidget::Prepare(parent, rc);
}

void
NetworkConfigWidget::Show(const PixelRect &rc) noexcept
{
  /* the connection may have changed since the last time */
  Refresh();

  GroupedListWidget::Show(rc);
}

std::unique_ptr<Widget>
CreateNetworkConfigPanel()
{
  return std::make_unique<NetworkConfigWidget>();
}
