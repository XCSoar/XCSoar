/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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

#include "DeviceListDialog.hpp"
#include "DeviceEditWidget.hpp"
#include "Vega/VegaDialogs.hpp"
#include "ManageCAI302Dialog.hpp"
#include "ManageFlarmDialog.hpp"
#include "LX/ManageV7Dialog.hpp"
#include "LX/ManageNanoDialog.hpp"
#include "LX/ManageLX16xxDialog.hpp"
#include "PortMonitor.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "UIGlobals.hpp"
#include "Util/TrivialArray.hpp"
#include "Util/StaticString.hpp"
#include "Util/Macros.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "Device/Register.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Components.hpp"
#include "Look/DialogLook.hpp"
#include "Form/List.hpp"
#include "Widget/ListWidget.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Simulator.hpp"
#include "Logger/ExternalLogger.hpp"
#include "Profile/Profile.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Interface.hpp"

#ifdef ANDROID
#include "Java/Global.hpp"
#include "Android/BluetoothHelper.hpp"
#endif

class DeviceListWidget final
  : public ListWidget, private ActionListener,
    private NullBlackboardListener {
  enum Buttons {
    DISABLE,
    RECONNECT, FLIGHT, EDIT, MANAGE, MONITOR,
    DEBUG,
  };

  const DialogLook &look;
  const TerminalLook &terminal_look;

  UPixelScalar font_height;

  struct Flags {
    bool duplicate:1;
    bool open:1, error:1;
    bool alive:1, location:1, gps:1, baro:1, airspeed:1, vario:1, traffic:1;
    bool debug:1, calibrate:1;

    void Set(const DeviceConfig &config, const DeviceDescriptor &device,
             const NMEAInfo &basic) {
      /* if a DeviceDescriptor is "unconfigured" but its DeviceConfig
         contains a valid configuration, then it got disabled by
         DeviceConfigOverlaps(), i.e. it's duplicate */
      duplicate = !config.IsDisabled() && !device.IsConfigured();

      switch (device.GetState()) {
      case PortState::READY:
        open = true;
        error = false;
        break;

      case PortState::FAILED:
        open = false;
        error = true;
        break;

      case PortState::LIMBO:
        open = false;
        error = false;
        break;
      }

      alive = basic.alive;
      location = basic.location_available;
      gps = basic.gps.fix_quality_available;
      baro = basic.baro_altitude_available ||
        basic.pressure_altitude_available ||
        basic.static_pressure_available;
      airspeed = basic.airspeed_available ||
        basic.dyn_pressure_available;
      vario = basic.total_energy_vario_available;
      traffic = basic.flarm.IsDetected();
      debug = device.IsDumpEnabled();
      calibrate = device.IsCalibrating();
    }
  };

  union Item {
  private:
    Flags flags;
    uint16_t i;

    static_assert(sizeof(flags) <= sizeof(i), "wrong size");

  public:
    void Clear() {
      i = 0;
    }

    void Set(const DeviceConfig &config, const DeviceDescriptor &device,
             const NMEAInfo &basic) {
      i = 0;
      flags.Set(config, device, basic);
    }

    bool operator==(const Item &other) const {
      return i == other.i;
    }

    bool operator!=(const Item &other) const {
      return i != other.i;
    }

    const Flags &operator->() const {
      return flags;
    }

    const Flags &operator*() const {
      return flags;
    }
  };

  static_assert(sizeof(Item) == 2, "wrong size");

  Item items[NUMDEV];

  WndButton *disable_button;
  WndButton *reconnect_button, *flight_button;
  WndButton *edit_button;
  WndButton *manage_button, *monitor_button;
  WndButton *debug_button;

public:
  DeviceListWidget(const DialogLook &_look, const TerminalLook &_terminal_look)
    :look(_look), terminal_look(_terminal_look) {}

  void CreateButtons(WidgetDialog &dialog);

protected:
  bool RefreshList();

  void UpdateButtons();

  void EnableDisableCurrent();
  void ReconnectCurrent();
  void DownloadFlightFromCurrent();
  void EditCurrent();
  void ManageCurrent();
  void MonitorCurrent();
  void DebugCurrent();

public:
  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual void Unprepare() override {
    DeleteWindow();
  }

  virtual void Show(const PixelRect &rc) override {
    ListWidget::Show(rc);

    CommonInterface::GetLiveBlackboard().AddListener(*this);

    RefreshList();
    UpdateButtons();
  }

  virtual void Hide() override {
    ListWidget::Hide();

    CommonInterface::GetLiveBlackboard().RemoveListener(*this);
  }

  /* virtual methods from class List::Handler */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;
  virtual void OnCursorMoved(unsigned index) override;

private:
  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;

  /* virtual methods from class BlackboardListener */
  virtual void OnGPSUpdate(const MoreData &basic) override;
};

void
DeviceListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  const UPixelScalar margin = Layout::GetTextPadding();
  font_height = look.list.font->GetHeight();
  CreateList(parent, look, rc, 3 * margin + font_height +
             look.small_font->GetHeight()).SetLength(NUMDEV);

  for (Item &i : items)
    i.Clear();

  UpdateButtons();
}

bool
DeviceListWidget::RefreshList()
{
  bool modified = false;
  for (unsigned i = 0; i < NUMDEV; ++i) {
    Item &item = items[i];

    Item n;
    n.Set(CommonInterface::GetSystemSettings().devices[i],
          *device_list[i], device_blackboard->RealState(i));

    if (n != item) {
      item = n;
      modified = true;
    }
  }

  if (modified)
    GetList().Invalidate();
  return modified;
}

void
DeviceListWidget::CreateButtons(WidgetDialog &dialog)
{
  disable_button = dialog.AddButton(_("Disable"), *this, DISABLE);
  reconnect_button = dialog.AddButton(_("Reconnect"), *this, RECONNECT);
  flight_button = dialog.AddButton(_("Flight download"), *this, FLIGHT);
  edit_button = dialog.AddButton(_("Edit"), *this, EDIT);
  manage_button = dialog.AddButton(_("Manage"), *this, MANAGE);
  monitor_button = dialog.AddButton(_("Monitor"), *this, MONITOR);
  debug_button = dialog.AddButton(_("Debug"), *this, DEBUG);
}

void
DeviceListWidget::UpdateButtons()
{
  const unsigned current = GetList().GetCursorIndex();

  if (current < NUMDEV) {
    const auto &config = CommonInterface::GetSystemSettings().devices[current];

    if (config.port_type != DeviceConfig::PortType::DISABLED) {
      disable_button->SetEnabled(true);
      disable_button->SetText(config.enabled ? _("Disable") : _("Enable"));
    } else
      disable_button->SetEnabled(false);
  } else
    disable_button->SetEnabled(false);

  if (is_simulator() || current >= NUMDEV) {
    reconnect_button->SetEnabled(false);
    flight_button->SetEnabled(false);
    manage_button->SetEnabled(false);
    monitor_button->SetEnabled(false);
    debug_button->SetEnabled(false);
  } else {
    const DeviceDescriptor &device = *device_list[current];

    reconnect_button->SetEnabled(device.IsConfigured());
    flight_button->SetEnabled(device.IsLogger());
    manage_button->SetEnabled(device.IsManageable() ||
                              device.IsCalibrable());
    monitor_button->SetEnabled(device.GetConfig().UsesPort());
    debug_button->SetEnabled(device.GetConfig().UsesPort() &&
                             device.GetState() == PortState::READY);
  }

  edit_button->SetEnabled(current < NUMDEV);
}

void
DeviceListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned idx)
{
  assert(idx < NUMDEV);

  const DeviceConfig &config =
    CommonInterface::SetSystemSettings().devices[idx];
  const Flags flags(*items[idx]);

  const UPixelScalar margin = Layout::GetTextPadding();

  TCHAR port_name_buffer[128];
  const TCHAR *port_name =
    config.GetPortName(port_name_buffer, ARRAY_SIZE(port_name_buffer));

  StaticString<256> text(_T("A: "));
  text[0u] += idx;

  if (config.UsesDriver()) {
    const TCHAR *driver_name = FindDriverDisplayName(config.driver_name);

    text.AppendFormat(_("%s on %s"), driver_name, port_name);
  } else {
    text.append(port_name);
  }

  canvas.Select(*look.list.font);
  canvas.DrawText(rc.left + margin, rc.top + margin, text);

  /* show a list of features that are available in the second row */

  StaticString<256> buffer;
  const TCHAR *status;
  if (flags.alive) {
    if (flags.location) {
      buffer = _("GPS fix");
    } else if (flags.gps) {
      /* device sends GPGGA, but no valid location */
      buffer = _("Bad GPS");
    } else {
      buffer = _("Connected");
    }

    if (flags.baro) {
      buffer.append(_T("; "));
      buffer.append(_("Baro"));
    }

    if (flags.airspeed) {
      buffer.append(_T("; "));
      buffer.append(_("Airspeed"));
    }

    if (flags.vario) {
      buffer.append(_T("; "));
      buffer.append(_("Vario"));
    }

    if (flags.traffic)
      buffer.append(_T("; FLARM"));

    if (flags.debug) {
      buffer.append(_T("; "));
      buffer.append(_("Debug"));
    }

    if (flags.calibrate) {
      buffer.append(_T("; "));
      buffer.append(_("Calibrate"));
    }

    status = buffer;
  } else if (config.IsDisabled()) {
    status = _("Disabled");
  } else if (is_simulator() || !config.IsAvailable()) {
    status = _("N/A");
  } else if (flags.open) {
    buffer = _("No data");

    if (flags.debug) {
      buffer.append(_T("; "));
      buffer.append(_("Debug"));
    }

    status = buffer;
#ifdef ANDROID
  } else if ((config.port_type == DeviceConfig::PortType::RFCOMM ||
              config.port_type == DeviceConfig::PortType::RFCOMM_SERVER) &&
             !BluetoothHelper::isEnabled(Java::GetEnv())) {
    status = _("Bluetooth is disabled");
#endif
  } else if (flags.duplicate) {
    status = _("Duplicate");
  } else if (flags.error) {
    status = _("Error");
  } else {
    status = _("Not connected");
  }

  canvas.Select(*look.small_font);
  canvas.DrawText(rc.left + margin, rc.top + 2 * margin + font_height,
                  status);
}

void
DeviceListWidget::OnCursorMoved(unsigned index)
{
  UpdateButtons();
}

inline void
DeviceListWidget::EnableDisableCurrent()
{
  const unsigned index = GetList().GetCursorIndex();
  if (index >= NUMDEV)
    return;

  DeviceConfig &config = CommonInterface::SetSystemSettings().devices[index];
  if (config.port_type == DeviceConfig::PortType::DISABLED)
    return;

  /* save new config to profile .. */

  config.enabled = !config.enabled;
  Profile::SetDeviceConfig(index, config);
  Profile::Save();

  /* .. and reopen the device */

  DeviceDescriptor &descriptor = *device_list[index];
  descriptor.SetConfig(config);

  GetList().Invalidate();
  UpdateButtons();

  /* this OperationEnvironment instance must be persistent, because
     DeviceDescriptor::Open() is asynchronous */
  static MessageOperationEnvironment env;
  descriptor.Reopen(env);
}

inline void
DeviceListWidget::ReconnectCurrent()
{
  const unsigned current = GetList().GetCursorIndex();
  if (current >= NUMDEV)
    return;

#ifdef ANDROID
  const DeviceConfig &config =
    CommonInterface::SetSystemSettings().devices[current];
  if ((config.port_type == DeviceConfig::PortType::RFCOMM ||
       config.port_type == DeviceConfig::PortType::RFCOMM_SERVER) &&
      !BluetoothHelper::isEnabled(Java::GetEnv())) {
    ShowMessageBox(_("Bluetooth is disabled"), _("Reconnect"),
                   MB_OK | MB_ICONERROR);
    return;
  }
#endif

  DeviceDescriptor &device = *device_list[current];
  if (device.IsBorrowed()) {
    ShowMessageBox(_("Device is occupied"), _("Reconnect"), MB_OK | MB_ICONERROR);
    return;
  }

  /* this OperationEnvironment instance must be persistent, because
     DeviceDescriptor::Open() is asynchronous */
  static MessageOperationEnvironment env;
  device.ResetFailureCounter();
  device.Reopen(env);
}

inline void
DeviceListWidget::DownloadFlightFromCurrent()
{
  const unsigned current = GetList().GetCursorIndex();
  if (current >= NUMDEV)
    return;

  DeviceDescriptor &device = *device_list[current];
  if (!device.IsLogger())
    return;

  if (device.GetState() != PortState::READY) {
    ShowMessageBox(_("Device is not connected"), _("Manage"),
                   MB_OK | MB_ICONERROR);
    return;
  }

  if (!device.Borrow()) {
    ShowMessageBox(_("Device is occupied"), _("Manage"), MB_OK | MB_ICONERROR);
    return;
  }

  ExternalLogger::DownloadFlightFrom(device);
  device.Return();
}

inline void
DeviceListWidget::EditCurrent()
{
  const unsigned current = GetList().GetCursorIndex();
  if (current >= NUMDEV)
    return;

  const unsigned index = current;
  DeviceConfig &config = CommonInterface::SetSystemSettings().devices[index];
  DeviceEditWidget widget(config);

  if (!DefaultWidgetDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                           _("Edit device"), widget))
    /* not modified */
    return;

  /* save new config to profile .. */

  config = widget.GetConfig();
  Profile::SetDeviceConfig(index, config);
  Profile::Save();

  /* .. and reopen the device */

  DeviceDescriptor &descriptor = *device_list[index];
  descriptor.SetConfig(widget.GetConfig());

  GetList().Invalidate();
  UpdateButtons();

  /* this OperationEnvironment instance must be persistent, because
     DeviceDescriptor::Open() is asynchronous */
  static MessageOperationEnvironment env;
  descriptor.Reopen(env);
}

inline void
DeviceListWidget::ManageCurrent()
{
  const unsigned current = GetList().GetCursorIndex();
  if (current >= NUMDEV)
    return;

  DeviceDescriptor &descriptor = *device_list[current];
  if (!descriptor.IsManageable())
    return;

  if (descriptor.GetState() != PortState::READY) {
    ShowMessageBox(_("Device is not connected"), _("Manage"),
                   MB_OK | MB_ICONERROR);
    return;
  }

  if (!descriptor.Borrow()) {
    ShowMessageBox(_("Device is occupied"), _("Manage"), MB_OK | MB_ICONERROR);
    return;
  }

  if (descriptor.IsCalibrable()) {
    if (descriptor.GetConfig().press_use == DeviceConfig::PressureUse::DYNAMIC) {
      descriptor.Calibrate(0);
      descriptor.Return();
      return;
    }
  }

  Device *device = descriptor.GetDevice();
  if (device == NULL) {
    descriptor.Return();
    return;
  }

  if (descriptor.IsDriver(_T("CAI 302")))
    ManageCAI302Dialog(UIGlobals::GetMainWindow(), look, *device);
  else if (descriptor.IsDriver(_T("FLARM"))) {
    device_blackboard->mutex.Lock();
    const NMEAInfo &basic = device_blackboard->RealState(current);
    const FlarmVersion version = basic.flarm.version;
    device_blackboard->mutex.Unlock();

    ManageFlarmDialog(*device, version);
  } else if (descriptor.IsDriver(_T("LX"))) {
    device_blackboard->mutex.Lock();
    const NMEAInfo &basic = device_blackboard->RealState(current);
    const DeviceInfo info = basic.device;
    const DeviceInfo secondary_info = basic.secondary_device;
    device_blackboard->mutex.Unlock();

    LXDevice &lx_device = *(LXDevice *)device;
    if (lx_device.IsV7())
      ManageV7Dialog(lx_device, info, secondary_info);
    else if (lx_device.IsNano())
      ManageNanoDialog(lx_device, info);
    else if (lx_device.IsLX16xx())
      ManageLX16xxDialog(lx_device, info);
  } else if (descriptor.IsDriver(_T("Vega")))
    dlgConfigurationVarioShowModal(*device);

  MessageOperationEnvironment env;
  descriptor.EnableNMEA(env);
  descriptor.Return();
}

inline void
DeviceListWidget::MonitorCurrent()
{
  const unsigned current = GetList().GetCursorIndex();
  if (current >= NUMDEV)
    return;

  DeviceDescriptor &descriptor = *device_list[current];
  ShowPortMonitor(UIGlobals::GetMainWindow(), look, terminal_look,
                  descriptor);
}

inline void
DeviceListWidget::DebugCurrent()
{
  const unsigned current = GetList().GetCursorIndex();
  if (current >= NUMDEV)
    return;

  DeviceDescriptor &device = *device_list[current];
  if (!device.GetConfig().UsesPort() || device.GetState() != PortState::READY)
    return;

  static constexpr unsigned MINUTES = 10;

  device.EnableDumpTemporarily(MINUTES * 60000);
  RefreshList();

  StaticString<256> msg;
  msg.Format(_("Communication with this device will be logged for the next %u minutes."),
             MINUTES);
  ShowMessageBox(msg, _("Debug"), MB_OK | MB_ICONINFORMATION);
}

void
DeviceListWidget::OnAction(int id)
{
  switch (id) {
  case DISABLE:
    EnableDisableCurrent();
    break;

  case RECONNECT:
    ReconnectCurrent();
    break;

  case FLIGHT:
    DownloadFlightFromCurrent();
    break;

  case EDIT:
    EditCurrent();
    break;

  case MANAGE:
    ManageCurrent();
    break;

  case MONITOR:
    MonitorCurrent();
    break;

  case DEBUG:
    DebugCurrent();
    break;
  }
}

void
DeviceListWidget::OnGPSUpdate(const MoreData &basic)
{
  if (RefreshList())
    UpdateButtons();
}

void
ShowDeviceList(const TerminalLook &terminal_look)
{
  DeviceListWidget widget(UIGlobals::GetDialogLook(), terminal_look);

  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Devices"), &widget);
  dialog.AddButton(_("Close"), mrOK);
  widget.CreateButtons(dialog);

  dialog.ShowModal();
  dialog.StealWidget();
}
