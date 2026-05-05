// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WifiDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/TextEntry.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Language/Language.hpp"
#include "Widget/ListWidget.hpp"
#include "net/wifi/WifiBackend.hpp"
#include "net/IPv4Address.hxx"
#include "ui/event/PeriodicTimer.hpp"

#include <array>
#include <memory>
#include <utility>

namespace {

[[gnu::pure]]
static const char *
GetAuthLabel(WifiSecurity security) noexcept
{
  switch (ToWifiAuthMode(security)) {
  case WifiAuthMode::Open:
    return "Open";

  case WifiAuthMode::Passphrase:
    return "Passphrase";

  case WifiAuthMode::Unsupported:
  case WifiAuthMode::COUNT:
    return "Unsupported";
  }

  return "Unsupported";
}

} // namespace

class WifiListWidget final
  : public ListWidget {

  Button *connect_button;

  WifiBackendStatus status;
  TrivialArray<WifiNetworkEntry, 64> networks;

  TwoTextRowsRenderer row_renderer;

  UniqueWifiBackend backend_;

  UI::PeriodicTimer update_timer{[this]{ UpdateList(); }};

public:
  explicit WifiListWidget(UniqueWifiBackend _backend)
    :backend_(std::move(_backend)) {}

  void CreateButtons(WidgetDialog &dialog) {
    dialog.AddButton(_("Scan"), [this](){
      try {
        if (backend_ == nullptr)
          return;

        backend_->EnsureConnected();
        backend_->Scan();
        UpdateList();
      } catch (...) {
        ShowError(std::current_exception(), _("Error"));
      }
    });

    connect_button = dialog.AddButton(_("Connect"), [this](){
      try {
        Connect();
      } catch (...) {
        ShowError(std::current_exception(), _("Error"));
      }
    });
  }

  void UpdateButtons();

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    const DialogLook &look = UIGlobals::GetDialogLook();

    CreateList(parent, look, rc,
               row_renderer.CalculateLayout(look.text_font,
                                            look.small_font));
    UpdateList();
    update_timer.Schedule(std::chrono::seconds(1));
  }

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from class ListCursorHandler */
  void OnCursorMoved([[maybe_unused]] unsigned index) noexcept override {
    UpdateButtons();
  }

private:
  void UpdateList();

  void Connect();
};

void
WifiListWidget::UpdateButtons()
{
  const unsigned cursor = GetList().GetCursorIndex();

  if (cursor < networks.size()) {
    const auto &info = networks[cursor];

    if (info.can_disconnect) {
      connect_button->SetCaption(_("Disconnect"));
      connect_button->SetEnabled(true);
    } else if (info.can_connect) {
      connect_button->SetCaption(_("Connect"));
      connect_button->SetEnabled(true);
    } else if (info.can_forget) {
      connect_button->SetCaption(_("Remove"));
      connect_button->SetEnabled(true);
    } else {
      connect_button->SetCaption(_("Connect"));
      connect_button->SetEnabled(false);
    }
  } else {
    connect_button->SetEnabled(false);
  }
}

void
WifiListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                            unsigned idx) noexcept
{
  const auto &info = networks[idx];
  const char *auth = GetAuthLabel(info.security);

  row_renderer.DrawFirstRow(canvas, rc, info.ssid);
  row_renderer.DrawSecondRow(canvas, rc, info.bssid);

  const char *state = nullptr;
  StaticString<40> state_buffer;

  if (info.kind == WifiNetworkKind::ConnectedNetwork) {
    state = _("Connected");

    /* look up ip address for wlan0 or eth0 */
    const auto addr = IPv4Address::GetDeviceAddress(status.interface_name);
    if (addr.IsDefined()) { /* valid address? */
      StaticString<40> addr_str;
      if (addr.ToString(addr_str.buffer(), addr_str.capacity()) != nullptr) {
        state_buffer.Format("%s (%s)", state, addr_str.c_str());
        state = state_buffer;
      }
    }
  }
  else if (info.kind == WifiNetworkKind::SavedProfile)
    state = info.is_visible
      ? _("Saved and visible")
      : _("Saved, but not visible");
  else if (info.is_visible)
    state = _("Visible");

  if (state != nullptr)
    row_renderer.DrawRightFirstRow(canvas, rc, state);

  if (info.is_visible) {
    StaticString<64> text;
    switch (info.signal_unit) {
    case WifiSignalUnit::Dbm:
      text.UnsafeFormat("%s %d dBm", auth, info.signal_level);
      break;

    case WifiSignalUnit::Relative:
      text.UnsafeFormat("%s %d", auth, info.signal_level);
      break;

    case WifiSignalUnit::Unknown:
    case WifiSignalUnit::COUNT:
      text = auth;
      break;
    }

    row_renderer.DrawRightSecondRow(canvas, rc, text);
  } else if (auth != nullptr) {
    row_renderer.DrawRightSecondRow(canvas, rc, auth);
  }
}

inline void
WifiListWidget::Connect()
{
  if (backend_ == nullptr)
    return;

  const unsigned i = GetList().GetCursorIndex();
  if (i >= networks.size())
    return;

  const auto &info = networks[i];
  if (info.can_disconnect) {
    backend_->Disconnect();
  } else if (info.can_connect) {
    const auto ssid = info.ssid;

    StaticString<256> caption;
    caption.Format(_("Passphrase of network '%s'"), ssid.c_str());

    StaticString<32> passphrase;
    passphrase.clear();
    if (ToWifiAuthMode(info.security) == WifiAuthMode::Open)
      passphrase.clear();
    else if (!TextEntryDialog(passphrase, caption, false))
      return;

    WifiConnectRequest request;
    request.profile_id = info.profile_id;
    request.ssid = info.ssid;
    request.secret = passphrase;
    request.security = info.security;
    backend_->Connect(request);
  } else if (info.can_forget) {
    backend_->ForgetNetwork(info.profile_id);
  }

  UpdateList();
}

void
WifiListWidget::UpdateList()
{
  status.Clear();
  networks.clear();

  if (backend_ == nullptr) {
    GetList().SetLength(0);
    UpdateButtons();
    return;
  }

  try {
    backend_->EnsureConnected();
    status = backend_->GetBackendStatus();

    std::array<WifiNetworkEntry, 64> buffer;
    const auto n = backend_->GetNetworks(buffer.data(), buffer.size());
    for (std::size_t i = 0; i < n; ++i)
      networks.append(buffer[i]);
  } catch (...) {
    networks.clear();
  }

  GetList().SetLength(networks.size());

  UpdateButtons();
}

void
ShowWifiDialog(UniqueWifiBackend backend)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<WifiListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look, _("WiFi"));
  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget(std::move(backend));
  dialog.GetWidget().CreateButtons(dialog);
  dialog.ShowModal();
}
