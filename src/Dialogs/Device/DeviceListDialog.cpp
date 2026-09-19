// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DeviceListDialog.hpp"
#include "DeviceEditWidget.hpp"
#include "Vega/VegaDialogs.hpp"
#include "BlueFly/BlueFlyDialogs.hpp"
#include "Stratux/ConfigurationDialog.hpp"
#include "GDL90/ConfigurationDialog.hpp"
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
#include "ui/canvas/Color.hpp"
#include "ui/control/List.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Simulator.hpp"
#include "Logger/ExternalLogger.hpp"
#include "Profile/Current.hpp"
#include "Profile/Profile.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

#ifdef ANDROID
#include "java/Global.hxx"
#include "Android/Main.hpp"
#include "Android/BluetoothHelper.hpp"
#endif

using namespace UI;

/* Same threshold as BatteryTimer::BATTERY_WARNING (host "Battery low"). */
static constexpr int BATTERY_WARNING_PERCENT = 10;

static void
DrawStatusToken(Canvas &canvas, PixelPoint &p,
                const Font &regular, const Font &bold,
                const char *text, bool used, bool &need_sep,
                bool warning=false) noexcept
{
  if (need_sep) {
    canvas.Select(regular);
    canvas.DrawText(p, "; ");
    p.x += canvas.CalcTextWidth("; ");
  }

  const Color old_color = canvas.GetTextColor();
  if (warning && HasColors())
    canvas.SetTextColor(COLOR_RED);

  canvas.Select(used || warning ? bold : regular);
  canvas.DrawText(p, text);
  p.x += canvas.CalcTextWidth(text);

  if (warning && HasColors())
    canvas.SetTextColor(old_color);

  need_sep = true;
}

class DeviceListWidget final
  : public ListWidget,
    NullBlackboardListener, PortListener {
  DeviceBlackboard &device_blackboard;
  MultipleDevices *const devices;

  const DialogLook &look;

  unsigned font_height;

  struct Flags {
    bool duplicate:1;
    bool open:1, error:1, connecting:1;
    bool alive:1, location:1, gps:1, baro:1, pitot:1, airspeed:1, vario:1, traffic:1;
    bool flarm_status:1;
    bool gdl90:1;
    bool foreflight_id:1;
    bool foreflight_ahrs:1;
    bool temperature:1;
    bool humidity:1;
    bool pressure:1;
    bool imu:1;
    bool accel:1;
    bool heart_rate:1;
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
        connecting = false;
        break;

      case PortState::FAILED:
        open = false;
        error = true;
        connecting = false;
        break;

      case PortState::LIMBO:
        open = false;
        error = false;
        connecting = true;
        break;
      }

      alive = basic.alive;
      location = basic.location_available;
      gps = basic.gps.fix_quality_available;
      baro = basic.baro_altitude_available ||
        basic.pressure_altitude_available;
      pressure = basic.static_pressure_available;
      pitot = basic.pitot_pressure_available;
      airspeed = basic.airspeed_available ||
        basic.dyn_pressure_available;
      vario = basic.netto_vario_available ||
        basic.total_energy_vario_available ||
        basic.noncomp_vario_available;
      traffic = basic.flarm.IsDetected();
      /* PFLAU heartbeat; expires after 10 s, unlike leftover targets */
      flarm_status = basic.flarm.status.available;
      /* GDL90 status follows protocol activity (heartbeat / any frame
         sets alive), not traffic presence — SoftRF may have ownship
         with an empty traffic list. */
      gdl90 = alive && config.UsesDriver() && config.driver_name == "GDL90";
      foreflight_id = config.UsesDriver() && config.driver_name == "GDL90" &&
        basic.device.license.equals("ForeFlight");
      foreflight_ahrs = config.UsesDriver() && config.driver_name == "GDL90" &&
        (basic.attitude.bank_angle_available ||
         basic.attitude.pitch_angle_available ||
         basic.attitude.heading_available);
      temperature = basic.temperature_available;
      humidity = basic.humidity_available;
      imu = basic.gyroscope.available;
      accel = basic.acceleration.available;
      heart_rate = basic.heart_rate_available;
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
    uint64_t i;

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

  static_assert(sizeof(Item) == 8, "wrong size");

  Item items[NUMDEV];
  std::string error_messages[NUMDEV];

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
  template<typename Pred>
  bool EarlierHas(unsigned idx, Pred pred) const noexcept {
    for (unsigned i = 0; i < idx; ++i)
      if ((*items[i]).alive && pred(*items[i]))
        return true;
    return false;
  }

  void DrawAliveStatus(Canvas &canvas, PixelPoint p,
                       unsigned idx) noexcept;

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

    reconnect_button->SetEnabled(!device.GetConfig().IsDisabled() && 
                                 !device.IsWaitingToCallOpen());
    flight_button->SetEnabled(device.IsLogger());
    manage_button->SetEnabled(device.IsManageable());
    monitor_button->SetEnabled(device.GetConfig().UsesPort());
    debug_button->SetEnabled(device.GetConfig().UsesPort() &&
                             device.GetState() == PortState::READY);
  }

  edit_button->SetEnabled(current < NUMDEV);
}

void
DeviceListWidget::DrawAliveStatus(Canvas &canvas, PixelPoint p,
                                  unsigned idx) noexcept
{
  const Flags flags(*items[idx]);
  const Font &regular = look.small_font;
  const Font &bold = look.small_font_bold;
  bool need_sep = false;

  if (flags.location)
    DrawStatusToken(canvas, p, regular, bold, _("GPS fix"),
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.location;
                    }), need_sep);
  else if (flags.gps)
    /* device sends GPGGA, but no valid location */
    DrawStatusToken(canvas, p, regular, bold, _("Bad GPS"),
                    false, need_sep, true);
  else
    DrawStatusToken(canvas, p, regular, bold, _("Connected"),
                    false, need_sep);

  if (flags.baro)
    DrawStatusToken(canvas, p, regular, bold, _("Baro"),
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.baro;
                    }), need_sep);

  if (flags.pressure)
    DrawStatusToken(canvas, p, regular, bold, _("Pressure"),
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.pressure;
                    }), need_sep);

  if (flags.pitot)
    DrawStatusToken(canvas, p, regular, bold, _("Pitot"),
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.pitot;
                    }), need_sep);

  if (flags.airspeed)
    DrawStatusToken(canvas, p, regular, bold, _("Airspeed"),
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.airspeed;
                    }), need_sep);

  if (flags.vario)
    DrawStatusToken(canvas, p, regular, bold, _("Vario"),
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.vario;
                    }), need_sep);

  if (flags.gdl90)
    DrawStatusToken(canvas, p, regular, bold, "GDL90",
                    flags.alive, need_sep);
  else if (flags.traffic)
    DrawStatusToken(canvas, p, regular, bold, "FLARM",
                    flags.flarm_status, need_sep);

  if (flags.foreflight_ahrs)
    DrawStatusToken(canvas, p, regular, bold, "ForeFlight AHRS",
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.foreflight_ahrs;
                    }), need_sep);

  if (flags.foreflight_id)
    DrawStatusToken(canvas, p, regular, bold, "ForeFlight ID",
                    true, need_sep);

  if (flags.temperature)
    DrawStatusToken(canvas, p, regular, bold, _("Temperature"),
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.temperature;
                    }), need_sep);

  if (flags.humidity)
    DrawStatusToken(canvas, p, regular, bold, _("Relative humidity"),
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.humidity;
                    }), need_sep);

  if (flags.imu)
    DrawStatusToken(canvas, p, regular, bold, _("IMU"),
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.imu;
                    }), need_sep);

  if (flags.accel)
    DrawStatusToken(canvas, p, regular, bold, "G",
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.accel;
                    }), need_sep);

  if (flags.heart_rate)
    DrawStatusToken(canvas, p, regular, bold, _("Heart Rate"),
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.heart_rate;
                    }), need_sep);

  if (flags.radio)
    DrawStatusToken(canvas, p, regular, bold, "Radio",
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.radio;
                    }), need_sep);

  if (flags.transponder)
    DrawStatusToken(canvas, p, regular, bold, "XPDR",
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.transponder;
                    }), need_sep);

  if (flags.engine)
    DrawStatusToken(canvas, p, regular, bold, "Engine",
                    !EarlierHas(idx, [](const Flags &f) {
                      return f.engine;
                    }), need_sep);

  if (flags.debug)
    DrawStatusToken(canvas, p, regular, bold, _("Debug"),
                    true, need_sep);

  if (flags.battery_percent >= 0) {
    StaticString<32> battery;
    battery.Format("%s=%d%%", _("Battery"), flags.battery_percent);
    DrawStatusToken(canvas, p, regular, bold, battery, false, need_sep,
                    flags.battery_percent < BATTERY_WARNING_PERCENT);
  }
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

  char port_name_buffer[128];
  const char *port_name =
    config.GetPortName(port_name_buffer, ARRAY_SIZE(port_name_buffer));

  StaticString<256> text("A: ");
  text[0u] += idx;

  if (config.UsesDriver()) {
    const char *driver_name = FindDriverDisplayName(config.driver_name);

    text.AppendFormat(_("%s on %s"), driver_name, port_name);
  } else {
    text.append(port_name);
  }

  canvas.Select(*look.list.font);
  canvas.DrawText(rc.GetTopLeft() + PixelSize{margin, margin}, text);

  /* show a list of features that are available in the second row;
     merge-priority flags are bold when this device is the first
     source (A before B, and so on) */

  const PixelPoint status_p =
    rc.GetTopLeft() + PixelSize{margin, 2 * margin + font_height};

  if (flags.alive) {
    DrawAliveStatus(canvas, status_p, idx);
    return;
  }

  const char *status;
  if (config.IsDisabled()) {
    status = _("Disabled");
  } else if (is_simulator() || !config.IsAvailable()) {
    status = _("N/A");
  } else if (flags.bluetooth_disabled) {
    status = _("Bluetooth is disabled");
  } else if (flags.duplicate) {
    status = _("Duplicate");
  } else if (flags.connecting) {
    status = _("Connecting...");
  } else if (flags.open) {
    bool need_sep = false;
    PixelPoint p = status_p;
    DrawStatusToken(canvas, p, look.small_font, look.small_font_bold,
                    _("No data"), false, need_sep);
    if (flags.debug)
      DrawStatusToken(canvas, p, look.small_font, look.small_font_bold,
                      _("Debug"), true, need_sep);
    return;
  } else if (flags.error) {
    if (error_messages[idx].empty())
      status = _("Error");
    else
      status = error_messages[idx].c_str();
  } else {
    status = _("Not connected");
  }

  canvas.Select(look.small_font);
  canvas.DrawText(status_p, status);
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
       config.port_type == DeviceConfig::PortType::BLE_SERIAL ||
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

  device.ResetFailureCounter();
  device.SlowReopen();
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

  if (descriptor.IsDriver("GDL90")) {
    ManageGDL90Dialog();
    return;
  }

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

  if (descriptor.IsDriver("CAI 302"))
    ManageCAI302Dialog(UIGlobals::GetMainWindow(), look, *device);
  else if (descriptor.IsDriver("Stratux"))
    ManageStratuxDialog(*device);
  else if (descriptor.IsDriver("FLARM")) {
    FlarmVersion version;
    FlarmHardware hardware;
    FlarmState state;

    {
      const std::lock_guard lock{device_blackboard.mutex};
      const NMEAInfo &basic = device_blackboard.RealState(current);
      version = basic.flarm.version;
      state = basic.flarm.state;
    }

    ManageFlarmDialog(*device, version, hardware, state);
  } else if (descriptor.IsDriver("LX")) {
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
  } else if (descriptor.IsDriver("Vega"))
    dlgConfigurationVarioShowModal(*device);
  else if (descriptor.IsDriver("BlueFly"))
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
ShowDeviceList(MultipleDevices *devices)
{
  /* Per-device NMEA (FLARM version, port flags) lives on
     DeviceBlackboard, not the merged InterfaceBlackboard. */
  DeviceBlackboard *device_blackboard =
    backend_components != nullptr
    ? backend_components->device_blackboard.get()
    : nullptr;
  if (device_blackboard == nullptr)
    return;

  TWidgetDialog<DeviceListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Devices"));
  dialog.SetWidget(*device_blackboard, devices,
                   UIGlobals::GetDialogLook());
  dialog.GetWidget().CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);
  dialog.EnableCursorSelection();

  dialog.ShowModal();
}
