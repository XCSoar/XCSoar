/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Device/Driver/Generic.hpp"
#include "Device/Driver/AltairPro.hpp"
#include "Device/Driver/CAI302.hpp"
#include "Device/Driver/Condor.hpp"
#include "Device/Driver/EW.hpp"
#include "Device/Driver/EWMicroRecorder.hpp"
#include "Device/Driver/FlymasterF1.hpp"
#include "Device/Driver/LX.hpp"
#include "Device/Driver/ILEC.hpp"
#include "Device/Driver/PosiGraph.hpp"
#include "Device/Driver/Vega.hpp"
#include "Device/Driver/Volkslogger.hpp"
#include "Device/Driver/Zander.hpp"
#include "Device/Driver.hpp"
#include "Device/Parser.hpp"
#include "Device/Geoid.h"
#include "Device/device.hpp"
#include "NMEA/Info.hpp"
#include "Protection.hpp"
#include "InputEvents.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "FaultInjectionPort.hpp"
#include "TestUtil.hpp"

#include <string.h>

static bool gps_updated, vario_updated;

FLYING_STATE::FLYING_STATE() {}

void TriggerGPSUpdate()
{
  gps_updated = true;
}

void TriggerVarioUpdate()
{
  vario_updated = true;
}

bool
HaveCondorDevice()
{
  return false;
}

bool
devHasBaroSource()
{
  return false;
}

/*
 * Fake Waypoints
 */

Waypoints way_points;

TaskProjection::TaskProjection() {}
Waypoints::Waypoints() {}

const Waypoint*
Waypoints::find_home() const
{
  return NULL;
}

/*
 * Fake Device/Geoid.cpp
 */

fixed
LookupGeoidSeparation(const GeoPoint pt)
{
  return fixed_zero;
}

/*
 * Fake InputEvents.cpp
 */

bool
InputEvents::processGlideComputer(unsigned gce_id)
{
  return true;
}

/*
 * Unit tests
 */

static void
TestGeneric()
{
  NMEAParser parser;

  NMEA_INFO nmea_info;
  memset(&nmea_info, 0, sizeof(nmea_info));

  /* no GPS reception */
  parser.ParseNMEAString_Internal("$GPRMC,082310,V,,,,,230610*3f",
                                  &nmea_info);
  ok1(nmea_info.gps.Connected == 2);
  ok1(nmea_info.gps.NAVWarning);
  ok1(nmea_info.DateTime.year == 2010);
  ok1(nmea_info.DateTime.month == 6);
  ok1(nmea_info.DateTime.day == 23);
  ok1(nmea_info.DateTime.hour == 8);
  ok1(nmea_info.DateTime.minute == 23);
  ok1(nmea_info.DateTime.second == 10);

  /* got a GPS fix */
  parser.ParseNMEAString_Internal("$GPRMC,082311,A,5103.5403,N,00741.5742,E,055.3,022.4,230610,000.3,W*6C",
                                  &nmea_info);
  ok1(nmea_info.gps.Connected == 2);
  ok1(!nmea_info.gps.NAVWarning);
  ok1(nmea_info.DateTime.hour == 8);
  ok1(nmea_info.DateTime.minute == 23);
  ok1(nmea_info.DateTime.second == 11);
  ok1(equals(nmea_info.Location.Longitude, 7.693));
  ok1(equals(nmea_info.Location.Latitude, 51.059));
  ok1(!nmea_info.BaroAltitudeAvailable);

  /* baro altitude (proprietary Garmin sentence) */
  parser.ParseNMEAString_Internal("$PGRMZ,100,m,3*11", &nmea_info);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 100));
}

static void
TestCAI302()
{
  NMEAParser parser;

  Device *device = cai302Device.CreateOnPort(NULL);
  ok1(device != NULL);

  NMEA_INFO nmea_info;
  memset(&nmea_info, 0, sizeof(nmea_info));
  nmea_info.pressure.set_QNH(fixed(1013.25));

  /* baro altitude disabled */
  device->ParseNMEA("!w,000,000,0000,500,01287,01020,-0668,191,199,191,000,000,100*44",
                    &nmea_info, false);
  ok1(!nmea_info.BaroAltitudeAvailable);
  ok1(nmea_info.AirspeedAvailable);
  ok1(nmea_info.TotalEnergyVarioAvailable);

  device->ParseNMEA("$PCAID,N,500,0,*14", &nmea_info, false);
  ok1(!nmea_info.BaroAltitudeAvailable);

  /* baro altitude enabled (PCAID) */
  device->ParseNMEA("$PCAID,N,500,0,*14", &nmea_info, true);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(between(nmea_info.BaroAltitude, 499, 501));

  /* baro altitude enabled */
  device->ParseNMEA("!w,000,000,0000,500,01287,01020,-0668,191,199,191,000,000,100*44",
                    &nmea_info, true);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 287));
  ok1(nmea_info.AirspeedAvailable);
  ok1(equals(nmea_info.TrueAirspeed, -6.68));
  ok1(nmea_info.TotalEnergyVarioAvailable);
  ok1(equals(nmea_info.TotalEnergyVario, -0.463));

  /* PCAID should not override !w */
  device->ParseNMEA("$PCAID,N,500,0,*14", &nmea_info, true);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 287));

  /* check if RMZ overrides CAI's baro altitude */
  parser.ParseNMEAString_Internal("$PGRMZ,100,m,3*11", &nmea_info);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 287));

  delete device;
}

static void
TestFlymasterF1()
{
  NMEAParser parser;

  Device *device = flymasterf1Device.CreateOnPort(NULL);
  ok1(device != NULL);

  NMEA_INFO nmea_info;
  memset(&nmea_info, 0, sizeof(nmea_info));
  nmea_info.pressure.set_QNH(fixed(1013.25));

  /* baro altitude disabled */
  device->ParseNMEA("$VARIO,999.98,-12,12.4,12.7,0,21.3,25.5*CS",
                    &nmea_info, false);
  ok1(!nmea_info.BaroAltitudeAvailable);
  ok1(!nmea_info.AirspeedAvailable);
  ok1(nmea_info.TotalEnergyVarioAvailable);
  ok1(equals(nmea_info.TotalEnergyVario, -1.2));
  ok1(!nmea_info.TemperatureAvailable);

  /* baro altitude enabled */
  device->ParseNMEA("$VARIO,999.98,-1.2,12.4,12.7,0,21.3,25.5*CS",
                    &nmea_info, true);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(between(nmea_info.BaroAltitude, 111.0, 111.1));

  delete device;
}

static void
TestLX(const struct DeviceRegister &driver)
{
  Device *device = driver.CreateOnPort(NULL);
  ok1(device != NULL);

  NMEA_INFO nmea_info;
  memset(&nmea_info, 0, sizeof(nmea_info));

  /* baro altitude disabled */
  device->ParseNMEA("$LXWP0,N,,1266.5,,,,,,,,248,23.1*55", &nmea_info, false);
  ok1(!nmea_info.BaroAltitudeAvailable);
  ok1(!nmea_info.AirspeedAvailable);
  ok1(!nmea_info.TotalEnergyVarioAvailable);

  /* baro altitude enabled */
  device->ParseNMEA("$LXWP0,N,,1266.5,,,,,,,,248,23.1*55", &nmea_info, true);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 1266.5));
  ok1(!nmea_info.AirspeedAvailable);
  ok1(!nmea_info.TotalEnergyVarioAvailable);

  /* airspeed and vario available */
  device->ParseNMEA("$LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1",
                    &nmea_info, true);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 1665.5));
  ok1(nmea_info.AirspeedAvailable);
  ok1(equals(nmea_info.TrueAirspeed, 222.3/3.6));
  ok1(nmea_info.TotalEnergyVarioAvailable);
  ok1(equals(nmea_info.TotalEnergyVario, 1.71));

  delete device;
}

static void
TestILEC()
{
  Device *device = ilec_device_driver.CreateOnPort(NULL);
  ok1(device != NULL);

  NMEA_INFO nmea_info;
  memset(&nmea_info, 0, sizeof(nmea_info));

  /* baro altitude disabled */
  device->ParseNMEA("$PILC,PDA1,1489,-3.21*7D", &nmea_info, false);
  ok1(!nmea_info.BaroAltitudeAvailable);
  ok1(!nmea_info.AirspeedAvailable);
  ok1(nmea_info.TotalEnergyVarioAvailable);
  ok1(equals(nmea_info.TotalEnergyVario, -3.21));
  ok1(!nmea_info.ExternalWindAvailable);

  /* baro altitude enabled */
  device->ParseNMEA("$PILC,PDA1,1489,-3.21,274,15,58*7D", &nmea_info, true);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 1489));
  ok1(nmea_info.TotalEnergyVarioAvailable);
  ok1(equals(nmea_info.TotalEnergyVario, -3.21));
  ok1(nmea_info.ExternalWindAvailable);
  ok1(equals(nmea_info.ExternalWind.norm, 15 / 3.6));
  ok1(equals(nmea_info.ExternalWind.bearing, 274));

  delete device;
}

static void
TestZander()
{
  NMEAParser parser;

  Device *device = zanderDevice.CreateOnPort(NULL);
  ok1(device != NULL);

  NMEA_INFO nmea_info;
  memset(&nmea_info, 0, sizeof(nmea_info));
  nmea_info.pressure.set_QNH(fixed(999));

  /* baro altitude disabled */
  device->ParseNMEA("$PZAN1,02476,123456*04", &nmea_info, false);
  ok1(!nmea_info.BaroAltitudeAvailable);

  /* baro altitude enabled */
  device->ParseNMEA("$PZAN1,02476,123456*04", &nmea_info, true);
  ok1(nmea_info.BaroAltitudeAvailable);
  ok1(equals(nmea_info.BaroAltitude, 2476));

  device->ParseNMEA("$PZAN2,123,9850*03", &nmea_info, true);
  ok1(nmea_info.AirspeedAvailable);
  ok1(equals(nmea_info.TrueAirspeed, fixed(34.1667)));
  ok1(nmea_info.TotalEnergyVarioAvailable);
  ok1(equals(nmea_info.TotalEnergyVario, fixed(-1.5)));

  delete device;
}

Declaration::Declaration(OrderedTask const*) {}

static void
TestDeclare(const struct DeviceRegister &driver)
{
  FaultInjectionPort port(*(Port::Handler *)NULL);
  Device *device = driver.CreateOnPort(&port);
  ok1(device != NULL);

  Declaration declaration(NULL);
  _tcscpy(declaration.PilotName, _T("Foo Bar"));
  _tcscpy(declaration.AircraftType, _T("Cirrus"));
  _tcscpy(declaration.AircraftRego, _T("D-3003"));
  const GeoPoint gp(Angle::degrees(fixed(7.7061111111111114)),
                    Angle::degrees(fixed(51.051944444444445)));
  Waypoint wp(gp);
  wp.Name = _T("Foo");
  wp.Altitude = fixed(123);
  declaration.append(wp);
  declaration.append(wp);
  declaration.append(wp);
  declaration.append(wp);

  for (unsigned i = 0; i < 1024; ++i) {
    inject_port_fault = i;
    bool success = device->Declare(&declaration);
    if (success || !port.running || port.timeout != 0 ||
        port.baud_rate != FaultInjectionPort::DEFAULT_BAUD_RATE)
      break;
  }

  ok1(port.running);
  ok1(port.timeout == 0);
  ok1(port.baud_rate == FaultInjectionPort::DEFAULT_BAUD_RATE);

  delete device;
}

int main(int argc, char **argv)
{
  plan_tests(108);

  TestGeneric();
  TestCAI302();
  TestFlymasterF1();
  TestLX(lxDevice);
  TestLX(condorDevice);
  TestILEC();
  TestZander();

  /* XXX the Triadis drivers have too many dependencies, not enabling
     for now */
  //TestDeclare(atrDevice);
  TestDeclare(cai302Device);
  TestDeclare(ewDevice);
  TestDeclare(ewMicroRecorderDevice);
  TestDeclare(pgDevice);
  //TestDeclare(vgaDevice);

  /* XXX Volkslogger doesn't do well with this test case */
  //TestDeclare(vlDevice);

  return exit_status();
}
