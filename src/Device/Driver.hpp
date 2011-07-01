/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Device/Declaration.hpp"
#include "RadioFrequency.hpp"
#include "DateTime.hpp"
#include "Util/StaticArray.hpp"

#include <tchar.h>
#include <stdint.h>

struct NMEA_INFO;
struct DERIVED_INFO;
class Port;
class AtmosphericPressure;
class OperationEnvironment;

struct RecordedFlightInfo {
  BrokenDate date;

  BrokenTime start_time, end_time;

  /**
   * Optional driver specific data to address a flight.
   */
  union {
    /**
     * Flight number, used by the CAI302 driver.
     */
    uint8_t cai302;
  } internal;
};

class RecordedFlightList : public StaticArray<RecordedFlightInfo, 128u> {
};

/**
 * This is the interface for a device driver.
 */
class Device {
public:
  virtual ~Device();

  /**
   * Called right after opening the port.  May be used to initialise
   * the connected device, e.g. to switch to NMEA mode.
   */
  virtual bool Open(OperationEnvironment &env) = 0;

  /**
   * Called after there has not been any NMEA input on the port for
   * some time, and XCSoar considers the device "disconnected".  May
   * be used to reinitialise NMEA mode.
   */
  virtual void LinkTimeout() = 0;

  /**
   * Parse a line of input from the port.
   *
   * @param info destination for sensor values
   * @return true when the line has been processed
   */
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO &info) = 0;

  /**
   * Send the new MacCready value to the device.
   *
   * @param mac_cready the new MacCready value [m/s]
   * @return true on success
   */
  virtual bool PutMacCready(fixed mac_cready) = 0;

  /**
   * Send the new "bugs" value to the device (degradation of the
   * calculated polar).
   *
   * @param bugs the new bugs value (XXX define this)
   * @return true on success
   */
  virtual bool PutBugs(fixed bugs) = 0;

  /**
   * Send the new ballast value to the device.
   *
   * @param ballast the new ballast value (XXX define this)
   * @return true on success
   */
  virtual bool PutBallast(fixed ballast) = 0;

  /**
   * Send the new QNH value to the device.
   *
   * @param pressure the new QNH
   * @param calculated the current set of calculation results
   * @return true on success
   */
  virtual bool PutQNH(const AtmosphericPressure &pressure,
                      const DERIVED_INFO &calculated) = 0;

  /**
   * Send a "voice" sentence to the device (proprietary Vega feature).
   *
   * @param sentence the sentence (XXX define this)
   * @return true on success
   */
  virtual bool PutVoice(const TCHAR *sentence) = 0;

  /**
   * Set the radio volume.
   *
   * @param volume the new volume (XXX define this)
   * @return true on success
   */
  virtual bool PutVolume(int volume) = 0;

  /**
   * Set a new radio frequency.
   *
   * @param frequency the new frequency
   * @return true on success
   */
  virtual bool PutActiveFrequency(RadioFrequency frequency) = 0;

  /**
   * Set a new "standby" radio frequency.
   *
   * @param frequency the new frequency
   * @return true on success
   */
  virtual bool PutStandbyFrequency(RadioFrequency frequency) = 0;

  /**
   * Declare a task.
   *
   * @param declaration the task declaration
   * @return true on success
   */
  virtual bool Declare(const struct Declaration &declaration,
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
                              const TCHAR *path,
                              OperationEnvironment &env) = 0;

  /**
   * Called periodically each second
   *
   * @param info the combined sensor values of all devices
   * @param calculated the current set of calculation results
   */
  virtual void OnSysTicker(const NMEA_INFO &basic,
                           const DERIVED_INFO &calculated) = 0;
};

/**
 * This class implements all #Device methods.  You may use it as a
 * base class for specific device drivers.
 */
class AbstractDevice : public Device {
public:
  virtual bool Open(OperationEnvironment &env);

  virtual void LinkTimeout();

  virtual bool ParseNMEA(const char *line, struct NMEA_INFO &info);

  virtual bool PutMacCready(fixed MacCready);
  virtual bool PutBugs(fixed bugs);
  virtual bool PutBallast(fixed ballast);
  virtual bool PutQNH(const AtmosphericPressure &pres,
                      const DERIVED_INFO &calculated);
  virtual bool PutVoice(const TCHAR *sentence);
  virtual bool PutVolume(int volume);
  virtual bool PutActiveFrequency(RadioFrequency frequency);
  virtual bool PutStandbyFrequency(RadioFrequency frequency);

  virtual bool Declare(const struct Declaration &declaration,
                       OperationEnvironment &env);

  virtual bool ReadFlightList(RecordedFlightList &flight_list,
                              OperationEnvironment &env) {
    return false;
  }

  virtual bool DownloadFlight(const RecordedFlightInfo &flight,
                              const TCHAR *path,
                              OperationEnvironment &env) {
    return false;
  }

  virtual void OnSysTicker(const NMEA_INFO &basic,
                           const DERIVED_INFO &calculated);
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
  unsigned int Flags;

  /**
   * Create an instance of this driver for the given NMEA port.
   */
  Device *(*CreateOnPort)(Port *com_port);

  /**
   * Is this the NMEA out driver?
   */
  bool IsNMEAOut() const {
    return (Flags & NMEA_OUT) != 0;
  }

  /**
   * Does this driver support task declaration with Device::Declare()?
   */
  bool CanDeclare() const {
    return (Flags & DECLARE) != 0;
  }

  /**
   * Does this device store flight logs which can be downloaded?
   * See Device::ReadFlightList(), Device::DownloadFlight().
   */
  bool IsLogger() const {
    return (Flags & LOGGER) != 0;
  }
};

#endif
