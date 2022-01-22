/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_DESCRIPTOR_HPP
#define XCSOAR_DEVICE_DESCRIPTOR_HPP

#include "Features.hpp"
#include "Config.hpp"
#include "Device/Util/LineSplitter.hpp"
#include "Port/State.hpp"
#include "Port/Listener.hpp"
#include "Device/Parser.hpp"
#include "RadioFrequency.hpp"
#include "NMEA/ExternalSettings.hpp"
#include "time/PeriodClock.hpp"
#include "Job/Async.hpp"
#include "ui/event/Notify.hpp"
#include "thread/Mutex.hxx"
#include "thread/Debug.hpp"
#include "time/FloatDuration.hxx"
#include "util/tstring.hpp"
#include "util/StaticFifoBuffer.hxx"

#ifdef HAVE_INTERNAL_GPS
#include "SensorListener.hpp"
#endif

#ifdef ANDROID
#include "Math/SelfTimingKalmanFilter1d.hpp"
#include "Math/WindowFilter.hpp"
#endif

#include <array>
#include <atomic>
#include <chrono>
#include <memory>

#include <cassert>
#include <tchar.h>
#include <stdio.h>

namespace Cares { class Channel; }
namespace Java { class GlobalCloseable; }
class EventLoop;
struct NMEAInfo;
struct MoreData;
struct DerivedInfo;
struct Declaration;
struct Waypoint;
class Path;
class Port;
class DumpPort;
class Device;
class AtmosphericPressure;
struct DeviceRegister;
class InternalSensors;
class RecordedFlightList;
struct RecordedFlightInfo;
class OperationEnvironment;
class OpenDeviceJob;
class DeviceDataEditor;

class DeviceDescriptor final
  : PortListener,
#ifdef HAVE_INTERNAL_GPS
    SensorListener,
#endif
    PortLineSplitter {
  /**
   * The #EventLoop instance used by #Port instances.
   */
  EventLoop &event_loop;

  /**
   * The asynchronous DNS resolver used by #Port instances.
   */
  Cares::Channel &cares;

  UI::Notify job_finished_notify{[this]{ OnJobFinished(); }};

  /**
   * This mutex protects modifications of the attribute "device".  If
   * you use the attribute "device" from a thread other than the main
   * thread, you must hold this mutex.
   */
  mutable Mutex mutex;

  /** the index of this device in the global list */
  const unsigned index;

  PortListener *const port_listener;

  /**
   * This device's configuration.  It may differ from the instance in
   * #SystemSettings, because overlapping devices might have been
   * cleared.
   */
  DeviceConfig config;

  /**
   * This object runs the DoOpen() method in background to make it
   * non-blocking.
   */
  AsyncJobRunner async;

  /**
   * The #Job that currently opens the device.  nullptr if the device is
   * not currently being opened.
   */
  OpenDeviceJob *open_job = nullptr;

  /**
   * The #Port used by this device.  This is not applicable to some
   * devices, and is nullptr in that case.
   */
  std::unique_ptr<DumpPort> port;

  /**
   * A handler that will receive all data, to display it on the
   * screen.  Can be set with SetMonitor().
   */
  DataHandler *monitor = nullptr;

  /**
   * A handler that will receive all NMEA lines, to dispatch it to
   * other devices.
   */
  PortLineHandler *dispatcher = nullptr;

  /**
   * The device driver used to handle data to/from the device.
   */
  const DeviceRegister *driver = nullptr;

  /**
   * An instance of the driver.
   *
   * Modifications (from the main thread) must be protected by the
   * attribute "mutex".  Read access and any use of this object
   * outside of the main thread must also be protected, unless the
   * device was borrowed with the method Borrow().  The latter,
   * however, is only possible from the main thread.
   */
  Device *device = nullptr;

  /**
   * The second device driver for a passed through device.
   */
  const DeviceRegister *second_driver = nullptr;

  /**
   * An instance of the passed through driver, if available.
   */
  Device *second_device = nullptr;

#ifdef HAVE_INTERNAL_GPS
  /**
   * A pointer to the Java object managing all Android sensors (GPS,
   * baro sensor and others).
   */
  InternalSensors *internal_sensors = nullptr;
#endif

#ifdef ANDROID
  Java::GlobalCloseable *java_sensor = nullptr;
  Java::GlobalCloseable *second_java_sensor = nullptr;

  /* We use a Kalman filter to smooth Android device pressure sensor
     noise.  The filter requires two parameters: the first is the
     variance of the distribution of second derivatives of pressure
     values that we expect to see in flight, and the second is the
     maximum time between pressure sensor updates in seconds before
     the filter gives up on smoothing and uses the raw value.
     The pressure acceleration variance used here is actually wider
     than the maximum likelihood variance observed in the data: it
     turns out that the distribution is more heavy-tailed than a
     normal distribution, probably because glider pilots usually
     experience fairly constant pressure change most of the time. */
  static constexpr double KF_VAR_ACCEL = 0.0075;
  static constexpr SelfTimingKalmanFilter1d::Duration KF_MAX_DT =
    std::chrono::minutes{1};

  static constexpr SelfTimingKalmanFilter1d::Duration KF_I2C_MAX_DT =
    std::chrono::seconds{5};
  static constexpr double KF_I2C_VAR_ACCEL = 0.3;
  static constexpr double KF_I2C_VAR_ACCEL_85 = KF_VAR_ACCEL;

  SelfTimingKalmanFilter1d kalman_filter{KF_MAX_DT, KF_VAR_ACCEL};

  double voltage_offset;
  double voltage_factor;
  std::array<WindowFilter<16>, 1> voltage_filter;
  WindowFilter<64> temperature_filter;

  /**
   * State for Nunchuk.
   */
  int joy_state_x, joy_state_y;
#endif

  /**
   * This clock keeps track when we need to reopen the device next
   * time after a failure or after a timeout.  It gets updated each
   * time the failure/timeout occurs, and again after each retry.
   */
  PeriodClock reopen_clock;

  /**
   * The generic NMEA parser for this device.  It may hold internal
   * state.
   */
  NMEAParser parser;

  /**
   * The settings that were sent to the device.  This is used to check
   * if the device is sending back the new configuration; then the
   * device isn't actually sending a new setting, it is merely
   * repeating the settings we sent it.  This should not make XCSoar
   * reconfigure itself.
   */
  ExternalSettings settings_sent;

  /**
   * The settings that were received from the device.  This temporary
   * buffer mirrors NMEA_INFO::settings; NMEA_INFO::settings may get
   * cleared with ExternalSettings::EliminateRedundant(), so this one
   * always preserves the original values from the device, without
   * having to do a full NMEA_INFO copy.
   */
  ExternalSettings settings_received;

  /**
   * If this device has failed, then this attribute may contain an
   * error message.
   */
  tstring error_message;

  /**
   * Number of port failures since the device was last reset.
   *
   * @param see ResetFailureCounter()
   */
  unsigned n_failures = 0;

  /**
   * True when a sensor has failed and the device should be closed in
   * the next OnSysTicker() call.
   */
  std::atomic_bool has_failed{false};

  /**
   * Internal flag for OnSysTicker() for detecting link timeout.
   */
  bool was_alive;

  /**
   * Internal flag for OnSysTicker() for calling Device::OnSysTicker()
   * only every other time.
   */
  bool ticker = false;

  /**
   * True when somebody has "borrowed" the device.  Link timeouts are
   * disabled meanwhile.
   *
   * This attribute is only accessed from the main thread.
   *
   * @see CanBorrow(), Borrow()
   */
  bool borrowed = false;

public:
  DeviceDescriptor(EventLoop &_event_loop, Cares::Channel &_cares,
                   unsigned index, PortListener *port_listener) noexcept;
  ~DeviceDescriptor() noexcept;

  unsigned GetIndex() const noexcept {
    return index;
  }

  const DeviceConfig &GetConfig() const noexcept {
    return config;
  }

  void SetConfig(const DeviceConfig &config) noexcept;
  void ClearConfig() noexcept;

  bool IsConfigured() const {
    return config.port_type != DeviceConfig::PortType::DISABLED;
  }

  gcc_pure
  PortState GetState() const noexcept;

  tstring GetErrorMessage() const noexcept {
    const std::lock_guard<Mutex> lock(mutex);
    return error_message;
  }

  /**
   * Was there a failure on the #Port object?
   */
  bool HasPortFailed() const noexcept {
    return config.IsAvailable() && config.UsesPort() && port == nullptr;
  }

  /**
   * @see DumpPort::IsEnabled()
   */
  gcc_pure
  bool IsDumpEnabled() const noexcept;

  /**
   * @see DumpPort::Disable()
   */
  void DisableDump() noexcept;

  /**
   * @see DumpPort::EnableTemporarily()
   */
  void EnableDumpTemporarily(std::chrono::steady_clock::duration duration) noexcept;

  /**
   * Wrapper for Driver::HasTimeout().  This method can't be inline
   * because the Driver struct is incomplete at this point.
   */
  bool ShouldReopenDriverOnTimeout() const noexcept;

  /**
   * Should the #Port be reopened automatically when a timeout occurs?
   */
  bool ShouldReopenOnTimeout() const noexcept {
    return config.ShouldReopenOnTimeout() &&
      ShouldReopenDriverOnTimeout();
  }

  /**
   * Should the #Port be reopened?
   */
  bool ShouldReopen() const noexcept {
    return HasPortFailed() || (!IsAlive() && ShouldReopenOnTimeout());
  }

  /**
   * Returns the Device object; may be nullptr if the device is not open
   * or if the Device class is not applicable for this object.
   *
   * Should only be used by driver-specific code (such as the CAI 302
   * manager).
   */
  Device *GetDevice() noexcept {
    return device;
  }

private:
  /**
   * Cancel the #AsyncJobRunner object if it is running.
   */
  void CancelAsync() noexcept;

  /**
   * When this method fails, the caller is responsible for freeing the
   * Port object.
   *
   * Throws on error.
   */
  gcc_nonnull_all
  bool OpenOnPort(std::unique_ptr<DumpPort> &&port, OperationEnvironment &env);

  bool OpenInternalSensors();

  bool OpenDroidSoarV2();

  bool OpenI2Cbaro();

  bool OpenNunchuck();

  bool OpenVoltage();

  bool OpenGliderLink();

  bool OpenBluetoothSensor();

public:
  /**
   * To be used by OpenDeviceJob, don't call directly.
   */
  bool DoOpen(OperationEnvironment &env) noexcept;

  void ResetFailureCounter() noexcept {
    n_failures = 0u;
  }

  /**
   * @param env a persistent object
   */
  void Open(OperationEnvironment &env);

  void Close() noexcept;

  /**
   * @param env a persistent object
   */
  void Reopen(OperationEnvironment &env);

  /**
   * Call this periodically to auto-reopen a failed device after a
   * certain delay.
   *
   * @param env a persistent object
   */
  void AutoReopen(OperationEnvironment &env);

  /**
   * Call this method after Declare(), ReadFlightList(),
   * DownloadFlight() when you're done, to switch back to NMEA mode.
   *
   * Even when the driver's EnableNMEA() method fails, this method
   * will re-enable the receive thread, to avoid false negatives due
   * to flaky cables.
   */
  bool EnableNMEA(OperationEnvironment &env) noexcept;

  const TCHAR *GetDisplayName() const noexcept;

  /**
   * Compares the driver's name.
   */
  bool IsDriver(const TCHAR *name) const noexcept;

  gcc_pure
  bool CanDeclare() const noexcept;

  gcc_pure
  bool IsLogger() const noexcept;

  bool IsCondor() const noexcept {
    return IsDriver(_T("Condor"));
  }

  bool IsVega() const noexcept {
    return IsDriver(_T("Vega"));
  }

  bool IsNMEAOut() const noexcept;
  bool IsManageable() const noexcept;

  bool IsBorrowed() const noexcept {
    return borrowed;
  }

  /**
   * Is this device currently occupied, i.e. does somebody have
   * exclusive access?
   *
   * May only be called from the main thread.
   */
  bool IsOccupied() const noexcept {
    assert(InMainThread());

    return IsBorrowed() || async.IsBusy();
  }

  /**
   * Can this device be borrowed?
   *
   * May only be called from the main thread.
   *
   * @see Borrow()
   */
  bool CanBorrow() const noexcept {
    assert(InMainThread());

    return device != nullptr && GetState() == PortState::READY &&
      !IsOccupied();
  }

  /**
   * "Borrow" the device.  The caller gets exclusive access, e.g. to
   * submit a task declaration.  Call Return() when you are done.
   *
   * May only be called from the main thread.
   *
   * @return false if the device is already occupied and cannot be
   * borrowed
   */
  bool Borrow() noexcept;

  /**
   * Return a borrowed device.  The caller is responsible for
   * switching the device back to NMEA mode, see EnableNMEA().
   *
   * May only be called from the main thread.
   */
  void Return() noexcept;

  /**
   * Query the device's "alive" flag from the DeviceBlackboard.
   * This method locks the DeviceBlackboard.
   */
  gcc_pure
  bool IsAlive() const noexcept;

  [[gnu::pure]]
  TimeStamp GetClock() const noexcept;

  /**
   * Return a copy of the device's current data.
   */
  [[gnu::pure]]
  NMEAInfo GetData() const noexcept;

  DeviceDataEditor BeginEdit() noexcept;

private:
  bool ParseNMEA(const char *line, struct NMEAInfo &info) noexcept;

public:
  void SetMonitor(DataHandler  *_monitor) noexcept {
    monitor = _monitor;
  }

  void SetDispatcher(PortLineHandler *_dispatcher) noexcept {
    dispatcher = _dispatcher;
  }

  /**
   * Write a line to the device's port if it's a NMEA out port.
   */
  void ForwardLine(const char *line);

  bool WriteNMEA(const char *line, OperationEnvironment &env) noexcept;
#ifdef _UNICODE
  bool WriteNMEA(const TCHAR *line, OperationEnvironment &env) noexcept;
#endif

  bool PutMacCready(double mac_cready, OperationEnvironment &env) noexcept;
  bool PutBugs(double bugs, OperationEnvironment &env) noexcept;
  bool PutBallast(double fraction, double overload,
                  OperationEnvironment &env) noexcept;
  bool PutVolume(unsigned volume, OperationEnvironment &env) noexcept;
  bool PutPilotEvent(OperationEnvironment &env) noexcept;
  bool PutActiveFrequency(RadioFrequency frequency,
                          const TCHAR *name,
                          OperationEnvironment &env) noexcept;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const TCHAR *name,
                           OperationEnvironment &env) noexcept;
  bool PutQNH(AtmosphericPressure pres,
              OperationEnvironment &env) noexcept;

  /**
   * Caller is responsible for calling Borrow() and Return().
   */
  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env);

  /**
   * Caller is responsible for calling Borrow() and Return().
   */
  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env);

  /**
   * Caller is responsible for calling Borrow() and Return().
   */
  bool DownloadFlight(const RecordedFlightInfo &flight, Path path,
                      OperationEnvironment &env);

  void OnSysTicker() noexcept;

  /**
   * Wrapper for Driver::OnSensorUpdate().
   */
  void OnSensorUpdate(const MoreData &basic) noexcept;

  /**
   * Wrapper for Driver::OnCalculatedUpdate().
   */
  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) noexcept;

private:
  void OnJobFinished() noexcept;

  /* virtual methods from class PortListener */
  void PortStateChanged() noexcept override;
  void PortError(const char *msg) noexcept override;

  /* virtual methods from DataHandler  */
  bool DataReceived(std::span<const std::byte> s) noexcept override;

  /* virtual methods from PortLineHandler */
  bool LineReceived(const char *line) noexcept override;

#ifdef HAVE_INTERNAL_GPS
  /* methods from SensorListener */
  void OnConnected(int connected) noexcept override;
  void OnLocationSensor(std::chrono::system_clock::time_point time,
                        int n_satellites,
                        GeoPoint location,
                        bool hasAltitude, bool geoid_altitude,
                        double altitude,
                        bool hasBearing, double bearing,
                        bool hasSpeed, double speed,
                        bool hasAccuracy, double accuracy) noexcept override;

#ifdef ANDROID
  void OnAccelerationSensor(double acceleration) noexcept override;
  void OnAccelerationSensor(float ddx, float ddy,
                            float ddz) noexcept override;
  void OnRotationSensor(float dtheta_x, float dtheta_y,
                        float dtheta_z) noexcept override;
  void OnMagneticFieldSensor(float h_x, float h_y, float h_z) noexcept override;
  void OnBarometricPressureSensor(float pressure,
                                  float sensor_noise_variance) noexcept override;
  void OnPressureAltitudeSensor(float altitude) noexcept override;
  void OnI2CbaroSensor(int index, int sensorType,
                       AtmosphericPressure pressure) noexcept override;
  void OnVarioSensor(float vario) noexcept override;
  void OnHeartRateSensor(unsigned bpm) noexcept override;
  void OnVoltageValues(int temp_adc, unsigned voltage_index,
                       int volt_adc) noexcept override;
  void OnNunchukValues(int joy_x, int joy_y,
                       int acc_x, int acc_y, int acc_z,
                       int switches) noexcept final;
  void OnGliderLinkTraffic(GliderLinkId id, const char *callsign,
                           GeoPoint location, double altitude,
                           double gspeed, double vspeed,
                           unsigned bearing) noexcept override;
  void OnTemperature(Temperature temperature) noexcept override;
  void OnBatteryPercent(double battery_percent) noexcept override;
  void OnSensorStateChanged() noexcept override;
  void OnSensorError(const char *msg) noexcept override;
#endif // ANDROID
#endif // HAVE_INTERNAL_GPS
};

/**
 * This scope class calls DeviceDescriptor::Return() and
 * DeviceDescriptor::EnableNMEA() when the caller leaves the current
 * scope.  The caller must have called DeviceDescriptor::Borrow()
 * successfully before constructing this class.
 */
class ScopeReturnDevice {
  DeviceDescriptor &device;
  OperationEnvironment &env;

public:
  ScopeReturnDevice(DeviceDescriptor &_device,
                    OperationEnvironment &_env) noexcept
    :device(_device), env(_env) {
  }

  ~ScopeReturnDevice() noexcept {
    device.EnableNMEA(env);
    device.Return();
  }
};

#endif
