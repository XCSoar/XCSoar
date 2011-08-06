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

#ifndef XCSOAR_DEVICE_DESCRIPTOR_HPP
#define XCSOAR_DEVICE_DESCRIPTOR_HPP

#include "Device/Port.hpp"
#include "Device/Parser.hpp"
#include "Profile/DeviceConfig.hpp"
#include "RadioFrequency.hpp"
#include "NMEA/ExternalSettings.hpp"
#include "PeriodClock.hpp"

#include <assert.h>
#include <tchar.h>
#include <stdio.h>

struct NMEAInfo;
struct DerivedInfo;
class Port;
class Device;
class AtmosphericPressure;
struct DeviceRegister;
class InternalGPS;
class RecordedFlightList;
struct RecordedFlightInfo;
class OperationEnvironment;

class DeviceDescriptor : public Port::Handler {
  /** the index of this device in the global list */
  unsigned index;

  DeviceConfig config;

  Port *Com;
  DeviceDescriptor *pDevPipeTo;
  const struct DeviceRegister *Driver;

  Device *device;

#ifdef ANDROID
  InternalGPS *internal_gps;
#endif

  /**
   * This clock keeps track when we need to reopen the device next
   * time after a failure or after a timeout.  It gets updated each
   * time the failure/timeout occurs, and again after each retry.
   */
  PeriodClock reopen_clock;

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

  bool was_connected;

  bool ticker;

  /**
   * True during task declaration or flight download.  Link timeouts
   * are disabled meanwhile.
   */
  bool busy;

public:
  DeviceDescriptor();
  ~DeviceDescriptor();

  unsigned GetIndex() const {
    return index;
  }

  void SetIndex(unsigned _index) {
    index = _index;
  }

  const DeviceConfig &GetConfig() const {
    return config;
  }

  DeviceConfig &SetConfig() {
    return config;
  }

  void SetPipeTo(DeviceDescriptor *other) {
    pDevPipeTo = other;
  }

  bool IsConfigured() const {
    return config.port_type != DeviceConfig::DISABLED;
  }

  bool IsOpen() const {
    return Com != NULL
#ifdef ANDROID
      || internal_gps != NULL;
#endif
    ;
  }

  /**
   * Returns the Device object; may be NULL if the device is not open
   * or if the Device class is not applicable for this object.
   *
   * Should only be used by driver-specific code (such as the CAI 302
   * manager).
   */
  Device *GetDevice() {
    return device;
  }

  /**
   * When this method fails, the caller is responsible for freeing the
   * Port object.
   */
  bool Open(Port *port, const struct DeviceRegister *driver,
            OperationEnvironment &env);

  bool OpenInternalGPS();

  bool Open(OperationEnvironment &env);

  void Close();

  bool Reopen(OperationEnvironment &env);

  /**
   * Call this periodically to auto-reopen a failed device after a
   * certain delay.
   */
  void AutoReopen(OperationEnvironment &env);

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

  /**
   * Is this device currently busy (i.e. not in normal NMEA receiving
   * mode)?  During that, we do not expect it to send GPS updates, and
   * the link timeouts are disabled.
   */
  bool IsBusy() const {
    return busy;
  }

  void SetBusy(bool _busy) {
    assert(_busy != busy);

    busy = _busy;
  }

  /**
   * Query the device's "connected" flag from the DeviceBlackboard.
   * This method locks the DeviceBlackboard.
   */
  gcc_pure
  bool IsConnected() const;

private:
  bool ParseNMEA(const char *line, struct NMEAInfo &info);

public:
  void WriteNMEA(const char *line);
#ifdef _UNICODE
  void WriteNMEA(const TCHAR *line);
#endif

  bool PutMacCready(fixed MacCready);
  bool PutBugs(fixed bugs);
  bool PutBallast(fixed ballast);
  bool PutVolume(int volume);
  bool PutActiveFrequency(RadioFrequency frequency);
  bool PutStandbyFrequency(RadioFrequency frequency);
  bool PutQNH(const AtmosphericPressure &pres);
  bool PutVoice(const TCHAR *sentence);

  bool Declare(const struct Declaration &declaration,
               OperationEnvironment &env);

  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env);
  bool DownloadFlight(const RecordedFlightInfo &flight, const TCHAR *path,
                      OperationEnvironment &env);

  void OnSysTicker(const DerivedInfo &calculated);

  bool ParseLine(const char *line);

  virtual void LineReceived(const char *line);
};

#endif
