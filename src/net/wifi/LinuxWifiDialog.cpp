// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LinuxWifiDialog.hpp"
#include "WifiService.hpp"
#include "WifiError.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/Message.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Widget/ListWidget.hpp"
#include "Language/Language.hpp"
#include "ui/event/Notify.hpp"
#include "util/StaticString.hxx"
#include "Form/Button.hpp"
#include "lib/dbus/Connection.hxx"

#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class LinuxWifiListWidget final : public ListWidget {
  WifiService &service;

  /**
   * Last scan rows (copy of #WifiService::GetRows); use #WifiService::Row
   * list_label for row text like #NetworkConfigWidget::RepopulateApList.
   */
  std::vector<WifiService::Row> list_rows;

  std::unique_ptr<UI::Notify> refresh;
  std::atomic<bool> working{false};

  std::mutex err_mutex;
  std::string last_error;

  /**
   * Joined in the destructor and before a new job in #RunBackground; never
   * detached (same as #NetworkConfigWidget).
   */
  std::thread worker;

  Button *connect_button{nullptr};

  TwoTextRowsRenderer row_renderer;

  void
  RepopulateListFromService() noexcept;

  /** Same implementation pattern as #NetworkConfigWidget::RunBackground. */
  void
  RunBackground(const char *fallback_error, std::function<void()> work) noexcept;

  void
  OnRefresh() noexcept;
  void
  DoScan() noexcept;
  void
  Reconnect() noexcept;
  void
  DoConnect() noexcept;
  void
  DoDisconnect() noexcept;
  void
  UpdateButtons() noexcept;

  /** For passphrase prompt; SSID as in #WifiService (not list_label). */
  static const char *PassphrasePromptSsid(
    const WifiService::Row &r) noexcept;

  static bool
  RowNeedsKey(const WifiService::Row &r) noexcept;

  static void
  FormatDetail(const WifiService::Row &r, StaticString<64> &out) noexcept;

public:
  explicit LinuxWifiListWidget(WifiService &_service) noexcept
    : service(_service) {
  }

  ~LinuxWifiListWidget() noexcept override;

  void
  CreateButtons(WidgetDialog &dialog) noexcept
  {
    connect_button = dialog.AddButton(
      _("Connect"), [this]() { DoConnect(); });
    dialog.AddButton(_("Re-Connect"), [this]() { Reconnect(); });
    dialog.AddButton(_("Scan"), [this]() { DoScan(); });
  }

  void
  Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  void
  OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned idx) noexcept override;

  void
  OnCursorMoved(unsigned index) noexcept override
  {
    (void)index;
    UpdateButtons();
  }
};

void
ShowLinuxWifiDialog(WifiService &service) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<LinuxWifiListWidget> dialog(WidgetDialog::Full{},
    UIGlobals::GetMainWindow(), look, _("WiFi"));
  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget(service);
  dialog.GetWidget().CreateButtons(dialog);
  dialog.ShowModal();
}

LinuxWifiListWidget::~LinuxWifiListWidget() noexcept
{
  if (worker.joinable()) {
    worker.join();
  }
}

void
LinuxWifiListWidget::RepopulateListFromService() noexcept
{
  try {
    list_rows = service.GetRows();
  } catch (...) {
    list_rows.clear();
  }
  try {
    GetList().SetLength(list_rows.size());
  } catch (...) {
  }
}

const char *
LinuxWifiListWidget::PassphrasePromptSsid(const WifiService::Row &r) noexcept
{
  if (r.row_backend == WifiService::Backend::NetworkManager) {
    if (!r.nm.ssid_text.empty()) {
      return r.nm.ssid_text.c_str();
    }
  } else if (r.row_backend == WifiService::Backend::ConnMan) {
    if (!r.cm.ssid_text.empty()) {
      return r.cm.ssid_text.c_str();
    }
  }
  return r.list_label.c_str();
}

bool
LinuxWifiListWidget::RowNeedsKey(const WifiService::Row &r) noexcept
{
  if (r.row_backend == WifiService::Backend::NetworkManager) {
    return r.nm.needs_key;
  }
  if (r.row_backend == WifiService::Backend::ConnMan) {
    return r.cm.needs_key;
  }
  return true;
}

void
LinuxWifiListWidget::FormatDetail(
  const WifiService::Row &r, StaticString<64> &out) noexcept
{
  int pct = 0;
  if (r.row_backend == WifiService::Backend::NetworkManager) {
    pct = r.nm.strength;
  } else {
    pct = r.cm.strength;
  }
  if (pct > 0) {
    out.UnsafeFormat(
      "%s  %d%%", RowNeedsKey(r) ? _("WPA") : _("Open"), pct);
  } else {
    out = RowNeedsKey(r) ? _("WPA") : _("Open");
  }
}

void
LinuxWifiListWidget::Prepare(
  ContainerWindow &parent, const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  (void)CreateList(
    parent, look, rc, row_renderer.CalculateLayout(look.text_font, look.small_font));
  list_rows.clear();
  refresh = std::make_unique<UI::Notify>([this]() { OnRefresh(); });
  DoScan();
}

void
LinuxWifiListWidget::OnRefresh() noexcept
{
  working = false;
  {
    const std::lock_guard lock{err_mutex};
    if (!last_error.empty()) {
      ShowMessageBox(last_error.c_str(), _("Network"), MB_OK);
      last_error.clear();
    }
  }
  RepopulateListFromService();
  UpdateButtons();
  GetList().Invalidate();
}

void
LinuxWifiListWidget::RunBackground(
  const char *fallback_error, std::function<void()> work) noexcept
{
  if (working.load()) {
    return;
  }
  if (worker.joinable()) {
    worker.join();
  }
  try {
    working = true;
    worker = std::thread(
        [this, fe = std::string{fallback_error}, w = std::move(work)]() {
          try {
            w();
          } catch (const std::exception &e) {
            const std::lock_guard lock{err_mutex};
            last_error = FormatWifiErrorForUser(e.what());
          } catch (...) {
            const std::lock_guard lock{err_mutex};
            last_error = fe;
          }
          if (refresh) {
            refresh->SendNotification();
          }
        });
  } catch (...) {
    working = false;
  }
}

void
LinuxWifiListWidget::DoScan() noexcept
{
  RunBackground("Network scan failed", [this]() { service.Scan(); });
}

void
LinuxWifiListWidget::Reconnect() noexcept
{
  RunBackground("Re-connect failed", [this]() { service.ReconnectRadio(); });
}

void
LinuxWifiListWidget::DoConnect() noexcept
{
  if (working.load() || service.GetRows().empty()) {
    return;
  }
  const unsigned i = GetList().GetCursorIndex();
  if (i >= list_rows.size() || i >= (unsigned)service.GetRows().size()) {
    return;
  }
  const WifiService::Row snapshot{list_rows[i]};
  if (snapshot.is_connected) {
    DoDisconnect();
    return;
  }
  std::string pws;
  if (RowNeedsKey(snapshot)) {
    ODBus::Connection c{ODBus::Connection::GetSystem()};
    if (c && service.HasSavedSystemPsk(c, snapshot)) {
      pws.clear();
    } else {
      StaticString<256> cap;
      cap.UnsafeFormat(
        _("Passphrase of network '%s' (empty = use saved)"),
        PassphrasePromptSsid(snapshot));
      StaticString<256> passphrase;
      passphrase.clear();
      if (!TextEntryDialog(passphrase, cap, false)) {
        return;
      }
      pws = passphrase.c_str();
    }
  } else {
    pws.clear();
  }
  RunBackground("Connect failed", [this, snapshot, pws]() {
    service.Connect(snapshot, pws.c_str());
  });
}

void
LinuxWifiListWidget::DoDisconnect() noexcept
{
  /* Same lambda as #NetworkConfigWidget::DoDisconnect. */
  RunBackground("Disconnect failed", [this]() { service.Disconnect(); });
}

void
LinuxWifiListWidget::UpdateButtons() noexcept
{
  if (connect_button == nullptr) {
    return;
  }
  if (list_rows.empty() || (GetList().GetLength() == 0U)) {
    connect_button->SetEnabled(false);
    return;
  }
  const unsigned i = GetList().GetCursorIndex();
  if (i < list_rows.size()) {
    if (list_rows[i].is_connected) {
      connect_button->SetCaption(_("Disconnect"));
    } else {
      connect_button->SetCaption(_("Connect"));
    }
    connect_button->SetEnabled(true);
  } else {
    connect_button->SetEnabled(false);
  }
  if (working.load()) {
    connect_button->SetEnabled(false);
  }
}

void
LinuxWifiListWidget::OnPaintItem(
  Canvas &canvas, const PixelRect rc, unsigned idx) noexcept
{
  if (idx >= list_rows.size()) {
    return;
  }
  const auto &r = list_rows[idx];
  /* Line 1: same string as the Access points enum in #NetworkConfigWidget. */
  row_renderer.DrawFirstRow(canvas, rc, r.list_label.c_str());
  StaticString<64> detail;
  FormatDetail(r, detail);
  row_renderer.DrawSecondRow(canvas, rc, detail.c_str());
  if (r.is_connected) {
    row_renderer.DrawRightFirstRow(canvas, rc, _("Connected"));
  }
}
