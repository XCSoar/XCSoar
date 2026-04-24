// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WifiDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/TextEntry.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "LogFile.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "util/Exception.hxx"
#include "Language/Language.hpp"
#include "Widget/ListWidget.hpp"
#include "net/wifi/WifiBackend.hpp"
#include "net/wifi/WifiError.hpp"
#include "net/wifi/WifiFormat.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include <array>
#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

namespace {

static constexpr auto kWifiRefreshInterval = std::chrono::seconds{1};
static constexpr auto kWifiAutoScanInterval = std::chrono::seconds{30};

[[gnu::pure]]
static unsigned
SortRank(const WifiNetworkEntry &entry) noexcept
{
  if (entry.kind == WifiNetworkKind::ConnectedNetwork)
    return 0;

  if (entry.is_visible)
    return 1;

  return 2;
}

[[gnu::pure]]
static bool
NetworkEntrySortLess(const WifiNetworkEntry &a,
                     const WifiNetworkEntry &b) noexcept
{
  const auto a_rank = SortRank(a);
  const auto b_rank = SortRank(b);
  if (a_rank != b_rank)
    return a_rank < b_rank;

  if (a.is_visible && b.is_visible && a.signal_level != b.signal_level)
    return a.signal_level > b.signal_level;

  return std::strcmp(a.ssid.c_str(), b.ssid.c_str()) < 0;
}

[[gnu::pure]]
static bool
IsNmConnectionPath(const WifiNetworkEntry &entry) noexcept
{
  return !entry.profile_id.empty() && entry.profile_id.c_str()[0] == '/';
}

[[gnu::pure]]
static bool
CanMergeNetworkEntries(const WifiNetworkEntry &a,
                       const WifiNetworkEntry &b) noexcept
{
  if (!a.profile_id.empty() && !b.profile_id.empty() &&
      a.profile_id == b.profile_id)
    return true;

  if (!a.ssid.empty() && !b.ssid.empty() && a.ssid == b.ssid) {
    /* Same SSID is not enough: visible AP (nm-bssid:…) and a saved NM
       profile (/org/freedesktop/…) must stay separate rows. */
    if (!a.profile_id.empty() && !b.profile_id.empty() &&
        a.profile_id != b.profile_id)
      return false;

    return true;
  }

  return a.is_visible && b.is_visible &&
    !a.bssid.empty() && !b.bssid.empty() &&
    a.bssid == b.bssid;
}

[[gnu::pure]]
static const char *
GetPrimaryText(const WifiNetworkEntry &entry) noexcept
{
  if (!entry.ssid.empty())
    return entry.ssid.c_str();

  if (!entry.bssid.empty())
    return entry.bssid.c_str();

  if (!entry.profile_id.empty())
    return entry.profile_id.c_str();

  return _("Hidden network");
}

[[gnu::pure]]
static bool
IsOpaqueProfileId(const WifiNetworkEntry &entry) noexcept
{
  return !entry.profile_id.empty() && entry.profile_id.c_str()[0] == '/';
}

[[gnu::pure]]
static const char *
GetSecondaryText(const WifiNetworkEntry &entry) noexcept
{
  if (entry.kind == WifiNetworkKind::SavedProfile)
    return "";

  if (!entry.ssid.empty()) {
    if (!entry.bssid.empty())
      return entry.bssid.c_str();

    if (!entry.profile_id.empty() && !IsOpaqueProfileId(entry))
      return entry.profile_id.c_str();

    return "";
  }

  if (!entry.bssid.empty() && !entry.profile_id.empty() &&
      !IsOpaqueProfileId(entry))
    return entry.profile_id.c_str();

  return "";
}

static void
MergeNetworkEntry(WifiNetworkEntry &dest, const WifiNetworkEntry &src) noexcept
{
  if (!src.profile_id.empty()) {
    if (dest.profile_id.empty())
      dest.profile_id = src.profile_id;
    else if (!IsNmConnectionPath(dest) && IsNmConnectionPath(src))
      dest.profile_id = src.profile_id;
  }

  if (dest.ssid.empty() && !src.ssid.empty())
    dest.ssid = src.ssid;

  if (dest.bssid.empty() && !src.bssid.empty())
    dest.bssid = src.bssid;

  if (dest.interface_name.empty() && !src.interface_name.empty())
    dest.interface_name = src.interface_name;

  if (!dest.is_visible && src.is_visible) {
    dest.bssid = src.bssid;
    dest.signal_level = src.signal_level;
    dest.security = src.security;
    dest.signal_unit = src.signal_unit;
    dest.kind = src.kind;
    dest.is_visible = true;
  } else if (src.kind == WifiNetworkKind::ConnectedNetwork) {
    dest.kind = WifiNetworkKind::ConnectedNetwork;
  } else if (dest.kind != WifiNetworkKind::ConnectedNetwork &&
             src.kind == WifiNetworkKind::VisibleAccessPoint) {
    dest.kind = WifiNetworkKind::VisibleAccessPoint;
  }

  dest.has_stored_credentials = dest.has_stored_credentials ||
    src.has_stored_credentials;
  dest.can_connect = dest.can_connect || src.can_connect;
  dest.can_disconnect = dest.can_disconnect || src.can_disconnect;
  dest.can_forget = dest.can_forget || src.can_forget;
}

[[gnu::pure]]
static bool
NeedsPassphrasePrompt(const WifiNetworkEntry &entry) noexcept
{
  return ToWifiAuthMode(entry.security) == WifiAuthMode::Passphrase &&
    !entry.has_stored_credentials;
}

static void
ShowWifiError(std::exception_ptr error, const char *caption) noexcept
{
  ShowMessageBox(WifiError::Format(std::move(error)).c_str(), caption,
                 MB_OK | MB_ICONEXCLAMATION);
}

} // namespace

class WifiListWidget final
  : public ListWidget {

  Button *scan_button;
  Button *connect_button;
  Button *forget_button;
  TrivialArray<WifiNetworkEntry, 64> networks;

  TwoTextRowsRenderer row_renderer;

  UniqueWifiBackend backend_;
  std::string refresh_error;
  bool scan_pending = false;

  UI::PeriodicTimer update_timer{[this]{ UpdateList(); }};
  UI::PeriodicTimer scan_timer{[this]{ Scan(false, false); }};

public:
  explicit WifiListWidget(UniqueWifiBackend _backend)
    :scan_button(nullptr), connect_button(nullptr), forget_button(nullptr),
     backend_(std::move(_backend)) {}

  void CreateButtons(WidgetDialog &dialog) {
    scan_button = dialog.AddButton(_("Scan"), [this](){
      Scan(true, true);
      scan_timer.Schedule(kWifiAutoScanInterval);
    });

    connect_button = dialog.AddButton(_("Connect"), [this](){
      try {
        Connect();
      } catch (...) {
        ShowWifiError(std::current_exception(), _("Error"));
      }
    });

    forget_button = dialog.AddButton(_("Forget"), [this](){
      try {
        Forget();
      } catch (...) {
        ShowWifiError(std::current_exception(), _("Error"));
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
    Scan(false, true);
    update_timer.Schedule(kWifiRefreshInterval);
    scan_timer.Schedule(kWifiAutoScanInterval);
  }

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from class ListCursorHandler */
  void OnCursorMoved([[maybe_unused]] unsigned index) noexcept override {
    UpdateButtons();
  }

private:
  void Scan(bool show_error, bool show_progress) noexcept;
  void UpdateList();

  void Connect();
  void Forget();
};

void
WifiListWidget::Scan(bool show_error, bool show_progress) noexcept
{
  if (backend_ == nullptr)
    return;

  try {
    backend_->Scan();

    if (show_progress && scan_button != nullptr) {
      scan_pending = true;
      scan_button->SetCaption(_("Scanning..."));
      scan_button->SetEnabled(false);
    }
  } catch (...) {
    if (show_error)
      ShowWifiError(std::current_exception(), _("Error"));
  }
}

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
    } else {
      connect_button->SetCaption(_("Connect"));
      connect_button->SetEnabled(false);
    }

    forget_button->SetEnabled(info.can_forget);
  } else {
    connect_button->SetEnabled(false);
    forget_button->SetEnabled(false);
  }
}

void
WifiListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                            unsigned idx) noexcept
{
  if (idx >= networks.size()) {
    row_renderer.DrawFirstRow(canvas, rc, _("WiFi update failed"));
    row_renderer.DrawSecondRow(canvas, rc, refresh_error.c_str());
    return;
  }

  const auto &info = networks[idx];
  const char *auth = !info.is_visible &&
    info.kind == WifiNetworkKind::SavedProfile
    ? nullptr
    : FormatWifiSecurityLabel(info.security);

  row_renderer.DrawFirstRow(canvas, rc, GetPrimaryText(info));
  row_renderer.DrawSecondRow(canvas, rc, GetSecondaryText(info));

  const bool connected = info.kind == WifiNetworkKind::ConnectedNetwork;
  const auto state = WifiNetworkEntry::FormatState(info);

  if (!state.empty()) {
    const auto old_text_color = canvas.GetTextColor();
    if (connected)
      canvas.SetTextColor(COLOR_GREEN);

    row_renderer.DrawRightFirstRow(canvas, rc, state.c_str());

    if (connected)
      canvas.SetTextColor(old_text_color);
  }

  if (info.is_visible) {
    StaticString<64> text;
    switch (info.signal_unit) {
    case WifiSignalUnit::Dbm:
      text.UnsafeFormat("%s %d dBm", auth, info.signal_level);
      break;

    case WifiSignalUnit::Relative:
      text.UnsafeFormat("%s %d%%", auth, info.signal_level);
      break;

    case WifiSignalUnit::Unknown:
      text = auth;
      break;

    case WifiSignalUnit::COUNT:
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
    WifiConnectRequest request;
    request.profile_id = info.profile_id;
    request.security = info.security;

    const bool needs_passphrase = NeedsPassphrasePrompt(info);

    if (info.profile_id.empty() || needs_passphrase) {
      request.ssid = info.ssid;

      if (needs_passphrase) {
        const char *name = GetPrimaryText(info);

        StaticString<256> caption;
        caption.Format(_("Passphrase of network '%s'"), name);

        StaticString<256> passphrase;
        passphrase.clear();
        if (!TextEntryDialog(passphrase, caption, false))
          return;

        request.secret = passphrase;
      }
    }

    backend_->Connect(request);
  }

  UpdateList();
}

void
WifiListWidget::Forget()
{
  if (backend_ == nullptr)
    return;

  const unsigned i = GetList().GetCursorIndex();
  if (i >= networks.size())
    return;

  const auto &info = networks[i];
  if (!info.can_forget)
    return;

  backend_->ForgetNetwork(info.profile_id);
  UpdateList();
}

void
WifiListWidget::UpdateList()
{
  networks.clear();

  if (backend_ == nullptr) {
    GetList().SetLength(0);
    UpdateButtons();
    return;
  }

  try {
    std::array<WifiNetworkEntry, 64> buffer;
    const auto n = backend_->GetNetworks(buffer.data(), buffer.size());
    std::stable_sort(buffer.begin(), buffer.begin() + n, NetworkEntrySortLess);

    for (std::size_t i = 0; i < n; ++i) {
      const auto &candidate = buffer[i];

      WifiNetworkEntry *existing = nullptr;
      for (std::size_t j = 0; j < networks.size(); ++j) {
        if (CanMergeNetworkEntries(networks[j], candidate)) {
          existing = &networks[j];
          break;
        }
      }

      if (existing != nullptr) {
        MergeNetworkEntry(*existing, candidate);
        continue;
      }

      networks.append(candidate);
    }
    refresh_error.clear();
  } catch (const std::exception &e) {
    const auto error = std::current_exception();
    LogFmt("WiFi dialog refresh failed: {}", GetFullMessage(error));
    refresh_error = WifiError::Format(error);
    networks.clear();
  } catch (...) {
    LogFormat("WiFi dialog refresh failed: unknown exception");
    refresh_error = _("The operation failed.");
    networks.clear();
  }

  if (scan_pending && scan_button != nullptr) {
    scan_pending = false;
    scan_button->SetCaption(_("Scan"));
    scan_button->SetEnabled(true);
  }

  GetList().SetLength(networks.size() + (refresh_error.empty() ? 0U : 1U));

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
  dialog.EnableCursorSelection();
  dialog.ShowModal();
}
