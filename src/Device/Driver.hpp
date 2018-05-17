/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_DRIVER_HPP
#define XCSOAR_DEVICE_DRIVER_HPP

#include <stddef.h>
#include <tchar.h>

struct NMEAInfo;
struct MoreData;
struct DerivedInfo;
struct DeviceConfig;
struct Declaration;
struct Waypoint;
class Path;
class Port;
class AtmosphericPressure;
class RadioFrequency;
class OperationEnvironment;
struct RecordedFlightInfo;
class RecordedFlightList;

/**
 * This is the interface for a device driver.
 */
class Device {
public:
  virtual ~Device();

  /**
   * Called after there has not been any NMEA input on the port for
   * some time, and XCSoar considers the device "disconnected".  The
   * next EnableNMEA() code should attempt to put the device back to
   * NMEA mode.
   */
  virtual void LinkTimeout() = 0;

  /**
   * Enable NMEA mode.  This method is called after opening the
   * device, after LinkTimeout() and after all other methods that
   * manipulate the device.  If a driver does not need to disable NMEA
   * mode for a method implementation, this should be a no-op.
   *
   * The caller is responsible for invoking Port::StartRxThread().
   */
  virtual bool EnableNMEA(OperationEnvironment &env) = 0;

  /**
   * Parse a line of input from the port.
   *
   * @param info destination for sensor values
   * @return true when the line has been processed
   */
  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info) = 0;

  /**
   * Send the new MacCready value to the device.
   *
   * @param mac_cready the new MacCready value [m/s]
   * @return true on success
   */
  virtual bool PutMacCready(double mac_cready, OperationEnvironment &env) = 0;

  /**
   * Send the new "bugs" value to the device (degradation of the
   * calculated polar).
   *
   * @param bugs the new bugs value (XXX define this)
   * @return true on success
   */
  virtual bool PutBugs(double bugs, OperationEnvironment &env) = 0;

  /**
   * Send the new ballast value to the device.
   *
   * @param fraction the new ballast value (0=no ballast, 1=full)
   * @param overload an alternative description of ballast value
   * @return true on success
   */
  virtual bool PutBallast(double fraction, double overload,
                          OperationEnvironment &env) = 0;

  /**
   * Send the new QNH value to the device.
   *
   * @param pressure the new QNH
   * @param calculated the current set of calculation results
   * @return true on success
   */
  virtual bool PutQNH(const AtmosphericPressure &pressure,
                      OperationEnvironment &env) = 0;

  /**
   * Set the radio volume.
   *
   * @param volume the new volume (0 - 100%)
   * @return true on success
   */
  virtual bool PutVolume(unsigned volume, OperationEnvironment &env) = 0;

  /**
   * Set a new radio frequency.
   *
   * @param frequency the new frequency
   * @param name name of the radio station
   * @return true on success
   */
  virtual bool PutActiveFrequency(RadioFrequency frequency,
                                  const TCHAR *name,
                                  OperationEnvironment &env) = 0;

  /**
   * Set a new "standby" radio frequency.
   *
   * @param frequency the new frequency
   * @param name name of the radio station
   * @return true on success
   */
  virtual bool PutStandbyFrequency(RadioFrequency frequency,
                                   const TCHAR *name,
                                   OperationEnvironment &env) = 0;

  /**
   * Enable pass-through mode.  This may be used to communicate
   * directly with the device that is "behind" this one (e.g. a LX1600
   * connected to a FLARM).
   */
  virtual bool EnablePassThrough(OperationEnvironment &env) = 0;

  /**
   * Declare a task.
   *
   * @param declaration the task declaration
   * @param home the home waypoint, or nullptr if not known/configured;
   * this is not part of the task declaration, but some drivers might
   * want to send it to the logger during the declaration process
   * @return true on success
   */
  virtual bool Declare(const Declaration &declaration, const Waypoint *home,
                       OperationEnvironment &env) = 0;

  /**
   * Read the list of recorded flights.
   *
   * @param flight_list the flights will be appended to this list
   * @return true on success
   */
  virtual bool ReadFlightList(RecordedFlightList &flight_list,
                              OperationEnvironment &env) = 0;

  /**
   * Download a flight into a file.
   *
   * @param flight the flight that shall be downloaded
   * @param path the file name to save to
   * @return true on success
   */
  virtual bool DownloadFlight(const RecordedFlightInfo &flight,
                              Path path,
                              OperationEnvironment &env) = 0;

  /**
   * Called periodically each second
   */
  virtual void OnSysTicker() = 0;

  /**
   * Called when data is received and the device is configured to
   * use the binary data directly.
   *
   * @return true when the data has been processed,
   *         false if more data is necessary
   */
  virtual bool DataReceived(const void *data, size_t length,
                            struct NMEAInfo &info) = 0;

  /**
   * This method is invoked by #MergeThread after each merge,
   * i.e. each time any device updates its #NMEAInfo object.  If there
   * are many devices or if there is a device with a high output rate,
   * this method may be called very often.  If no device is connected,
   * it may not be called at all.
   *
   * It is meant to be implemented by drivers that forward data
   * quickly to the device, for example an analog vario needle.
   *
   * Note that this method will be invoked on sensor changes on any
   * device, including this one.
   *
   * Caution!  This method will be called with the DeviceBlackboard
   * mutex locked.  Therefore it must not block and must be very
   * careful with obtaining more mutexes, as this may block the whole
   * XCSoar process.
   *
   * @param basic the merged sensor data
   */
  virtual void OnSensorUpdate(const MoreData &basic) = 0;

  /**
   * This method is invoked by the main thread after each
   * CalculationThread run.  It is meant for drivers which want to
   * send calculation results to the device.
   */
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated) = 0;
};

/**
 * This class implements all #Device methods.  You may use it as a
 * base class for specific device drivers.
 */
class AbstractDevice : public Device {
public:
  void LinkTimeout() override;
  bool EnableNMEA(OperationEnvironment &env) override;

  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;

  bool PutMacCready(double MacCready, OperationEnvironment &env) override;
  bool PutBugs(double bugs, OperationEnvironment &env) override;
  bool PutBallast(double fraction, double overload,
                  OperationEnvironment &env) override;
  bool PutQNH(const AtmosphericPressure &pres,
              OperationEnvironment &env) override;
  bool PutVolume(unsigned volume, OperationEnvironment &env) override;
  bool PutActiveFrequency(RadioFrequency frequency,
                          const TCHAR *name,
                          OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const TCHAR *name,
                           OperationEnvironment &env) override;

  bool EnablePassThrough(OperationEnvironment &env) override;

  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env) override;

  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env) override;

  bool DownloadFlight(const RecordedFlightInfo &flight,
                      Path path,
                      OperationEnvironment &env) override;

  void OnSysTicker() override;

  bool DataReceived(const void *data, size_t length,
                    struct NMEAInfo &info) override;

  void OnSensorUpdate(const MoreData &basic) override {}

  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override {}
};

/**
 * This is the structure exported by a device driver.
 */
struct DeviceRegister {
  enum {
    /**
     * Makes XCSoar forward all NMEA input to this device.  This is
     * only used by the "NmeaOut" driver.
     */
    NMEA_OUT = 0x1,

    /**
     * Does this driver support task declaration with
     * Device::Declare()?
     */
    DECLARE = 0x2,

    /**
     * Does this device store flight logs which can be downloaded?
     * See Device::ReadFlightList(), Device::DownloadFlight().
     */
    LOGGER = 0x4,

    /**
     * Does this driver support switching to a "bulk" baud rate?
     */
    BULK_BAUD_RATE = 0x8,

    /**
     * Does this driver support additional configuration in form
     * of a "Manage" dialog?
     */
    MANAGE = 0x10,

    /**
     * Shall timeout and auto-restart be disabled for this driver?
     * This flag should be set for devices that are not expected to
     * send data every second.
     */
    NO_TIMEOUT = 0x20,

    /**
     * Is this device sending GPS data in binary form? The line-based
     * handler/parser will be disabled in this case.
     */
    RAW_GPS_DATA = 0x40,

    /**
     * Is this driver able to receive settings like MC value,
     * bugs or ballast from the device?
     */
    RECEIVE_SETTINGS = 0x80,

    /**
     * Is this driver able to send settings like MC value,
     * bugs or ballast to the device?
     */
    SEND_SETTINGS = 0x100,

    /**
     * Is this driver capable of passing through communication to
     * another device behind it?  This indicates that
     * EnablePassThrough() is implemented.
     */
    PASS_THROUGH = 0x200,
  };

  /**
   * The internal name of the driver, i.e. the one that is stored in
   * the profile.
   */
  const TCHAR *name;

  /**
   * The human-readable name of this driver.
   */
  const TCHAR *display_name;

  /**
   * A bit set describing the features of this driver.
   */
  unsigned int flags;

  /**
   * Create an instance of this driver for the given NMEA port.
   */
  Device *(*CreateOnPort)(const DeviceConfig &config, Port &com_port);

  /**
   * Is this driver able to receive settings like MC value,
   * bugs or ballast from the device?
   */
  bool CanReceiveSettings() const {
    return (flags & RECEIVE_SETTINGS) != 0;
  }

  /**
   * Is this driver able to send settings like MC value,
   * bugs or ballast to the device?
   */
  bool CanSendSettings() const {
    return (flags & SEND_SETTINGS) != 0;
  }

  /**
   * Is this the NMEA out driver?
   */
  bool IsNMEAOut() const {
    return (flags & NMEA_OUT) != 0;
  }

  /**
   * Does this driver support task declaration with Device::Declare()?
   */
  bool CanDeclare() const {
    return (flags & DECLARE) != 0;
  }

  /**
   * Does this device store flight logs which can be downloaded?
   * See Device::ReadFlightList(), Device::DownloadFlight().
   */
  bool IsLogger() const {
    return (flags & LOGGER) != 0;
  }

  /**
   * Does this device support additional configuration in form
   * of a "Manage" dialog?
   */
  bool IsManageable() const {
    return (flags & MANAGE) != 0;
  }

  /**
   * Does this driver support switching to a "bulk" baud rate?
   */
  bool SupportsBulkBaudRate() const {
    return (flags & BULK_BAUD_RATE) != 0;
  }

  /**
   * Shall devices be restarted automatically when they time out?
   */
  bool HasTimeout() const {
    return (flags & NO_TIMEOUT) == 0;
  }

  /**
   * Is this device sending GPS data in binary form? The line-based
   * handler/parser will be disabled in this case.
   */
  bool UsesRawData() const {
    return (flags & RAW_GPS_DATA) != 0;
  }

  /**
   * Does this driver implement EnablePassThrough()?
   */
  bool HasPassThrough() const {
    return (flags & PASS_THROUGH) != 0;
  }
};

#endif
