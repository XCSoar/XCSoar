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
#include "Device/Driver/BorgeltB50.hpp"
#include "Device/Driver/CAI302.hpp"
#include "Device/Driver/Condor.hpp"
#include "Device/Driver/EW.hpp"
#include "Device/Driver/EWMicroRecorder.hpp"
#include "Device/Driver/FlymasterF1.hpp"
#include "Device/Driver/Flytec.hpp"
#include "Device/Driver/Leonardo.hpp"
#include "Device/Driver/LX.hpp"
#include "Device/Driver/ILEC.hpp"
#include "Device/Driver/IMI.hpp"
#include "Device/Driver/PosiGraph.hpp"
#include "Device/Driver/Vega.hpp"
#include "Device/Driver/Volkslogger.hpp"
#include "Device/Driver/Westerboer.hpp"
#include "Device/Driver/Zander.hpp"
#include "Device/Driver.hpp"
#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "NMEA/Info.hpp"
#include "Protection.hpp"
#include "InputEvents.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Operation.hpp"
#include "FaultInjectionPort.hpp"
#include "TestUtil.hpp"
#include "Profile/DeviceConfig.hpp"

static const DeviceConfig dummy_config = DeviceConfig();

bool
InputEvents::processNmea(unsigned ne_id)
{
  return false;
}

/*
 * Fake Waypoints
 */

Waypoints way_points;

Waypoints::Waypoints() {}

/*
 * Unit tests
 */

static void
TestGeneric()
{
  NMEAParser parser;

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  nmea_info.connected.Update(nmea_info.clock);

  /* no GPS reception */
  ok1(parser.ParseNMEAString_Internal("$GPRMC,082310,V,,,,,230610*3f",
                                      nmea_info));
  ok1(nmea_info.connected);
  ok1(!nmea_info.location_available);
  ok1(nmea_info.date_time_utc.year == 2010);
  ok1(nmea_info.date_time_utc.month == 6);
  ok1(nmea_info.date_time_utc.day == 23);
  ok1(nmea_info.date_time_utc.hour == 8);
  ok1(nmea_info.date_time_utc.minute == 23);
  ok1(nmea_info.date_time_utc.second == 10);

  /* got a GPS fix */
  ok1(parser.ParseNMEAString_Internal("$GPRMC,082311,A,5103.5403,N,00741.5742,E,055.3,022.4,230610,000.3,W*6C",
                                      nmea_info));
  ok1(nmea_info.connected);
  ok1(nmea_info.location_available);
  ok1(nmea_info.date_time_utc.hour == 8);
  ok1(nmea_info.date_time_utc.minute == 23);
  ok1(nmea_info.date_time_utc.second == 11);
  ok1(equals(nmea_info.location.Longitude, 7.693));
  ok1(equals(nmea_info.location.Latitude, 51.059));
  ok1(!nmea_info.baro_altitude_available);

  /* baro altitude (proprietary Garmin sentence) */
  ok1(parser.ParseNMEAString_Internal("$PGRMZ,100,m,3*11", nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 100));
}

static void
TestFLARM()
{
  NMEAParser parser;

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(parser.ParseNMEAString_Internal("$PFLAU,3,1,1,1,0*50",
                                      nmea_info));
  ok1(nmea_info.flarm.rx == 3);
  ok1(nmea_info.flarm.tx);
  ok1(nmea_info.flarm.gps == 1);
  ok1(nmea_info.flarm.alarm_level == 0);
  ok1(nmea_info.flarm.GetActiveTrafficCount() == 0);
  ok1(!nmea_info.flarm.NewTraffic);

  ok1(parser.ParseNMEAString_Internal("$PFLAA,0,100,-150,10,2,DDA85C,123,13,24,1.4,2*7f",
                                      nmea_info));
  ok1(nmea_info.flarm.NewTraffic);
  ok1(nmea_info.flarm.GetActiveTrafficCount() == 1);

  FlarmId id;
  id.parse("DDA85C", NULL);

  FLARM_TRAFFIC *traffic = nmea_info.flarm.FindTraffic(id);
  if (ok1(traffic != NULL)) {
    ok1(traffic->valid);
    ok1(traffic->alarm_level == 0);
    ok1(equals(traffic->relative_north, 100));
    ok1(equals(traffic->relative_east, -150));
    ok1(equals(traffic->relative_altitude, 10));
    ok1(equals(traffic->track, 123));
    ok1(traffic->track_received);
    ok1(equals(traffic->turn_rate, 13));
    ok1(traffic->turn_rate_received);
    ok1(equals(traffic->speed, 24));
    ok1(traffic->speed_received);
    ok1(equals(traffic->climb_rate, 1.4));
    ok1(traffic->climb_rate_received);
    ok1(traffic->type == FLARM_TRAFFIC::acTowPlane);
    ok1(!traffic->stealth);
  } else {
    skip(16, 0, "traffic == NULL");
  }

  ok1(parser.ParseNMEAString_Internal("$PFLAA,2,20,10,24,2,DEADFF,,,,,1*46",
                                      nmea_info));
  ok1(nmea_info.flarm.GetActiveTrafficCount() == 2);

  id.parse("DEADFF", NULL);
  traffic = nmea_info.flarm.FindTraffic(id);
  if (ok1(traffic != NULL)) {
    ok1(traffic->valid);
    ok1(traffic->alarm_level == 2);
    ok1(equals(traffic->relative_north, 20));
    ok1(equals(traffic->relative_east, 10));
    ok1(equals(traffic->relative_altitude, 24));
    ok1(!traffic->track_received);
    ok1(!traffic->turn_rate_received);
    ok1(!traffic->speed_received);
    ok1(!traffic->climb_rate_received);
    ok1(traffic->type == FLARM_TRAFFIC::acGlider);
    ok1(traffic->stealth);
  } else {
    skip(12, 0, "traffic == NULL");
  }
}

static void
TestBorgeltB50()
{
  Device *device = b50Device.CreateOnPort(dummy_config, NULL);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$PBB50,042,-01.1,1.0,12345,10,1.3,1,-28", nmea_info));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 21.60666666666667));
  ok1(equals(nmea_info.indicated_airspeed, 57.15892189196558));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -0.5658888888888889));
  ok1(nmea_info.settings.mac_cready_available);
  ok1(equals(nmea_info.settings.mac_cready, 0.5144444444444444));
  ok1(nmea_info.settings.bugs_available);
  ok1(equals(nmea_info.settings.bugs, 0.9));
  ok1(nmea_info.switch_state.flight_mode == SwitchInfo::MODE_CIRCLING);
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature, 245.15));

  delete device;
}

static void
TestCAI302()
{
  Device *device = cai302Device.CreateOnPort(dummy_config, NULL);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("!w,000,000,0000,500,01287,01020,-0668,191,199,191,000,000,100*44",
                        nmea_info));
  ok1(nmea_info.airspeed_available);
  ok1(nmea_info.total_energy_vario_available);
  ok1(!nmea_info.engine_noise_level_available);

  ok1(device->ParseNMEA("$PCAID,N,500,0,*14", nmea_info));
  ok1(nmea_info.engine_noise_level_available);
  ok1(nmea_info.engine_noise_level == 0);

  /* pressure altitude enabled (PCAID) */
  ok1(device->ParseNMEA("$PCAID,N,500,0,*14", nmea_info));
  ok1(nmea_info.pressure_altitude_available);
  ok1(between(nmea_info.pressure_altitude, 499, 501));

  /* ENL */
  ok1(device->ParseNMEA("$PCAID,N,500,100,*15", nmea_info));
  ok1(nmea_info.engine_noise_level_available);
  ok1(nmea_info.engine_noise_level == 100);

  /* baro altitude enabled */
  ok1(device->ParseNMEA("!w,000,000,0000,500,01287,01020,-0668,191,199,191,000,000,100*44",
                        nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 287));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, -6.68));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -0.463));

  /* PCAID should not override !w */
  ok1(device->ParseNMEA("$PCAID,N,500,0,*14", nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 287));

  /* MC, ballast, bugs */
  ok1(device->ParseNMEA("!w,0,0,0,0,0,0,0,0,0,0,10,50,90*56", nmea_info));
  ok1(nmea_info.settings.mac_cready_available);
  ok1(equals(nmea_info.settings.mac_cready, 0.5144444444444444));
  ok1(nmea_info.settings.ballast_fraction_available);
  ok1(equals(nmea_info.settings.ballast_fraction, 0.5));
  ok1(nmea_info.settings.bugs_available);
  ok1(equals(nmea_info.settings.bugs, 0.9));

  delete device;
}

static void
TestFlymasterF1()
{
  Device *device = flymasterf1Device.CreateOnPort(dummy_config, NULL);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  /* baro altitude disabled */
  ok1(device->ParseNMEA("$VARIO,999.98,-12,12.4,12.7,0,21.3,25.5*CS",
                        nmea_info));
  ok1(!nmea_info.airspeed_available);
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -1.2));
  ok1(!nmea_info.temperature_available);

  /* baro altitude enabled */
  ok1(device->ParseNMEA("$VARIO,999.98,-1.2,12.4,12.7,0,21.3,25.5*CS",
                        nmea_info));
  ok1(!nmea_info.baro_altitude_available);
  ok1(!nmea_info.pressure_altitude_available);
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure, 99998));

  delete device;
}

static void
TestFlytec()
{
  Device *device = flytec_device_driver.CreateOnPort(dummy_config, NULL);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$BRSF,063,-013,-0035,1,193,00351,535,485*38",
                        nmea_info));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 17.5));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$VMVABD,1234.5,M,0547.0,M,-0.0,,,MS,63.0,KH,22.4,C*51",
                        nmea_info));
  ok1(nmea_info.gps_altitude_available);
  ok1(equals(nmea_info.gps_altitude, 1234.5));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 547.0));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 17.5));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature, 295.55));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$FLYSEN,,,,,,,,,V,,101450,02341,0334,02000,,,,,,,,,*5e",
                        nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure, 1014.5));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 2341));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 3.34));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 20.0));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$FLYSEN,,,,,,,,,,V,,101450,02341,0334,02000,,,,,,,,,*5e",
                        nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure, 1014.5));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 2341));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 3.34));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 20.0));

  ok1(!device->ParseNMEA("$FLYSEN,,,,,,,,,,,,,,,,,,,,*5e", nmea_info));

  delete device;
}

static void
TestLeonardo()
{
  Device *device = leonardo_device_driver.CreateOnPort(dummy_config, NULL);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$C,+2025,-7,+18,+25,+29,122,314,314,0,-356,+25,45,T*3D",
                        nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 2025));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -0.7));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 5));
  ok1(nmea_info.netto_vario_available);
  ok1(equals(nmea_info.netto_vario, 2.5));

  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature, 302.15));

  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 45));
  ok1(equals(nmea_info.external_wind.norm, 6.94444444));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$D,+7,100554,+25,18,+31,,0,-356,+25,+11,115,96*6A",
                        nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 0.7));
  ok1(nmea_info.netto_vario_available);
  ok1(equals(nmea_info.netto_vario, 2.5));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 5));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature, 304.15));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$PDGFTL1,2025,2000,250,-14,45,134,28,65,382,153*3D",
                        nmea_info));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 2025));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 2000));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 2.5));
  ok1(nmea_info.netto_vario_available);
  ok1(equals(nmea_info.netto_vario, -1.4));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 65));
  ok1(equals(nmea_info.external_wind.norm, 7.777777));
  ok1(nmea_info.voltage_available);
  ok1(equals(nmea_info.voltage, 3.82));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$PDGFTTL,2025,2000,250,-14*41", nmea_info));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 2025));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 2000));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 2.5));
  ok1(nmea_info.netto_vario_available);
  ok1(equals(nmea_info.netto_vario, -1.4));

  delete device;
}

static void
TestLX(const struct DeviceRegister &driver, bool condor=false)
{
  Device *device = driver.CreateOnPort(dummy_config, NULL);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  /* pressure altitude disabled */
  ok1(device->ParseNMEA("$LXWP0,N,,1266.5,,,,,,,,248,23.1*55", nmea_info));
  ok1(!nmea_info.airspeed_available);
  ok1(!nmea_info.total_energy_vario_available);

  /* pressure altitude enabled */
  ok1(device->ParseNMEA("$LXWP0,N,,1266.5,,,,,,,,248,23.1*55", nmea_info));
  ok1((bool)nmea_info.pressure_altitude_available == !condor);
  ok1((bool)nmea_info.baro_altitude_available == condor);
  ok1(equals(condor ? nmea_info.baro_altitude : nmea_info.pressure_altitude,
             1266.5));
  ok1(!nmea_info.airspeed_available);
  ok1(!nmea_info.total_energy_vario_available);

  /* airspeed and vario available */
  ok1(device->ParseNMEA("$LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1",
                        nmea_info));
  ok1((bool)nmea_info.pressure_altitude_available == !condor);
  ok1((bool)nmea_info.baro_altitude_available == condor);
  ok1(equals(condor ? nmea_info.baro_altitude : nmea_info.pressure_altitude,
             1665.5));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 222.3/3.6));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 1.71));

  if (!condor) {
    ok1(device->ParseNMEA("$LXWP2,1.7,1.1,5,,,,", nmea_info));
    ok1(nmea_info.settings.mac_cready_available);
    ok1(equals(nmea_info.settings.mac_cready, 1.7));
    ok1(nmea_info.settings.ballast_overload_available);
    ok1(equals(nmea_info.settings.ballast_overload, 1.1));
    ok1(nmea_info.settings.bugs_available);
    ok1(equals(nmea_info.settings.bugs, 0.95));
  }

  delete device;
}

static void
TestILEC()
{
  Device *device = ilec_device_driver.CreateOnPort(dummy_config, NULL);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  /* baro altitude disabled */
  ok1(device->ParseNMEA("$PILC,PDA1,1489,-3.21*7D", nmea_info));
  ok1(!nmea_info.airspeed_available);
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -3.21));
  ok1(!nmea_info.external_wind_available);

  /* baro altitude enabled */
  ok1(device->ParseNMEA("$PILC,PDA1,1489,-3.21,274,15,58*7D", nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 1489));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -3.21));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.norm, 15 / 3.6));
  ok1(equals(nmea_info.external_wind.bearing, 274));

  delete device;
}

static void
TestVega()
{
  Device *device = vgaDevice.CreateOnPort(dummy_config, NULL);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  /* enable FLARM mode (switches the $PGRMZ parser to pressure
     altitude) */
  NMEAParser parser;
  ok1(parser.ParseNMEAString_Internal("$PFLAU,0,0,0,1,0,,0,,*63", nmea_info));
  ok1(parser.ParseNMEAString_Internal("$PGRMZ,2447,F,2*0F", nmea_info));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 745.845));

  ok1(device->ParseNMEA("$PDSWC,0,1002000,100,115*54", nmea_info));
  ok1(nmea_info.settings.mac_cready_available);
  ok1(equals(nmea_info.settings.mac_cready, 0));
  ok1(nmea_info.switch_state_available);
  ok1(nmea_info.voltage_available);
  ok1(equals(nmea_info.voltage, 11.5));

  ok1(device->ParseNMEA("$PDVDV,1,0,1062,762,9252,0*5B", nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 0.1));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 0));
  ok1(equals(nmea_info.indicated_airspeed, 0));
  ok1(!nmea_info.static_pressure_available);
  ok1(!nmea_info.baro_altitude_available);
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 762));

  /* parse $PGRMZ again, it should be ignored */
  ok1(parser.ParseNMEAString_Internal("$PGRMZ,2447,F,2*0F", nmea_info));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 762));

  delete device;
}

static void
TestWesterboer()
{
  Device *device = westerboer_device_driver.CreateOnPort(dummy_config, NULL);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$PWES0,20,-25,25,-22,2,-100,589,589,1260,1296,128,295*01",
                        nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 589));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -2.5));
  ok1(nmea_info.netto_vario_available);
  ok1(equals(nmea_info.netto_vario, -2.2));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.indicated_airspeed, 35));
  ok1(equals(nmea_info.true_airspeed, 36));
  ok1(nmea_info.voltage_available);
  ok1(equals(nmea_info.voltage, 12.8));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature, 29.5));

  ok1(device->ParseNMEA("$PWES1,20,21,0,030,1,6,385,10*1a", nmea_info));
  ok1(nmea_info.settings.mac_cready_available);
  ok1(equals(nmea_info.settings.mac_cready, 2.1));
  ok1(nmea_info.settings.wing_loading_available);
  ok1(equals(nmea_info.settings.wing_loading, 38.5));
  ok1(nmea_info.settings.bugs_available);
  ok1(equals(nmea_info.settings.bugs, 0.9));

  delete device;
}

static void
TestZander()
{
  Device *device = zanderDevice.CreateOnPort(dummy_config, NULL);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  /* baro altitude enabled */
  ok1(device->ParseNMEA("$PZAN1,02476,123456*04", nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 2476));

  ok1(device->ParseNMEA("$PZAN2,123,9850*03", nmea_info));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, fixed(34.1667)));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, fixed(-1.5)));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  ok1(device->ParseNMEA("$PZAN3,+,026,V,321,035,A,321,035,V*44", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  ok1(device->ParseNMEA("$PZAN3,+,026,V,321,035,V,321,035,V*53", nmea_info));
  ok1(!nmea_info.external_wind_available);

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,A*2f", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,A,V*55", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,V,A*55", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,A,A*42", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,V*38", nmea_info));
  ok1(!nmea_info.external_wind_available);

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,V,V*42", nmea_info));
  ok1(!nmea_info.external_wind_available);

  ok1(device->ParseNMEA("$PZAN4,1.5,+,20,39,45*15", nmea_info));
  ok1(nmea_info.settings.mac_cready_available);
  ok1(equals(nmea_info.settings.mac_cready, 1.5));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  ok1(!device->ParseNMEA("$PZAN5,,MUEHL,123.4,KM,T,234*24", nmea_info));
  ok1(!nmea_info.switch_state_available);

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  ok1(device->ParseNMEA("$PZAN5,SF,MUEHL,123.4,KM,T,234*31", nmea_info));
  ok1(nmea_info.switch_state_available);
  ok1(nmea_info.switch_state.flight_mode == SwitchInfo::MODE_CRUISE);
  ok1(nmea_info.switch_state.speed_command);

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  ok1(device->ParseNMEA("$PZAN5,VA,MUEHL,123.4,KM,T,234*33", nmea_info));
  ok1(nmea_info.switch_state_available);
  ok1(nmea_info.switch_state.flight_mode == SwitchInfo::MODE_CIRCLING);
  ok1(!nmea_info.switch_state.speed_command);

  delete device;
}

Declaration::Declaration(OrderedTask const*) {}

static void
TestDeclare(const struct DeviceRegister &driver)
{
  FaultInjectionPort port(*(Port::Handler *)NULL);
  Device *device = driver.CreateOnPort(dummy_config, &port);
  ok1(device != NULL);

  Declaration declaration(NULL);
  declaration.pilot_name = _T("Foo Bar");
  declaration.aircraft_type = _T("Cirrus");
  declaration.aircraft_registration = _T("D-3003");
  declaration.competition_id = _T("33");
  const GeoPoint gp(Angle::degrees(fixed(7.7061111111111114)),
                    Angle::degrees(fixed(51.051944444444445)));
  Waypoint wp(gp);
  wp.name = _T("Foo");
  wp.altitude = fixed(123);
  declaration.Append(wp);
  declaration.Append(wp);
  declaration.Append(wp);
  declaration.Append(wp);

  for (unsigned i = 0; i < 1024; ++i) {
    inject_port_fault = i;
    NullOperationEnvironment env;
    bool success = device->Declare(declaration, env);
    if (success || !port.running || port.timeout != 0 ||
        port.baud_rate != FaultInjectionPort::DEFAULT_BAUD_RATE)
      break;
  }

  ok1(port.running);
  ok1(port.baud_rate == FaultInjectionPort::DEFAULT_BAUD_RATE);

  delete device;
}

static void
TestFlightList(const struct DeviceRegister &driver)
{
  FaultInjectionPort port(*(Port::Handler *)NULL);
  Device *device = driver.CreateOnPort(dummy_config, &port);
  ok1(device != NULL);

  for (unsigned i = 0; i < 1024; ++i) {
    inject_port_fault = i;
    NullOperationEnvironment env;
    RecordedFlightList flight_list;
    bool success = device->ReadFlightList(flight_list, env);
    if (success || !port.running || port.timeout != 0 ||
        port.baud_rate != FaultInjectionPort::DEFAULT_BAUD_RATE)
      break;
  }

  ok1(port.running);
  ok1(port.baud_rate == FaultInjectionPort::DEFAULT_BAUD_RATE);

  delete device;
}

int main(int argc, char **argv)
{
  plan_tests(374);

  TestGeneric();
  TestFLARM();
  TestBorgeltB50();
  TestCAI302();
  TestFlymasterF1();
  TestFlytec();
  TestLeonardo();
  TestLX(lxDevice);
  TestLX(condorDevice, true);
  TestILEC();
  TestVega();
  TestWesterboer();
  TestZander();

  /* XXX the Triadis drivers have too many dependencies, not enabling
     for now */
  //TestDeclare(atrDevice);
  TestDeclare(cai302Device);
  TestDeclare(ewDevice);
  TestDeclare(ewMicroRecorderDevice);
  TestDeclare(pgDevice);
  TestDeclare(lxDevice);
  TestDeclare(imi_device_driver);
  //TestDeclare(vgaDevice);

  /* XXX Volkslogger doesn't do well with this test case */
  //TestDeclare(vlDevice);

  TestFlightList(cai302Device);
  TestFlightList(lxDevice);
  TestFlightList(imi_device_driver);

  return exit_status();
}
