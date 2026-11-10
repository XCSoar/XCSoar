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
#include "Dialogs/HelpDialog.hpp"
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
#include "Widget/GroupedListWidget.hpp"
#include "Form/Button.hpp"
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

#include <string>

#ifdef ANDROID
#include "java/Global.hxx"
#include "Android/Main.hpp"
#include "Android/BluetoothHelper.hpp"
#endif

using namespace UI;

/* Same threshold as BatteryTimer::BATTERY_WARNING (host "Battery low"). */
static constexpr int BATTERY_WARNING_PERCENT = 10;

static const char *
GetDeviceListHelp() noexcept
{
  return
    /* xgettext:no-c-format */
    _("XCSoar uses the first device that supplies each value "
           "(device A before B, and so on). "
           "Flags in brackets are the values XCSoar is using from "
           "this device. "
           "Flags without brackets mean an earlier device already "
           "supplies the same data. "
           "Baro is height with the current QNH; QNE is pressure "
           "altitude; Alt IGC is the altitude the logger writes. "
           "QNE and Pressure are the same ambient pressure, so only "
           "the earlier of those two is in brackets. "
           "Bad GPS and a battery below 10% are shown as a red badge "
           "on a colour display.");
}

/**
 * The status line is one font.  Brackets take the place of the bold
 * face: a flag in brackets is the value XCSoar is using.
 */
static void
AppendFlag(std::string &line, const char *text, bool used)
{
  if (!line.empty())
    line.append("; ");

  if (used)
    line.push_back('[');

  line.append(text);

  if (used)
    line.push_back(']');
}

static void
NoteWarning(std::string &badge, std::string &badge2,
            const char *text)
{
  if (badge.empty())
    badge = text;
  else
    badge2 = text;
}

class DeviceListWidget final
  : public GroupedListWidget,
    NullBlackboardListener, PortListener {
  using ItemOptions = GroupedListWidget::ItemOptions;
  using BadgeStyle = GroupedListWidget::BadgeStyle;

  DeviceBlackboard &device_blackboard;
  MultipleDevices *const devices;

  /** true while the port and blackboard listeners are installed */
  bool listening = false;

  struct Flags {
    bool duplicate:1;
    bool open:1, error:1, connecting:1;
    bool alive:1, location:1, gps:1, baro:1, pressure_altitude:1;
    bool igc_altitude:1, pitot:1, airspeed:1, vario:1, traffic:1;
    bool gdl90:1;
    bool foreflight_id:1;
    bool foreflight_ahrs:1;
    bool temperature:1;
    bool humidity:1;
    bool pressure:1;
    bool imu:1;
    bool accel:1;
    bool heart_rate:1;
    bool blood_oxygen:1;
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
      baro = basic.baro_altitude_available;
      pressure_altitude = basic.pressure_altitude_available;
      igc_altitude = basic.igc_pressure_altitude_available;
      pressure = basic.static_pressure_available;
      pitot = basic.pitot_pressure_available;
      airspeed = basic.airspeed_available ||
        basic.dyn_pressure_available;
      vario = basic.netto_vario_available ||
        basic.total_energy_vario_available ||
        basic.noncomp_vario_available;
      /* PFLAU heartbeat; expires after 10 s */
      traffic = basic.flarm.status.available;
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
      blood_oxygen = basic.blood_oxygen_available;
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
  std::string captions[NUMDEV];
  std::string statuses[NUMDEV];
  std::string badge_text[NUMDEV];
  std::string badge2_text[NUMDEV];

  Button *disable_button = nullptr;
  Button *reconnect_button = nullptr, *flight_button = nullptr;
  Button *edit_button = nullptr;
  Button *manage_button = nullptr, *monitor_button = nullptr;
  Button *debug_button = nullptr;

  Notify port_state_notify{[this]{ Refresh(); }};

public:
  DeviceListWidget(DeviceBlackboard &_device_blackboard,
                   MultipleDevices *_devices) noexcept
    :GroupedListWidget(UIGlobals::GetDialogLook()),
     device_blackboard(_device_blackboard),
     devices(_devices)
  {
    SetVisibilityCallback([this](bool visible){ Listen(visible); });
    SetCursorCallback([this](int){ UpdateButtons(); });
  }

  void CreateButtons(WidgetDialog &dialog);

protected:
  /** @return true when a row's words have changed */
  bool Load();

  void Build();
  void Refresh();

  void Listen(bool visible);

  [[nodiscard]]
  int Current() const noexcept;

  [[nodiscard]]
  std::string MakeCaption(unsigned idx) const;

  void FormatRow(unsigned idx, std::string &line,
                 std::string &badge, std::string &badge2) const;

  void UpdateButtons();

  void EnableDisableCurrent();
  void ReconnectCurrent();
  void DownloadFlightFromCurrent();
  void EditCurrent();
  void ManageCurrent();
  void MonitorCurrent();
  void DebugCurrent();

  template<typename Pred>
  bool EarlierHas(unsigned idx, Pred pred) const noexcept {
    for (unsigned i = 0; i < idx; ++i)
      if ((*items[i]).alive && pred(*items[i]))
        return true;
    return false;
  }

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

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
  Load();
  Build();
  GroupedListWidget::Prepare(parent, rc);
  UpdateButtons();
}

void
DeviceListWidget::Listen(bool visible)
{
  if (visible) {
    if (!listening) {
      if (devices != nullptr)
        devices->AddPortListener(*this);
      CommonInterface::GetLiveBlackboard().AddListener(*this);
      listening = true;
    }
    Refresh();
    return;
  }

  if (!listening)
    return;

  listening = false;
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
  if (devices != nullptr)
    devices->RemovePortListener(*this);
}

int
DeviceListWidget::Current() const noexcept
{
  const int i = GetCursorIndex();
  if (i < 0 || (unsigned)i >= NUMDEV)
    return -1;
  return i;
}

std::string
DeviceListWidget::MakeCaption(unsigned idx) const
{
  const DeviceConfig &config =
    CommonInterface::GetSystemSettings().devices[idx];

  char port_name_buffer[128];
  const char *port_name =
    config.GetPortName(port_name_buffer, ARRAY_SIZE(port_name_buffer));

  StaticString<256> text("A: ");
  text[0u] += idx;

  if (config.UsesDriver()) {
    const char *driver_name = FindDriverDisplayName(config.driver_name);
    text.AppendFormat(_("%s on %s"), driver_name, port_name);
  } else
    text.append(port_name);

  return std::string{text.c_str()};
}

bool
DeviceListWidget::Load()
{
  bool changed = false;

  for (unsigned i = 0; i < NUMDEV; ++i) {
    Item n;
    n.Set(CommonInterface::GetSystemSettings().devices[i],
          devices != nullptr ? &(*devices)[i] : nullptr,
          device_blackboard.RealState(i));
    items[i] = n;

    if (devices != nullptr)
      error_messages[i] = (*devices)[i].GetErrorMessage();

    std::string caption = MakeCaption(i);
    std::string status, badge, badge2;
    FormatRow(i, status, badge, badge2);

    if (caption != captions[i] || status != statuses[i] ||
        badge != badge_text[i] || badge2 != badge2_text[i])
      changed = true;

    captions[i] = std::move(caption);
    statuses[i] = std::move(status);
    badge_text[i] = std::move(badge);
    badge2_text[i] = std::move(badge2);
  }

  return changed;
}

void
DeviceListWidget::Build()
{
  Clear();
  AddGroup();

  for (unsigned i = 0; i < NUMDEV; ++i) {
    ItemOptions options;
    options.subtitle = statuses[i].c_str();
    if (!badge_text[i].empty()) {
      options.badge = badge_text[i].c_str();
      options.badge_style = BadgeStyle::DANGER;
    }
    if (!badge2_text[i].empty()) {
      options.badge2 = badge2_text[i].c_str();
      options.badge_style2 = BadgeStyle::DANGER;
    }
    AddItem(captions[i].c_str(), options);
  }

  UpdateLayout();
}

void
DeviceListWidget::Refresh()
{
  if (Load())
    Build();
  UpdateButtons();
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
  if (edit_button == nullptr)
    return;

  const int current = Current();

  if (current >= 0) {
    const auto &config =
      CommonInterface::GetSystemSettings().devices[current];

    if (config.port_type != DeviceConfig::PortType::DISABLED) {
      disable_button->SetEnabled(true);
      disable_button->SetCaption(config.enabled ? _("Disable") : _("Enable"));
    } else
      disable_button->SetEnabled(false);
  } else
    disable_button->SetEnabled(false);

  if (is_simulator() || current < 0 || devices == nullptr) {
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

  edit_button->SetEnabled(current >= 0);
}

void
DeviceListWidget::FormatRow(unsigned idx, std::string &line,
                            std::string &badge, std::string &badge2) const
{
  line.clear();
  badge.clear();
  badge2.clear();

  const DeviceConfig &config =
    CommonInterface::GetSystemSettings().devices[idx];
  const Flags flags(*items[idx]);

  if (!flags.alive) {
    if (config.IsDisabled())
      line = _("Disabled");
    else if (is_simulator() || !config.IsAvailable())
      line = _("N/A");
    else if (flags.bluetooth_disabled)
      line = _("Bluetooth is disabled");
    else if (flags.duplicate)
      line = _("Duplicate");
    else if (flags.connecting)
      line = _("Connecting...");
    else if (flags.open) {
      AppendFlag(line, _("No data"), false);
      if (flags.debug)
        AppendFlag(line, _("Debug"), true);
    } else if (flags.error) {
      line = error_messages[idx].empty()
        ? _("Error")
        : error_messages[idx];
    } else
      line = _("Not connected");
    return;
  }

  if (flags.location)
    AppendFlag(line, _("GPS fix"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.location;
               }));
  else if (flags.gps) {
    /* device sends GPGGA, but no valid location */
    const char *text = _("Bad GPS");
    AppendFlag(line, text, false);
    NoteWarning(badge, badge2, text);
  } else
    AppendFlag(line, _("Connected"), false);

  if (flags.baro)
    AppendFlag(line, _("Baro"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.baro;
               }));

  /* QNE and Pressure are one ambient-pressure source (#3268). */
  const auto earlier_ambient = [](const Flags &f) {
    return f.pressure_altitude || f.pressure;
  };

  if (flags.pressure_altitude)
    AppendFlag(line, _("QNE"),
               !EarlierHas(idx, earlier_ambient));

  if (flags.igc_altitude)
    AppendFlag(line, C_("Abbreviation", "Alt IGC"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.igc_altitude;
               }));

  if (flags.pressure)
    AppendFlag(line, _("Pressure"),
               !EarlierHas(idx, earlier_ambient));

  if (flags.pitot)
    AppendFlag(line, _("Pitot"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.pitot;
               }));

  if (flags.airspeed)
    AppendFlag(line, _("Airspeed"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.airspeed;
               }));

  if (flags.vario)
    AppendFlag(line, _("Vario"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.vario;
               }));

  if (flags.gdl90)
    AppendFlag(line, "GDL90", true);
  else if (flags.traffic)
    AppendFlag(line, "FLARM", true);

  if (flags.foreflight_ahrs)
    AppendFlag(line, "ForeFlight AHRS",
               !EarlierHas(idx, [](const Flags &f) {
                 return f.foreflight_ahrs;
               }));

  if (flags.foreflight_id)
    AppendFlag(line, "ForeFlight ID", true);

  if (flags.temperature)
    AppendFlag(line, _("Temperature"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.temperature;
               }));

  if (flags.humidity)
    AppendFlag(line, _("Relative humidity"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.humidity;
               }));

  if (flags.imu)
    AppendFlag(line, _("IMU"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.imu;
               }));

  if (flags.accel)
    AppendFlag(line, "G",
               !EarlierHas(idx, [](const Flags &f) {
                 return f.accel;
               }));

  if (flags.heart_rate)
    AppendFlag(line, _("Heart Rate"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.heart_rate;
               }));

  if (flags.blood_oxygen)
    AppendFlag(line, _("Blood Oxygen"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.blood_oxygen;
               }));

  if (flags.radio)
    AppendFlag(line, _("Radio"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.radio;
               }));

  if (flags.transponder)
    AppendFlag(line, "XPDR",
               !EarlierHas(idx, [](const Flags &f) {
                 return f.transponder;
               }));

  if (flags.engine)
    AppendFlag(line, _("Engine"),
               !EarlierHas(idx, [](const Flags &f) {
                 return f.engine;
               }));

  if (flags.debug)
    AppendFlag(line, _("Debug"), true);

  if (flags.battery_percent >= 0) {
    StaticString<32> battery;
    battery.Format("%s=%d%%", _("Battery"), flags.battery_percent);
    AppendFlag(line, battery, false);
    if (flags.battery_percent < BATTERY_WARNING_PERCENT)
      NoteWarning(badge, badge2, battery);
  }
}


inline void
DeviceListWidget::EnableDisableCurrent()
{
  const int index = Current();
  if (index < 0)
    return;

  DeviceConfig &config = CommonInterface::SetSystemSettings().devices[index];
  if (config.port_type == DeviceConfig::PortType::DISABLED)
    return;

  /* save new config to profile .. */

  config.enabled = !config.enabled;
  Profile::SetDeviceConfig(Profile::map, index, config);
  Profile::Save();

  /* update the UI and reopen the device */

  Refresh();

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

  const int current = Current();
  if (current < 0)
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

  const int current = Current();
  if (current < 0)
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
  const int current = Current();
  if (current < 0)
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

  /* update the UI and reopen the device */

  Refresh();

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

  const int current = Current();
  if (current < 0)
    return;

  DeviceDescriptor &descriptor = (*devices)[current];
  if (!descriptor.IsManageable())
    return;

#ifdef ANDROID
  const auto &config = descriptor.GetConfig();
  if (config.port_type == DeviceConfig::PortType::DROIDSOAR_V2 ||
      (config.port_type == DeviceConfig::PortType::I2CPRESSURESENSOR &&
       config.press_use == DeviceConfig::PressureUse::PITOT)) {
    ManageI2CPitotDialog(UIGlobals::GetMainWindow(),
                         UIGlobals::GetDialogLook(), descriptor);
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
    ManageCAI302Dialog(UIGlobals::GetMainWindow(),
                       UIGlobals::GetDialogLook(), *device);
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

  const int current = Current();
  if (current < 0)
    return;

  DeviceDescriptor &descriptor = (*devices)[current];
  ShowPortMonitor(descriptor);
}

inline void
DeviceListWidget::DebugCurrent()
{
  if (devices == nullptr)
    return;

  const int current = Current();
  if (current < 0)
    return;

  DeviceDescriptor &device = (*devices)[current];
  if (!device.GetConfig().UsesPort() || device.GetState() != PortState::READY)
    return;

  static constexpr unsigned MINUTES = 10;

  device.EnableDumpTemporarily(std::chrono::minutes(MINUTES));
  Refresh();

  StaticString<256> msg;
  msg.Format(_("Communication with this device will be logged for the next %u minutes."),
             MINUTES);
  ShowMessageBox(msg, _("Debug"), MB_OK | MB_ICONINFORMATION);
}

void
DeviceListWidget::OnGPSUpdate([[maybe_unused]] const MoreData &basic)
{
  Refresh();
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
  dialog.SetWidget(*device_blackboard, devices);
  dialog.GetWidget().CreateButtons(dialog);
  dialog.AddButton(_("Help"), [](){
    HelpDialog(_("Devices"), GetDeviceListHelp());
  });
  dialog.AddButton(_("Close"), mrOK);
  dialog.GetWidget().SetActionBar(dialog.GetButtonPanel());
  dialog.EnableCursorSelection();

  dialog.ShowModal();
}
