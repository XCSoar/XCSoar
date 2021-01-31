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
#include "util/tstring.hpp"
#include "util/StaticFifoBuffer.hxx"
#include "Android/GliderLink.hpp"

#include <chrono>
#include <memory>

#include <cassert>
#include <tchar.h>
#include <stdio.h>

namespace Cares { class Channel; }
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
class BMP085Device;
class I2CbaroDevice;
class NunchuckDevice;
class VoltageDevice;
class RecordedFlightList;
struct RecordedFlightInfo;
class OperationEnvironment;
class OpenDeviceJob;

class DeviceDescriptor final : PortListener, PortLineSplitter {
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
  OpenDeviceJob *open_job;

  /**
   * The #Port used by this device.  This is not applicable to some
   * devices, and is nullptr in that case.
   */
  std::unique_ptr<DumpPort> port;

  /**
   * A handler that will receive all data, to display it on the
   * screen.  Can be set with SetMonitor().
   */
  DataHandler  *monitor;

  /**
   * A handler that will receive all NMEA lines, to dispatch it to
   * other devices.
   */
  PortLineHandler *dispatcher;

  /**
   * The device driver used to handle data to/from the device.
   */
  const DeviceRegister *driver;

  /**
   * An instance of the driver.
   *
   * Modifications (from the main thread) must be protected by the
   * attribute "mutex".  Read access and any use of this object
   * outside of the main thread must also be protected, unless the
   * device was borrowed with the method Borrow().  The latter,
   * however, is only possible from the main thread.
   */
  Device *device;

  /**
   * The second device driver for a passed through device.
   */
  const DeviceRegister *second_driver;

  /**
   * An instance of the passed through driver, if available.
   */
  Device *second_device;


#ifdef HAVE_INTERNAL_GPS
  /**
   * A pointer to the Java object managing all Android sensors (GPS,
   * baro sensor and others).
   */
  InternalSensors *internal_sensors;
#endif

#ifdef ANDROID
  BMP085Device *droidsoar_v2;
  I2CbaroDevice *i2cbaro[3]; // static, pitot, tek; in any order
  NunchuckDevice *nunchuck;
  VoltageDevice *voltage;
  GliderLink *glider_link;
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
  unsigned n_failures;

  /**
   * Internal flag for OnSysTicker() for detecting link timeout.
   */
  bool was_alive;

  /**
   * Internal flag for OnSysTicker() for calling Device::OnSysTicker()
   * only every other time.
   */
  bool ticker;

  /**
   * True when somebody has "borrowed" the device.  Link timeouts are
   * disabled meanwhile.
   *
   * This attribute is only accessed from the main thread.
   *
   * @see CanBorrow(), Borrow()
   */
  bool borrowed;

public:
  DeviceDescriptor(EventLoop &_event_loop, Cares::Channel &_cares,
                   unsigned index, PortListener *port_listener);
  ~DeviceDescriptor() noexcept;

  unsigned GetIndex() const {
    return index;
  }

  const DeviceConfig &GetConfig() const {
    return config;
  }

  void SetConfig(const DeviceConfig &config);
  void ClearConfig();

  bool IsConfigured() const {
    return config.port_type != DeviceConfig::PortType::DISABLED;
  }

  gcc_pure
  PortState GetState() const;

  tstring GetErrorMessage() const {
    const std::lock_guard<Mutex> lock(mutex);
    return error_message;
  }

  /**
   * Was there a failure on the #Port object?
   */
  bool HasPortFailed() const {
    return config.IsAvailable() && config.UsesPort() && port == nullptr;
  }

  /**
   * @see DumpPort::IsEnabled()
   */
  gcc_pure
  bool IsDumpEnabled() const;

  /**
   * @see DumpPort::Disable()
   */
  void DisableDump();

  /**
   * @see DumpPort::EnableTemporarily()
   */
  void EnableDumpTemporarily(std::chrono::steady_clock::duration duration) noexcept;

  /**
   * Wrapper for Driver::HasTimeout().  This method can't be inline
   * because the Driver struct is incomplete at this point.
   */
  bool ShouldReopenDriverOnTimeout() const;

  /**
   * Should the #Port be reopened automatically when a timeout occurs?
   */
  bool ShouldReopenOnTimeout() const {
    return config.ShouldReopenOnTimeout() &&
      ShouldReopenDriverOnTimeout();
  }

  /**
   * Should the #Port be reopened?
   */
  bool ShouldReopen() const {
    return HasPortFailed() || (!IsAlive() && ShouldReopenOnTimeout());
  }

  /**
   * Returns the Device object; may be nullptr if the device is not open
   * or if the Device class is not applicable for this object.
   *
   * Should only be used by driver-specific code (such as the CAI 302
   * manager).
   */
  Device *GetDevice() {
    return device;
  }

private:
  /**
   * Cancel the #AsyncJobRunner object if it is running.
   */
  void CancelAsync();

  /**
   * When this method fails, the caller is responsible for freeing the
   * Port object.
   */
  gcc_nonnull_all
  bool OpenOnPort(std::unique_ptr<DumpPort> &&port, OperationEnvironment &env);

  bool OpenInternalSensors();

  bool OpenDroidSoarV2();

  bool OpenI2Cbaro();

  bool OpenNunchuck();

  bool OpenVoltage();

  bool OpenGliderLink();
public:
  /**
   * To be used by OpenDeviceJob, don't call directly.
   */
  bool DoOpen(OperationEnvironment &env) noexcept;

  void ResetFailureCounter() {
    n_failures = 0u;
  }

  /**
   * @param env a persistent object
   */
  void Open(OperationEnvironment &env);

  void Close();

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
  bool EnableNMEA(OperationEnvironment &env);

  const TCHAR *GetDisplayName() const;

  /**
   * Compares the driver's name.
   */
  bool IsDriver(const TCHAR *name) const;

  gcc_pure
  bool CanDeclare() const;

  gcc_pure
  bool IsLogger() const;

  bool IsCondor() const {
    return IsDriver(_T("Condor"));
  }

  bool IsVega() const {
    return IsDriver(_T("Vega"));
  }

  bool IsNMEAOut() const;
  bool IsManageable() const;

  bool IsBorrowed() const {
    return borrowed;
  }

  /**
   * Is this device currently occupied, i.e. does somebody have
   * exclusive access?
   *
   * May only be called from the main thread.
   */
  bool IsOccupied() const {
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
  bool CanBorrow() const {
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
  bool Borrow();

  /**
   * Return a borrowed device.  The caller is responsible for
   * switching the device back to NMEA mode, see EnableNMEA().
   *
   * May only be called from the main thread.
   */
  void Return();

  /**
   * Query the device's "alive" flag from the DeviceBlackboard.
   * This method locks the DeviceBlackboard.
   */
  gcc_pure
  bool IsAlive() const;

private:
  bool ParseNMEA(const char *line, struct NMEAInfo &info);

public:
  void SetMonitor(DataHandler  *_monitor) {
    monitor = _monitor;
  }

  void SetDispatcher(PortLineHandler *_dispatcher) {
    dispatcher = _dispatcher;
  }

  /**
   * Write a line to the device's port if it's a NMEA out port.
   */
  void ForwardLine(const char *line);

  bool WriteNMEA(const char *line, OperationEnvironment &env);
#ifdef _UNICODE
  bool WriteNMEA(const TCHAR *line, OperationEnvironment &env);
#endif

  bool PutMacCready(double mac_cready, OperationEnvironment &env);
  bool PutBugs(double bugs, OperationEnvironment &env);
  bool PutBallast(double fraction, double overload,
                  OperationEnvironment &env);
  bool PutVolume(unsigned volume, OperationEnvironment &env);
  bool PutActiveFrequency(RadioFrequency frequency,
                          const TCHAR *name,
                          OperationEnvironment &env);
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const TCHAR *name,
                           OperationEnvironment &env);
  bool PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env);

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

  void OnSysTicker();

  /**
   * Wrapper for Driver::OnSensorUpdate().
   */
  void OnSensorUpdate(const MoreData &basic);

  /**
   * Wrapper for Driver::OnCalculatedUpdate().
   */
  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated);

private:
  bool ParseLine(const char *line);

  void OnJobFinished() noexcept;

  /* virtual methods from class PortListener */
  void PortStateChanged() noexcept override;
  void PortError(const char *msg) noexcept override;

  /* virtual methods from DataHandler  */
  bool DataReceived(const void *data, size_t length) noexcept override;

  /* virtual methods from PortLineHandler */
  bool LineReceived(const char *line) noexcept override;
};

#endif
