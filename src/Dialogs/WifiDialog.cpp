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
#include "Kobo/PlatformWifiBackend.hpp"
#include "net/IPv4Address.hxx"
#include "ui/event/PeriodicTimer.hpp"

#include <memory>

class WifiListWidget final
  : public ListWidget {

  struct NetworkInfo {
    StaticString<32> bssid;
    StaticString<256> ssid;
    bool signal_detected;
    signed signal_level;
    int id;

    enum WifiSecurity security;

    bool old_visible, old_configured;
  };

  Button *connect_button;

  WifiStatus status;
  TrivialArray<NetworkInfo, 64> networks;

  TwoTextRowsRenderer row_renderer;

  std::unique_ptr<WifiBackend> backend_;

  UI::PeriodicTimer update_timer{[this]{ UpdateList(); }};

  bool signal_level_in_dbm;

public:
  WifiListWidget(): backend_(CreatePlatformWifiBackend()) {
    signal_level_in_dbm = backend_ != nullptr && backend_->IsSignalLevelInDbm();
  }

  void CreateButtons(WidgetDialog &dialog) {
    dialog.AddButton(_("Scan"), [this](){
      try {
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
  [[gnu::pure]]
  NetworkInfo *FindByID(int id) noexcept;

  [[gnu::pure]]
  NetworkInfo *FindByBSSID(const char *bssid) noexcept;

  [[gnu::pure]]
  NetworkInfo *FindVisibleBySSID(const char *ssid) noexcept;

  [[gnu::pure]]
  NetworkInfo *Find(const WifiConfiguredNetworkInfo &c) noexcept;

  void MergeList(const WifiVisibleNetwork *p, unsigned n);
  void UpdateScanResults();
  void Append(const WifiConfiguredNetworkInfo &src);
  void Merge(const WifiConfiguredNetworkInfo &c);
  void MergeList(const WifiConfiguredNetworkInfo *p, unsigned n);
  void UpdateConfigured();
  void SweepList();
  void UpdateList();

  void Connect();
};

void
WifiListWidget::UpdateButtons()
{
  const unsigned cursor = GetList().GetCursorIndex();

  if (cursor < networks.size()) {
    const auto &info = networks[cursor];

    if (info.id >= 0) {
      connect_button->SetCaption(_("Remove"));
      connect_button->SetEnabled(true);
    } else if (info.signal_detected) {
      connect_button->SetCaption(_("Connect"));
      connect_button->SetEnabled(true);
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

  static char wifi_security[][20] = {
    "WPA",
    "WEP",
    "Open",
  };

  row_renderer.DrawFirstRow(canvas, rc, info.ssid);
  row_renderer.DrawSecondRow(canvas, rc, info.bssid);

  const char *state = nullptr;
  StaticString<40> state_buffer;

  /* found the currently connected wifi network? */
  if (StringIsEqual(info.bssid, status.bssid)) {
    state = _("Connected");

    /* look up ip address for wlan0 or eth0 */
    const auto addr = IPv4Address::GetDeviceAddress(backend_->GetInterfaceName());
    if (addr.IsDefined()) { /* valid address? */
      StaticString<40> addr_str;
      if (addr.ToString(addr_str.buffer(), addr_str.capacity()) != nullptr) {
        state_buffer.Format("%s (%s)", state, addr_str.c_str());
        state = state_buffer;
      }
    }
  }
  else if (info.id >= 0)
    state = info.signal_detected
      ? _("Saved and visible")
      : _("Saved, but not visible");
  else if (info.signal_detected)
    state = _("Visible");

  if (state != nullptr)
    row_renderer.DrawRightFirstRow(canvas, rc, state);

  if (info.signal_detected) {
    StaticString<36> text;
    text.UnsafeFormat(signal_level_in_dbm ? "%s %d dBm" : "%s %d",
                      wifi_security[info.security], info.signal_level);
    row_renderer.DrawRightSecondRow(canvas, rc, text);
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
  if (info.id < 0) {
    const auto ssid = info.ssid;

    StaticString<256> caption;
    caption.Format(_("Passphrase of network '%s'"), ssid.c_str());

    StaticString<32> passphrase;
    passphrase.clear();
    if (info.security == OPEN_SECURITY)
      passphrase.clear();
    else if (!TextEntryDialog(passphrase, caption, false))
      return;

    backend_->Connect(info.ssid.c_str(), passphrase.c_str(), info.security);
  } else {
    backend_->RemoveNetwork(info.id);
    backend_->SaveConfig();
  }

  UpdateList();
}

inline WifiListWidget::NetworkInfo *
WifiListWidget::FindByID(int id) noexcept
{
  auto f = std::find_if(networks.begin(), networks.end(),
                        [id](const NetworkInfo &info) {
                          return info.id == id;
                        });
  if (f == networks.end())
    return nullptr;

  return f;
}

WifiListWidget::NetworkInfo *
WifiListWidget::FindByBSSID(const char *bssid) noexcept
{
  auto f = std::find_if(networks.begin(), networks.end(),
                        [bssid](const NetworkInfo &info) {
                          return info.bssid == bssid;
                        });
  if (f == networks.end())
    return nullptr;

  return f;
}

WifiListWidget::NetworkInfo *
WifiListWidget::FindVisibleBySSID(const char *ssid) noexcept
{
  auto f = std::find_if(networks.begin(), networks.end(),
                        [ssid](const NetworkInfo &info) {
                          return info.signal_detected && info.ssid == ssid;
                        });
  if (f == networks.end())
    return nullptr;

  return f;
}

inline void
WifiListWidget::MergeList(const WifiVisibleNetwork *p, unsigned n)
{
  for (unsigned i = 0; i < unsigned(n); ++i) {
    const auto &found = p[i];

    auto info = FindByBSSID(found.bssid);
    if (info == nullptr) {
      info = &networks.append();
      info->bssid = found.bssid;
      info->id = -1;
    }

    info->ssid = found.ssid;
    info->signal_detected = true;
    info->signal_level = found.signal_level;
    info->security = found.security;
    info->old_visible = false;
  }
}

inline void
WifiListWidget::UpdateScanResults()
{
  WifiVisibleNetwork *buffer = new WifiVisibleNetwork[networks.capacity()];
  int n = (int)backend_->ScanResults(buffer, networks.capacity());
  if (n >= 0)
    MergeList(buffer, n);

  delete[] buffer;
}

inline WifiListWidget::NetworkInfo *
WifiListWidget::Find(const WifiConfiguredNetworkInfo &c) noexcept
{
  auto found = FindByID(c.id);
  if (found != nullptr)
    return found;

  return c.bssid == "any"
    ? FindVisibleBySSID(c.ssid)
    : FindByBSSID(c.bssid);
}

inline void
WifiListWidget::Append(const WifiConfiguredNetworkInfo &src)
{
  auto &dest = networks.append();
  dest.bssid = src.bssid;
  dest.ssid = src.ssid;
  dest.id = src.id;
  dest.signal_detected = false;
  dest.old_configured = false;
}

inline void
WifiListWidget::Merge(const WifiConfiguredNetworkInfo &c)
{
  auto found = Find(c);
  if (found != nullptr) {
    found->id = c.id;
    found->old_configured = false;
  } else
    Append(c);
}

inline void
WifiListWidget::MergeList(const WifiConfiguredNetworkInfo *p, unsigned n)
{
  for (unsigned i = 0; i < unsigned(n); ++i)
    Merge(p[i]);
}

inline void
WifiListWidget::UpdateConfigured()
{
  WifiConfiguredNetworkInfo *buffer =
    new WifiConfiguredNetworkInfo[networks.capacity()];
  int n = (int)backend_->ListNetworks(buffer, networks.capacity());
  if (n >= 0)
    MergeList(buffer, n);

  delete[] buffer;
}

inline void
WifiListWidget::SweepList()
{
  unsigned cursor = GetList().GetCursorIndex();

  for (int i = networks.size() - 1; i >= 0; --i) {
    NetworkInfo &info = networks[i];

    if (info.old_visible && info.old_configured) {
      networks.remove(i);
      if (cursor > unsigned(i))
        --cursor;
    } else {
      if (info.old_visible)
        info.signal_detected = false;

      if (info.old_configured)
        info.id = -1;
    }
  }

  GetList().SetCursorIndex(cursor);
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
    backend_->Status(status);

    for (auto &i : networks)
      i.old_visible = i.old_configured = true;

    UpdateScanResults();
    UpdateConfigured();

    /* remove items that are still marked as "old" */
    SweepList();
  } catch (...) {
    networks.clear();
  }

  GetList().SetLength(networks.size());

  UpdateButtons();
}

void
ShowWifiDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<WifiListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look, _("Wifi"));
  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget();
  dialog.GetWidget().CreateButtons(dialog);
  dialog.ShowModal();
}
