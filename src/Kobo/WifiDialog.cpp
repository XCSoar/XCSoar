/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "WifiDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/TextEntry.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Form/ActionListener.hpp"
#include "Widget/ListWidget.hpp"
#include "WPASupplicant.hpp"

class WifiListWidget final
  : public ListWidget, ActionListener, Timer {
  enum Buttons {
    SCAN,
    CONNECT,
  };

  struct NetworkInfo {
    StaticString<32> bssid;
    StaticString<256> ssid;
    int signal_level;
    int id;

    bool old_visible, old_configured;
  };

  WndButton *connect_button;

  WifiStatus status;
  TrivialArray<NetworkInfo, 64> networks;

  WPASupplicant wpa_supplicant;

public:
  void CreateButtons(WidgetDialog &dialog) {
    dialog.AddButton(_("Scan"), *this, SCAN);
    connect_button = dialog.AddButton(_("Connect"), *this, CONNECT);
  }

  void UpdateButtons();

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override {
    const DialogLook &look = UIGlobals::GetDialogLook();
    const unsigned row_height =
      std::max(Layout::GetMaximumControlHeight(),
               unsigned(Layout::GetTextPadding()) * 3
               + look.text_font->GetHeight()
               + look.small_font->GetHeight());

    CreateList(parent, look, rc, row_height);
    UpdateList();
    Timer::Schedule(1000);
  }

  virtual void Unprepare() override {
    Timer::Cancel();
    DeleteWindow();
  }

  /* virtual methods from class ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;

  /* virtual methods from class ListCursorHandler */
  virtual void OnCursorMoved(unsigned index) {
    UpdateButtons();
  }

  /* virtual methods from class Timer */
  virtual void OnTimer() {
    UpdateList();
  }

private:
  /**
   * Ensure that we're connected to wpa_supplicant.
   */
  bool EnsureConnected();

  gcc_pure
  NetworkInfo *FindByID(int id);

  gcc_pure
  NetworkInfo *FindByBSSID(const char *bssid);

  gcc_pure
  NetworkInfo *FindVisibleBySSID(const char *ssid);

  gcc_pure
  NetworkInfo *Find(const WifiConfiguredNetworkInfo &c);

  void MergeList(const WifiVisibleNetwork *p, unsigned n);
  void UpdateScanResults();
  void Append(const WifiConfiguredNetworkInfo &src);
  void Merge(const WifiConfiguredNetworkInfo &c);
  void MergeList(const WifiConfiguredNetworkInfo *p, unsigned n);
  void UpdateConfigured();
  void SweepList();
  void UpdateList();

  void Connect();

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;
};

void
WifiListWidget::UpdateButtons()
{
  const unsigned cursor = GetList().GetCursorIndex();

  if (cursor < networks.size()) {
    const auto &info = networks[cursor];

    if (info.id >= 0) {
      connect_button->SetText(_("Remove"));
      connect_button->SetEnabled(true);
    } else if (info.signal_level >= 0) {
      connect_button->SetText(_("Connect"));
      connect_button->SetEnabled(true);
    }
  } else {
    connect_button->SetEnabled(false);
  }
}

void
WifiListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                            unsigned idx)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  const auto &info = networks[idx];
  const unsigned padding = Layout::GetTextPadding();

  const unsigned x1 = rc.left + padding;
  const unsigned y1 = rc.top + padding;
  const unsigned y2 = y1 + look.text_font->GetHeight() + padding;

  canvas.Select(*look.text_font);
  canvas.DrawText(x1, y1, info.ssid);

  canvas.Select(*look.small_font);
  canvas.DrawText(x1, y2, info.bssid);

  const TCHAR *state = nullptr;
  if (StringIsEqual(info.bssid, status.bssid))
    state = _("Connected");
  else if (info.id >= 0)
    state = info.signal_level >= 0
      ? _("Saved and visible")
      : _("Saved, but not visible");
  else if (info.signal_level >= 0)
    state = _("Visible");

  if (state != nullptr) {
    unsigned width = canvas.CalcTextWidth(state);
    canvas.DrawText(rc.right - padding - width, y1, state);
  }

  if (info.signal_level >= 0) {
    StaticString<20> text;
    text.UnsafeFormat(_T("%u"), info.signal_level);
    unsigned width = canvas.CalcTextWidth(text);
    canvas.DrawText(rc.right - padding - width, y2, text);
  }
}

static bool
WifiConnect(WPASupplicant &wpa_supplicant, const char *ssid, const char *psk)
{
  int id = wpa_supplicant.AddNetwork();
  if (id < 0)
    return false;

  return wpa_supplicant.SetNetworkSSID(id, ssid) &&
    wpa_supplicant.SetNetworkPSK(id, psk) &&
    wpa_supplicant.EnableNetwork(id) &&
    wpa_supplicant.SaveConfig();
}

inline void
WifiListWidget::Connect()
{
  if (!EnsureConnected()) {
    ShowMessageBox(_T("Network failure"), _("Connect"), MB_OK);
    return;
  }

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
    if (!TextEntryDialog(passphrase, caption))
      return;

    if (!WifiConnect(wpa_supplicant, ssid, passphrase))
      ShowMessageBox(_T("Network failure"), _("Connect"), MB_OK);
  } else {
    if (!wpa_supplicant.RemoveNetwork(info.id) || !wpa_supplicant.SaveConfig())
      ShowMessageBox(_T("Error"), _("Remove"), MB_OK);
  }

  UpdateList();
}

void
WifiListWidget::OnAction(int id)
{
  switch (id) {
  case SCAN:
    if (EnsureConnected() && wpa_supplicant.Scan())
      UpdateList();
    break;

  case CONNECT:
    Connect();
    break;
  }
}

bool
WifiListWidget::EnsureConnected()
{
  return wpa_supplicant.IsConnected() ||
    wpa_supplicant.Connect("/var/run/wpa_supplicant/eth0");
}

inline WifiListWidget::NetworkInfo *
WifiListWidget::FindByID(int id)
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
WifiListWidget::FindByBSSID(const char *bssid)
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
WifiListWidget::FindVisibleBySSID(const char *ssid)
{
  auto f = std::find_if(networks.begin(), networks.end(),
                        [ssid](const NetworkInfo &info) {
                          return info.signal_level >= 0 && info.ssid == ssid;
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
    info->signal_level = found.signal_level;
    info->old_visible = false;
  }
}

inline void
WifiListWidget::UpdateScanResults()
{
  WifiVisibleNetwork *buffer = new WifiVisibleNetwork[networks.capacity()];
  int n = wpa_supplicant.ScanResults(buffer, networks.capacity());
  if (n >= 0)
    MergeList(buffer, n);

  delete[] buffer;
}

inline WifiListWidget::NetworkInfo *
WifiListWidget::Find(const WifiConfiguredNetworkInfo &c)
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
  dest.signal_level = -1;
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
  int n = wpa_supplicant.ListNetworks(buffer, networks.capacity());
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
        info.signal_level = -1;

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

  if (EnsureConnected()) {
    wpa_supplicant.Status(status);

    for (auto &i : networks)
      i.old_visible = i.old_configured = true;

    UpdateScanResults();
    UpdateConfigured();

    /* remove items that are still marked as "old" */
    SweepList();
  } else
    networks.clear();

  GetList().SetLength(networks.size());

  UpdateButtons();
}

void
ShowWifiDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WifiListWidget widget;
  WidgetDialog dialog(look);
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Wifi"), &widget);
  widget.CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  dialog.StealWidget();
}
