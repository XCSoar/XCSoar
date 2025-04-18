// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DeviceListDialog.hpp"
#include "DeviceEditWidget.hpp"
#include "Vega/VegaDialogs.hpp"
#include "BlueFly/BlueFlyDialogs.hpp"
#include "ManageI2CPitotDialog.hpp"
#include "ManageCAI302Dialog.hpp"
#include "ManageFlarmDialog.hpp"
#include "LX/ManageLXNAVVarioDialog.hpp"
#include "LX/ManageNanoDialog.hpp"
#include "LX/ManageLX16xxDialog.hpp"
#include "PortMonitor.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "UIGlobals.hpp"
#include "util/StaticString.hxx"
#include "util/Macros.hpp"
#include "Device/MultipleDevices.hpp"
#include "Device/Descriptor.hpp"
#include "Device/Register.hpp"
#include "Device/Port/Listener.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "ui/event/Notify.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/ListWidget.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/control/List.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Simulator.hpp"
#include "Logger/ExternalLogger.hpp"
#include "Profile/Current.hpp"
#include "Profile/Profile.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Interface.hpp"

#ifdef ANDROID
#include "java/Global.hxx"
#include "Android/Main.hpp"
#include "Android/BluetoothHelper.hpp"
#endif

using namespace UI;

class DeviceListWidget final
  : public ListWidget,
    NullBlackboardListener, PortListener {
  DeviceBlackboard &device_blackboard;
  MultipleDevices *const devices;

  const DialogLook &look;

  unsigned font_height;

  struct Flags {
    bool duplicate:1;
    bool open:1, error:1;
    bool alive:1, location:1, gps:1, baro:1, pitot:1, airspeed:1, vario:1, traffic:1;
    bool temperature:1;
    bool humidity:1;
    bool radio:1, transponder:1;
    bool engine:1;
    bool debug:1;

#ifdef ANDROID
    bool bluetooth_disabled:1;
#else
    static constexpr bool bluetooth_disabled = false;
#endif

    int8_t battery_percent;

    void Set(const DeviceConfig &config, const DeviceDescriptor *device,
             const NMEAInfo &basic) {
      /* if a DeviceDescriptor is "unconfigured" but its DeviceConfig
         contains a valid configuration, then it got disabled by
         DeviceConfigOverlaps(), i.e. it's duplicate */
      duplicate = !config.IsDisabled() && (device != nullptr && !device->IsConfigured());

      switch (device != nullptr ? device->GetState() : PortState::LIMBO) {
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
      pitot = basic.pitot_pressure_available;
      airspeed = basic.airspeed_available;
      vario = basic.total_energy_vario_available;
      traffic = basic.flarm.IsDetected();
      temperature = basic.temperature_available;
      humidity = basic.humidity_available;
      debug = device != nullptr && device->IsDumpEnabled();
      radio = basic.settings.has_active_frequency || 
        basic.settings.has_standby_frequency;
      transponder = basic.settings.has_transponder_code;
      engine = basic.engine.IsAnyDefined();

#ifdef ANDROID
      bluetooth_disabled = config.IsAndroidBluetooth() &&
        bluetooth_helper != nullptr &&
        !bluetooth_helper->IsEnabled(Java::GetEnv());
#endif

      battery_percent = basic.battery_level_available
        ? (int)basic.battery_level
        : -1;
    }
  };

  union Item {
  private:
    Flags flags;
    uint32_t i;

    static_assert(sizeof(flags) <= sizeof(i), "wrong size");

  public:
    void Clear() {
      i = 0;
    }

    void Set(const DeviceConfig &config, const DeviceDescriptor *device,
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

  static_assert(sizeof(Item) == 4, "wrong size");

  Item items[NUMDEV];
  tstring error_messages[NUMDEV];

  Button *disable_button;
  Button *reconnect_button, *flight_button;
  Button *edit_button;
  Button *manage_button, *monitor_button;
  Button *debug_button;

  Notify port_state_notify{[this]{
    if (RefreshList())
      UpdateButtons();
  }};

public:
  DeviceListWidget(DeviceBlackboard &_device_blackboard,
                   MultipleDevices *_devices,
                   const DialogLook &_look) noexcept
    :device_blackboard(_device_blackboard),
     devices(_devices),
     look(_look) {}

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
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    ListWidget::Show(rc);

    if (devices != nullptr)
      devices->AddPortListener(*this);
    CommonInterface::GetLiveBlackboard().AddListener(*this);

    RefreshList();
    UpdateButtons();
  }

  void Hide() noexcept override {
    ListWidget::Hide();

    CommonInterface::GetLiveBlackboard().RemoveListener(*this);

    if (devices != nullptr)
      devices->RemovePortListener(*this);
  }

  /* virtual methods from class List::Handler */
   void OnPaintItem(Canvas &canvas, const PixelRect rc,
                    unsigned idx) noexcept override;
  void OnCursorMoved(unsigned index) noexcept override;

private:
  /* virtual methods from class BlackboardListener */
  virtual void OnGPSUpdate(const MoreData &basic) override;

  /* virtual methods from class PortListener */
  void PortStateChanged() noexcept override {
    port_state_notify.SendNotification();
  }
};

void
DeviceListWidget::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  const unsigned margin = Layout::GetTextPadding();
  font_height = look.list.font->GetHeight();
  CreateList(parent, look, rc, 3 * margin + font_height +
             look.small_font.GetHeight()).SetLength(NUMDEV);

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
          devices != nullptr ? &(*devices)[i] : nullptr,
          device_blackboard.RealState(i));

    if (n != item) {
      item = n;
      modified = true;
    }

    if (devices != nullptr) {
      auto error_message = (*devices)[i].GetErrorMessage();
      if (error_message != error_messages[i]) {
        error_messages[i] = std::move(error_message);
        modified = true;
      }
    }
  }

  if (modified)
    GetList().Invalidate();
  return modified;
}

void
DeviceListWidget::CreateButtons(WidgetDialog &dialog)
{
  edit_button = dialog.AddButton(_("Edit"), [this](){
    EditCurrent();
  });

  flight_button = dialog.AddButton(_("Flight download"), [this](){
    DownloadFlightFromCurrent();
  });

  manage_button = dialog.AddButton(_("Manage"), [this](){
    ManageCurrent();
  });

  monitor_button = dialog.AddButton(_("Monitor"), [this](){
    MonitorCurrent();
  });

  reconnect_button = dialog.AddButton(_("Reconnect"), [this](){
    ReconnectCurrent();
  });

  disable_button = dialog.AddButton(_("Disable"), [this](){
    EnableDisableCurrent();
  });

  debug_button = dialog.AddButton(_("Debug"), [this](){
    DebugCurrent();
  });
}

void
DeviceListWidget::UpdateButtons()
{
  const unsigned current = GetList().GetCursorIndex();

  if (current < NUMDEV) {
    const auto &config = CommonInterface::GetSystemSettings().devices[current];

    if (config.port_type != DeviceConfig::PortType::DISABLED) {
      disable_button->SetEnabled(true);
      disable_button->SetCaption(config.enabled ? _("Disable") : _("Enable"));
    } else
      disable_button->SetEnabled(false);
  } else
    disable_button->SetEnabled(false);

  if (is_simulator() || current >= NUMDEV || devices == nullptr) {
    reconnect_button->SetEnabled(false);
    flight_button->SetEnabled(false);
    manage_button->SetEnabled(false);
    monitor_button->SetEnabled(false);
    debug_button->SetEnabled(false);
  } else {
    const DeviceDescriptor &device = (*devices)[current];

    reconnect_button->SetEnabled(!device.GetConfig().IsDisabled());
    flight_button->SetEnabled(device.IsLogger());
    manage_button->SetEnabled(device.IsManageable());
    monitor_button->SetEnabled(device.GetConfig().UsesPort());
    debug_button->SetEnabled(device.GetConfig().UsesPort() &&
                             device.GetState() == PortState::READY);
  }

  edit_button->SetEnabled(current < NUMDEV);
}

void
DeviceListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                              unsigned idx) noexcept
{
  assert(idx < NUMDEV);

  const DeviceConfig &config =
    CommonInterface::SetSystemSettings().devices[idx];
  const Flags flags(*items[idx]);

  const unsigned margin = Layout::GetTextPadding();

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
  canvas.DrawText(rc.GetTopLeft() + PixelSize{margin, margin}, text);

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

    if (flags.pitot) {
      buffer.append(_T("; "));
      buffer.append(_("Pitot"));
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

    if (flags.temperature || flags.humidity) {
      buffer.append(_T("; "));
      buffer.append(_T("Environment"));
    }

    if (flags.radio) {
      buffer.append(_T("; "));
      buffer.append(_T("Radio"));
    }

    if (flags.transponder) {
      buffer.append(_T("; XPDR"));
    }

    if (flags.engine)
      buffer.append(_T("; Engine"));

    if (flags.debug) {
      buffer.append(_T("; "));
      buffer.append(_("Debug"));
    }

    if (flags.battery_percent >= 0) {
      buffer.append(_T("; "));
      buffer.append(_("Battery"));
      buffer.AppendFormat(_T("=%d%%"), flags.battery_percent);
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
  } else if (flags.bluetooth_disabled) {
    status = _("Bluetooth is disabled");
  } else if (flags.duplicate) {
    status = _("Duplicate");
  } else if (flags.error) {
    if (error_messages[idx].empty())
      status = _("Error");
    else
      status = error_messages[idx].c_str();
  } else {
    status = _("Not connected");
  }

  canvas.Select(look.small_font);
  canvas.DrawText(rc.GetTopLeft() + PixelSize{margin, 2 * margin + font_height},
                  status);
}

void
DeviceListWidget::OnCursorMoved([[maybe_unused]] unsigned index) noexcept
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
  Profile::SetDeviceConfig(Profile::map, index, config);
  Profile::Save();

  /* update the UI */

  GetList().Invalidate();
  UpdateButtons();

  /* .. and reopen the device */

  if (devices != nullptr) {
    DeviceDescriptor &descriptor = (*devices)[index];
    descriptor.SetConfig(config);

    /* this OperationEnvironment instance must be persistent, because
       DeviceDescriptor::Open() is asynchronous */
    static MessageOperationEnvironment env;
    descriptor.Reopen(env);
  }
}

inline void
DeviceListWidget::ReconnectCurrent()
{
  if (devices == nullptr)
    return;

  const unsigned current = GetList().GetCursorIndex();
  if (current >= NUMDEV)
    return;

#ifdef ANDROID
  const DeviceConfig &config =
    CommonInterface::SetSystemSettings().devices[current];
  if ((config.port_type == DeviceConfig::PortType::RFCOMM ||
       config.port_type == DeviceConfig::PortType::BLE_HM10 ||
       config.port_type == DeviceConfig::PortType::RFCOMM_SERVER) &&
      bluetooth_helper != nullptr &&
      !bluetooth_helper->IsEnabled(Java::GetEnv())) {
    ShowMessageBox(_("Bluetooth is disabled"), _("Reconnect"),
                   MB_OK | MB_ICONERROR);
    return;
  }
#endif

  DeviceDescriptor &device = (*devices)[current];
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
  if (devices == nullptr)
    return;

  const unsigned current = GetList().GetCursorIndex();
  if (current >= NUMDEV)
    return;

  DeviceDescriptor &device = (*devices)[current];
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

  MessageOperationEnvironment env;
  const ScopeReturnDevice return_device{device, env};

  ExternalLogger::DownloadFlightFrom(device);
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
  Profile::SetDeviceConfig(Profile::map, index, config);
  Profile::Save();

  /* update the UI */

  GetList().Invalidate();
  UpdateButtons();

  /* .. and reopen the device */

  if (devices != nullptr) {
    DeviceDescriptor &descriptor = (*devices)[index];
    descriptor.SetConfig(widget.GetConfig());

    /* this OperationEnvironment instance must be persistent, because
       DeviceDescriptor::Open() is asynchronous */
    static MessageOperationEnvironment env;
    descriptor.Reopen(env);
  }
}

inline void
DeviceListWidget::ManageCurrent()
{
  if (devices == nullptr)
    return;

  const unsigned current = GetList().GetCursorIndex();
  if (current >= NUMDEV)
    return;

  DeviceDescriptor &descriptor = (*devices)[current];
  if (!descriptor.IsManageable())
    return;

#ifdef ANDROID
  const auto &config = descriptor.GetConfig();
  if (config.port_type == DeviceConfig::PortType::DROIDSOAR_V2 ||
      (config.port_type == DeviceConfig::PortType::I2CPRESSURESENSOR &&
       config.press_use == DeviceConfig::PressureUse::PITOT)) {
    ManageI2CPitotDialog(UIGlobals::GetMainWindow(), look, descriptor);
    return;
  }
#endif

  if (descriptor.GetState() != PortState::READY) {
    ShowMessageBox(_("Device is not connected"), _("Manage"),
                   MB_OK | MB_ICONERROR);
    return;
  }

  if (!descriptor.Borrow()) {
    ShowMessageBox(_("Device is occupied"), _("Manage"), MB_OK | MB_ICONERROR);
    return;
  }

  MessageOperationEnvironment env;
  const ScopeReturnDevice return_device{descriptor, env};

  Device *device = descriptor.GetDevice();
  if (device == NULL) {
    return;
  }

  if (descriptor.IsDriver(_T("CAI 302")))
    ManageCAI302Dialog(UIGlobals::GetMainWindow(), look, *device);
  else if (descriptor.IsDriver(_T("FLARM"))) {
    FlarmVersion version;
    FlarmHardware hardware;

    {
      const std::lock_guard lock{device_blackboard.mutex};
      const NMEAInfo &basic = device_blackboard.RealState(current);
      version = basic.flarm.version;
    }

    ManageFlarmDialog(*device, version, hardware);
  } else if (descriptor.IsDriver(_T("LX"))) {
    DeviceInfo info, secondary_info;

    {
      const std::lock_guard lock{device_blackboard.mutex};
      const NMEAInfo &basic = device_blackboard.RealState(current);
      info = basic.device;
      secondary_info = basic.secondary_device;
    }

    LXDevice &lx_device = *(LXDevice *)device;
    if (lx_device.IsLXNAVVario())
      ManageLXNAVVarioDialog(lx_device, info, secondary_info);
    else if (lx_device.IsNano())
      ManageNanoDialog(lx_device, info);
    else if (lx_device.IsLX16xx())
      ManageLX16xxDialog(lx_device, info);
  } else if (descriptor.IsDriver(_T("Vega")))
    dlgConfigurationVarioShowModal(*device);
  else if (descriptor.IsDriver(_T("BlueFly")))
    dlgConfigurationBlueFlyVarioShowModal(*device);
}

inline void
DeviceListWidget::MonitorCurrent()
{
  if (devices == nullptr)
    return;

  const unsigned current = GetList().GetCursorIndex();
  if (current >= NUMDEV)
    return;

  DeviceDescriptor &descriptor = (*devices)[current];
  ShowPortMonitor(descriptor);
}

inline void
DeviceListWidget::DebugCurrent()
{
  if (devices == nullptr)
    return;

  const unsigned current = GetList().GetCursorIndex();
  if (current >= NUMDEV)
    return;

  DeviceDescriptor &device = (*devices)[current];
  if (!device.GetConfig().UsesPort() || device.GetState() != PortState::READY)
    return;

  static constexpr unsigned MINUTES = 10;

  device.EnableDumpTemporarily(std::chrono::minutes(MINUTES));
  RefreshList();

  StaticString<256> msg;
  msg.Format(_("Communication with this device will be logged for the next %u minutes."),
             MINUTES);
  ShowMessageBox(msg, _("Debug"), MB_OK | MB_ICONINFORMATION);
}

void
DeviceListWidget::OnGPSUpdate([[maybe_unused]] const MoreData &basic)
{
  if (RefreshList())
    UpdateButtons();
}

void
ShowDeviceList(DeviceBlackboard &device_blackboard, MultipleDevices *devices)
{
  TWidgetDialog<DeviceListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Devices"));
  dialog.SetWidget(device_blackboard, devices,
                   UIGlobals::GetDialogLook());
  dialog.GetWidget().CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);
  dialog.EnableCursorSelection();

  dialog.ShowModal();
}
