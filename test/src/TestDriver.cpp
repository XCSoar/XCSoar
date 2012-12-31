/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Device/Driver/CProbe.hpp"
#include "Device/Driver/EW.hpp"
#include "Device/Driver/EWMicroRecorder.hpp"
#include "Device/Driver/FLARM.hpp"
#include "Device/Driver/FlymasterF1.hpp"
#include "Device/Driver/FlyNet.hpp"
#include "Device/Driver/Flytec.hpp"
#include "Device/Driver/GTAltimeter.hpp"
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
#include "Device/Port/NullPort.hpp"
#include "Logger/Settings.hpp"
#include "Plane/Plane.hpp"
#include "NMEA/Info.hpp"
#include "Protection.hpp"
#include "Input/InputEvents.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Operation/Operation.hpp"
#include "FaultInjectionPort.hpp"
#include "TestUtil.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Units/System.hpp"

static const DeviceConfig dummy_config = DeviceConfig();

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
  nmea_info.alive.Update(nmea_info.clock);

  /* no GPS reception */
  ok1(parser.ParseLine("$GPRMC,082310,V,,,,,230610*3f",
                                      nmea_info));
  ok1(nmea_info.alive);
  ok1(!nmea_info.location_available);
  ok1(nmea_info.date_time_utc.year == 2010);
  ok1(nmea_info.date_time_utc.month == 6);
  ok1(nmea_info.date_time_utc.day == 23);
  ok1(nmea_info.date_time_utc.hour == 8);
  ok1(nmea_info.date_time_utc.minute == 23);
  ok1(nmea_info.date_time_utc.second == 10);

  /* got a GPS fix */
  ok1(parser.ParseLine("$GPRMC,082311,A,5103.5403,N,00741.5742,E,055.3,022.4,230610,000.3,W*6C",
                                      nmea_info));
  ok1(nmea_info.alive);
  ok1(nmea_info.location_available);
  ok1(nmea_info.date_time_utc.hour == 8);
  ok1(nmea_info.date_time_utc.minute == 23);
  ok1(nmea_info.date_time_utc.second == 11);
  ok1(equals(nmea_info.location.longitude, 7.693));
  ok1(equals(nmea_info.location.latitude, 51.059));
  ok1(!nmea_info.baro_altitude_available);

  /* baro altitude (proprietary Garmin sentence) */
  ok1(parser.ParseLine("$PGRMZ,100,m,3*11", nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 100));
}

static void
TestTasman()
{
  NMEAParser parser;

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(parser.ParseLine("$PTAS1,200,200,02426,000*25", nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, fixed_zero));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, Units::ToSysUnit(fixed(426), Unit::FEET)));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, fixed_zero));

  ok1(parser.ParseLine("$PTAS1,234,000,00426,062*26", nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, Units::ToSysUnit(fixed(3.4), Unit::KNOTS)));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, Units::ToSysUnit(fixed(-1574), Unit::FEET)));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, Units::ToSysUnit(fixed(62), Unit::KNOTS)));
}

static void
TestFLARM()
{
  NMEAParser parser;

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(parser.ParseLine("$PFLAU,3,1,1,1,0*50",
                                      nmea_info));
  ok1(nmea_info.flarm.status.rx == 3);
  ok1(nmea_info.flarm.status.tx);
  ok1(nmea_info.flarm.status.gps == FlarmStatus::GPSStatus::GPS_2D);
  ok1(nmea_info.flarm.status.alarm_level == FlarmTraffic::AlarmType::NONE);
  ok1(nmea_info.flarm.traffic.GetActiveTrafficCount() == 0);
  ok1(!nmea_info.flarm.traffic.new_traffic);

  ok1(parser.ParseLine("$PFLAA,0,100,-150,10,2,DDA85C,123,13,24,1.4,2*7f",
                                      nmea_info));
  ok1(nmea_info.flarm.traffic.new_traffic);
  ok1(nmea_info.flarm.traffic.GetActiveTrafficCount() == 1);

  FlarmId id = FlarmId::Parse("DDA85C", NULL);

  FlarmTraffic *traffic = nmea_info.flarm.traffic.FindTraffic(id);
  if (ok1(traffic != NULL)) {
    ok1(traffic->valid);
    ok1(traffic->alarm_level == FlarmTraffic::AlarmType::NONE);
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
    ok1(traffic->type == FlarmTraffic::AircraftType::TOW_PLANE);
    ok1(!traffic->stealth);
  } else {
    skip(16, 0, "traffic == NULL");
  }

  ok1(parser.ParseLine("$PFLAA,2,20,10,24,2,DEADFF,,,,,1*46",
                                      nmea_info));
  ok1(nmea_info.flarm.traffic.GetActiveTrafficCount() == 2);

  id = FlarmId::Parse("DEADFF", NULL);
  traffic = nmea_info.flarm.traffic.FindTraffic(id);
  if (ok1(traffic != NULL)) {
    ok1(traffic->valid);
    ok1(traffic->alarm_level == FlarmTraffic::AlarmType::IMPORTANT);
    ok1(equals(traffic->relative_north, 20));
    ok1(equals(traffic->relative_east, 10));
    ok1(equals(traffic->relative_altitude, 24));
    ok1(!traffic->track_received);
    ok1(!traffic->turn_rate_received);
    ok1(!traffic->speed_received);
    ok1(!traffic->climb_rate_received);
    ok1(traffic->type == FlarmTraffic::AircraftType::GLIDER);
    ok1(traffic->stealth);
  } else {
    skip(12, 0, "traffic == NULL");
  }

  ok1(parser.ParseLine("$PFLAA,0,1206,574,21,2,DDAED5,196,,32,1.0,1*10",
                       nmea_info));
  ok1(nmea_info.flarm.traffic.GetActiveTrafficCount() == 3);

  id = FlarmId::Parse("DDAED5", NULL);
  traffic = nmea_info.flarm.traffic.FindTraffic(id);
  if (ok1(traffic != NULL)) {
    ok1(traffic->valid);
    ok1(traffic->alarm_level == FlarmTraffic::AlarmType::NONE);
    ok1(equals(traffic->relative_north, 1206));
    ok1(equals(traffic->relative_east, 574));
    ok1(equals(traffic->relative_altitude, 21));
    ok1(equals(traffic->track, 196));
    ok1(traffic->track_received);
    ok1(!traffic->turn_rate_received);
    ok1(equals(traffic->speed, 32));
    ok1(traffic->speed_received);
    ok1(equals(traffic->climb_rate, 1.0));
    ok1(traffic->climb_rate_received);
    ok1(traffic->type == FlarmTraffic::AircraftType::GLIDER);
    ok1(!traffic->stealth);
  } else {
    skip(15, 0, "traffic == NULL");
  }
}

static void
TestAltairRU()
{
  NullPort null;
  Device *device = atrDevice.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$PTFRS,1,0,0,0,0,0,0,0,5,1,10,0,3,1338313437,0,0,0,,,2*4E",
                        nmea_info));

  ok1(nmea_info.engine_noise_level_available);
  ok1(nmea_info.engine_noise_level == 5);
  ok1(!nmea_info.voltage_available);

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$PTFRS,1,0,0,0,0,0,0,0,342,1,10,0,3,1338313438,0,0,12743,,,2*42",
                        nmea_info));

  ok1(nmea_info.engine_noise_level_available);
  ok1(nmea_info.engine_noise_level == 342);
  ok1(nmea_info.voltage_available);
  ok1(equals(nmea_info.voltage, 12.743));
}

static void
TestGTAltimeter()
{
  NullPort null;
  Device *device = gt_altimeter_device_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$LK8EX1,99545,149,1,26,5.10*18", nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetHectoPascal(), 995.45));
  ok1(!nmea_info.pressure_altitude_available);
  ok1(nmea_info.noncomp_vario_available);
  ok1(equals(nmea_info.noncomp_vario, 0.01));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature, 26));
  ok1(!nmea_info.battery_level_available);
  ok1(nmea_info.voltage_available);
  ok1(equals(nmea_info.voltage, 5.1));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$LK8EX1,999999,149,-123,,1076,*32", nmea_info));
  ok1(!nmea_info.static_pressure_available);
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 149));
  ok1(nmea_info.noncomp_vario_available);
  ok1(equals(nmea_info.noncomp_vario, -1.23));
  ok1(!nmea_info.temperature_available);
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 76));
  ok1(!nmea_info.voltage_available);
}

static void
TestBorgeltB50()
{
  NullPort null;
  Device *device = b50Device.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$PBB50,042,-01.1,1.0,12345,10,1.3,1,-28*75", nmea_info));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 21.60666666666667));
  ok1(equals(nmea_info.indicated_airspeed, 57.15892189196558));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -0.5658888888888889));
  ok1(nmea_info.settings.mac_cready_available);
  ok1(equals(nmea_info.settings.mac_cready, 0.5144444444444444));
  ok1(nmea_info.settings.bugs_available);
  ok1(equals(nmea_info.settings.bugs, 0.9));
  ok1(nmea_info.settings.ballast_overload_available);
  ok1(equals(nmea_info.settings.ballast_overload, 1.3));
  ok1(nmea_info.switch_state.flight_mode == SwitchInfo::FlightMode::CIRCLING);
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature, 245.15));

  delete device;
}

static void
TestCAI302()
{
  NullPort null;
  Device *device = cai302Device.CreateOnPort(dummy_config, null);
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
TestCProbe()
{
  NullPort null;
  Device *device = c_probe_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$PCPROBE,T,FD92,FF93,00D9,FD18,017E,FEDB,0370,0075,00D6,0064,001C,000000,,",
                        nmea_info));
  ok1(nmea_info.attitude.pitch_angle_available);
  ok1(equals(nmea_info.attitude.pitch_angle, 25.6034467702));
  ok1(nmea_info.attitude.bank_angle_available);
  ok1(equals(nmea_info.attitude.bank_angle, -11.9963939863));
  ok1(nmea_info.attitude.heading_available);
  ok1(equals(nmea_info.attitude.heading, 257.0554705429));
  ok1(nmea_info.acceleration.available);
  ok1(nmea_info.acceleration.real);
  ok1(equals(nmea_info.acceleration.g_load, 1.0030817514));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature,
             Units::ToSysUnit(fixed(11.7), Unit::DEGREES_CELCIUS)));
  ok1(nmea_info.humidity_available);
  ok1(equals(nmea_info.humidity, 21.4));
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 100.0));

  delete device;
}

static void
TestFlymasterF1()
{
  NullPort null;
  Device *device = flymasterf1Device.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$VARIO,999.98,-12,12.4,12.7,0,21.3,25.5*66",
                        nmea_info));
  ok1(!nmea_info.airspeed_available);
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -1.2));
  ok1(!nmea_info.voltage_available);
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature,
             Units::ToSysUnit(fixed(21.3), Unit::DEGREES_CELCIUS)));
  ok1(!nmea_info.baro_altitude_available);
  ok1(!nmea_info.pressure_altitude_available);
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 99998));

  ok1(device->ParseNMEA("$VARIO,999.98,-12,12.4,12.7,1,21.3,25.5*67",
                        nmea_info));
  ok1(nmea_info.voltage_available);
  ok1(equals(nmea_info.voltage, 12.4));

  ok1(device->ParseNMEA("$VARIO,999.98,-12,12.4,12.7,2,21.3,25.5*64",
                        nmea_info));
  ok1(nmea_info.voltage_available);
  ok1(equals(nmea_info.voltage, 12.7));

  delete device;
}

static void
TestFlyNet()
{
  NullPort null;
  Device *device = flynet_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("_PRS 00017CBA", nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 97466));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("_PRS 00018BCD", nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 101325));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("_BAT 0", nmea_info));
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 0));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("_BAT 7", nmea_info));
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 70));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("_BAT A", nmea_info));
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 100));

  delete device;
}

static void
TestFlytec()
{
  NullPort null;
  Device *device = flytec_device_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$BRSF,063,-013,-0035,1,193,00351,535,485*33",
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

  ok1(device->ParseNMEA("$FLYSEN,,,,,,,,,V,,101450,02341,0334,02000,,,,,,,,,*72",
                        nmea_info));
  ok1(!nmea_info.date_available);
  ok1(!nmea_info.time_available);
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 101450));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 2341));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 3.34));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 200));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$FLYSEN,,,,,,,,,,V,,101450,02341,0334,02000,,,,,,,,,*5e",
                        nmea_info));
  ok1(!nmea_info.date_available);
  ok1(!nmea_info.time_available);
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 101450));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 2341));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 3.34));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 200));
  ok1(!nmea_info.battery_level_available);
  ok1(!nmea_info.temperature_available);

  ok1(!device->ParseNMEA("$FLYSEN,,,,,,,,,,,,,,,,,,,,*5e", nmea_info));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$FLYSEN,241211,201500,4700.840,N,00818.457,E,092,"
                        "01100,01234,A,09,097517,01321,-001,01030,P,023,,038,"
                        "088,00090,00088,800,,*29", nmea_info));
  ok1(nmea_info.date_available);
  ok1(nmea_info.date_time_utc.day == 24);
  ok1(nmea_info.date_time_utc.month == 12);
  ok1(nmea_info.date_time_utc.year == 2011);
  ok1(nmea_info.time_available);
  ok1(nmea_info.date_time_utc.hour == 20);
  ok1(nmea_info.date_time_utc.minute == 15);
  ok1(nmea_info.date_time_utc.second == 00);
  ok1(nmea_info.location_available);
  ok1(equals(nmea_info.location, 47.014, 8.307616667));
  ok1(nmea_info.track_available);
  ok1(equals(nmea_info.track, 92));
  ok1(nmea_info.ground_speed_available);
  ok1(equals(nmea_info.ground_speed, 110));
  ok1(nmea_info.gps_altitude_available);
  ok1(equals(nmea_info.gps_altitude, 1234));
  ok1(nmea_info.gps.satellites_used_available);
  ok1(nmea_info.gps.satellites_used == 9);
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 97517));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 1321));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -0.01));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 103));
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, (88.0 + 38.0) / 2));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature, Units::ToSysUnit(fixed(23), Unit::DEGREES_CELCIUS)));

  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$FLYSEN,241211,201500,4700.840,N,00818.457,E,092,"
                        "01100,01234,V,09,097517,01321,-001,01030,P,023,017,038,"
                        ",00090,00088,800,,*38", nmea_info));
  ok1(nmea_info.date_available);
  ok1(nmea_info.date_time_utc.day == 24);
  ok1(nmea_info.date_time_utc.month == 12);
  ok1(nmea_info.date_time_utc.year == 2011);
  ok1(nmea_info.time_available);
  ok1(nmea_info.date_time_utc.hour == 20);
  ok1(nmea_info.date_time_utc.minute == 15);
  ok1(nmea_info.date_time_utc.second == 00);
  ok1(!nmea_info.location_available);
  ok1(!nmea_info.track_available);
  ok1(!nmea_info.ground_speed_available);
  ok1(!nmea_info.gps_altitude_available);
  ok1(nmea_info.gps.satellites_used_available);
  ok1(nmea_info.gps.satellites_used == 9);
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 97517));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 1321));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -0.01));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 103));
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 38.0));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature, Units::ToSysUnit(fixed(17), Unit::DEGREES_CELCIUS)));

  delete device;
}

static void
TestLeonardo()
{
  NullPort null;
  Device *device = leonardo_device_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  ok1(device->ParseNMEA("$C,+2025,-7,+18,+25,+29,122,314,314,0,-356,+25,45,T*3D",
                        nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 2025));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -0.07));
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

  ok1(device->ParseNMEA("$c,+2025,-2,+18*5C", nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 2025));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -0.02));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed,
             Units::ToSysUnit(fixed(18), Unit::KILOMETER_PER_HOUR)));
  ok1(!nmea_info.netto_vario_available);

  nmea_info.Reset();
  nmea_info.clock = fixed(1);

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
  NullPort null;
  Device *device = driver.CreateOnPort(dummy_config, null);
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
  ok1(device->ParseNMEA("$LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1*47",
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
    ok1(device->ParseNMEA("$LXWP2,1.7,1.1,5,,,,*3e", nmea_info));
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
TestLXV7()
{
  NullPort null;
  Device *device = lxDevice.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo basic;
  basic.Reset();
  basic.clock = fixed_one;

  ok1(device->ParseNMEA("$PLXVF,,1.00,0.87,-0.12,-0.25,90.2,244.3,*64", basic));
  ok1(basic.netto_vario_available);
  ok1(equals(basic.netto_vario, -0.25));
  ok1(basic.airspeed_available);
  ok1(equals(basic.indicated_airspeed, 90.2));
  ok1(basic.pressure_altitude_available);
  ok1(equals(basic.pressure_altitude, 244.3));

  ok1(device->ParseNMEA("$PLXVS,23.1,0,12.3,*71", basic));
  ok1(basic.temperature_available);
  ok1(equals(basic.temperature, 296.25));
  ok1(basic.switch_state_available);
  ok1(basic.switch_state.flight_mode == SwitchInfo::FlightMode::CIRCLING);
  ok1(!basic.switch_state.speed_command);
  ok1(basic.voltage_available);
  ok1(equals(basic.voltage, 12.3));

  delete device;
}

static void
TestILEC()
{
  NullPort null;
  Device *device = ilec_device_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  /* baro altitude disabled */
  ok1(device->ParseNMEA("$PILC,PDA1,1489,-3.21*69", nmea_info));
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
  NullPort null;
  Device *device = vgaDevice.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = fixed_one;

  /* enable FLARM mode (switches the $PGRMZ parser to pressure
     altitude) */
  NMEAParser parser;
  ok1(parser.ParseLine("$PFLAU,0,0,0,1,0,,0,,*63", nmea_info));
  ok1(parser.ParseLine("$PGRMZ,2447,F,2*0F", nmea_info));
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
  ok1(parser.ParseLine("$PGRMZ,2447,F,2*0F", nmea_info));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 762));

  delete device;
}

static void
TestWesterboer()
{
  NullPort null;
  Device *device = westerboer_device_driver.CreateOnPort(dummy_config, null);
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
  ok1(equals(nmea_info.temperature, 29.5 + 273.15));

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
  NullPort null;
  Device *device = zanderDevice.CreateOnPort(dummy_config, null);
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
  ok1(nmea_info.switch_state.flight_mode == SwitchInfo::FlightMode::CRUISE);
  ok1(nmea_info.switch_state.speed_command);

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  ok1(device->ParseNMEA("$PZAN5,VA,MUEHL,123.4,KM,T,234*33", nmea_info));
  ok1(nmea_info.switch_state_available);
  ok1(nmea_info.switch_state.flight_mode == SwitchInfo::FlightMode::CIRCLING);
  ok1(!nmea_info.switch_state.speed_command);

  delete device;
}

static void
TestDeclare(const struct DeviceRegister &driver)
{
  FaultInjectionPort port(*(DataHandler *)NULL);
  Device *device = driver.CreateOnPort(dummy_config, port);
  ok1(device != NULL);

  LoggerSettings logger_settings;
  logger_settings.pilot_name = _T("Foo Bar");
  Plane plane;
  plane.registration = _T("D-3003");
  plane.competition_id = _T("33");
  plane.type = _T("Cirrus");

  Declaration declaration(logger_settings, plane, NULL);
  const GeoPoint gp(Angle::Degrees(fixed(7.7061111111111114)),
                    Angle::Degrees(fixed(51.051944444444445)));
  Waypoint wp(gp);
  wp.name = _T("Foo");
  wp.elevation = fixed(123);
  declaration.Append(wp);
  declaration.Append(wp);
  declaration.Append(wp);
  declaration.Append(wp);

  NullOperationEnvironment env;

  for (unsigned i = 0; i < 1024; ++i) {
    inject_port_fault = i;
    bool success = device->Declare(declaration, NULL, env);
    if (success || !port.running ||
        port.baud_rate != FaultInjectionPort::DEFAULT_BAUD_RATE)
      break;
  }

  device->EnableNMEA(env);

  ok1(port.baud_rate == FaultInjectionPort::DEFAULT_BAUD_RATE);

  delete device;
}

static void
TestFlightList(const struct DeviceRegister &driver)
{
  FaultInjectionPort port(*(DataHandler *)NULL);
  Device *device = driver.CreateOnPort(dummy_config, port);
  ok1(device != NULL);

  NullOperationEnvironment env;

  for (unsigned i = 0; i < 1024; ++i) {
    inject_port_fault = i;
    RecordedFlightList flight_list;
    bool success = device->ReadFlightList(flight_list, env);
    if (success || !port.running ||
        port.baud_rate != FaultInjectionPort::DEFAULT_BAUD_RATE)
      break;
  }

  device->EnableNMEA(env);

  ok1(port.baud_rate == FaultInjectionPort::DEFAULT_BAUD_RATE);

  delete device;
}

int main(int argc, char **argv)
{
  plan_tests(560);

  TestGeneric();
  TestTasman();
  TestFLARM();
  TestAltairRU();
  TestGTAltimeter();
  TestBorgeltB50();
  TestCAI302();
  TestCProbe();
  TestFlymasterF1();
  TestFlytec();
  TestLeonardo();
  TestLX(lxDevice);
  TestLX(condorDevice, true);
  TestLXV7();
  TestILEC();
  TestVega();
  TestWesterboer();
  TestZander();
  TestFlyNet();

  /* XXX the Triadis drivers have too many dependencies, not enabling
     for now */
  //TestDeclare(atrDevice);
  TestDeclare(cai302Device);
  TestDeclare(ewDevice);
  TestDeclare(ewMicroRecorderDevice);
  TestDeclare(pgDevice);
  TestDeclare(lxDevice);
  TestDeclare(imi_device_driver);
  TestDeclare(flarm_device);
  //TestDeclare(vgaDevice);

  /* XXX Volkslogger doesn't do well with this test case */
  //TestDeclare(vlDevice);

  TestFlightList(cai302Device);
  TestFlightList(lxDevice);
  TestFlightList(imi_device_driver);

  return exit_status();
}
