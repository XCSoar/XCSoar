/* Copyright_License {

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

#include "Device/Driver/Generic.hpp"
#include "Device/Driver/AltairPro.hpp"
#include "Device/Driver/BlueFlyVario.hpp"
#include "Device/Driver/BorgeltB50.hpp"
#include "Device/Driver/CAI302.hpp"
#include "Device/Driver/Condor.hpp"
#include "Device/Driver/CProbe.hpp"
#include "Device/Driver/EW.hpp"
#include "Device/Driver/EWMicroRecorder.hpp"
#include "Device/Driver/Eye.hpp"
#include "Device/Driver/FLARM.hpp"
#include "Device/Driver/FlymasterF1.hpp"
#include "Device/Driver/FlyNet.hpp"
#include "Device/Driver/Flytec.hpp"
#include "Device/Driver/LevilAHRS_G.hpp"
#include "Device/Driver/Leonardo.hpp"
#include "Device/Driver/LX.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "Device/Driver/ILEC.hpp"
#include "Device/Driver/IMI.hpp"
#include "Device/Driver/OpenVario.hpp"
#include "Device/Driver/PosiGraph.hpp"
#include "Device/Driver/Vaulter.hpp"
#include "Device/Driver/Vega.hpp"
#include "Device/Driver/Volkslogger.hpp"
#include "Device/Driver/Westerboer.hpp"
#include "Device/Driver/XCTracer.hpp"
#include "Device/Driver/Zander.hpp"
#include "Device/Driver.hpp"
#include "Device/RecordedFlight.hpp"
#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Device/Port/NullPort.hpp"
#include "Device/Declaration.hpp"
#include "Device/Config.hpp"
#include "Logger/Settings.hpp"
#include "Plane/Plane.hpp"
#include "NMEA/Info.hpp"
#include "Protection.hpp"
#include "Input/InputEvents.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Operation/Operation.hpp"
#include "FaultInjectionPort.hpp"
#include "TestUtil.hpp"
#include "Units/System.hpp"

#include <memory>

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
  nmea_info.clock = 1;
  nmea_info.alive.Update(nmea_info.clock);

  /* no GPS reception */
  ok1(parser.ParseLine("$GPRMC,082310.141,V,,,,,230610*25", nmea_info));
  ok1(nmea_info.alive);
  ok1(!nmea_info.location_available);
  ok1(nmea_info.date_time_utc.year == 2010);
  ok1(nmea_info.date_time_utc.month == 6);
  ok1(nmea_info.date_time_utc.day == 23);
  ok1(nmea_info.date_time_utc.hour == 8);
  ok1(nmea_info.date_time_utc.minute == 23);
  ok1(nmea_info.date_time_utc.second == 10);
  ok1(equals(nmea_info.time, 8 * 3600 + 23 * 60 + 10.141));

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
  ok1(nmea_info.variation_available);
  ok1(equals(nmea_info.variation, -000.3));

  /* baro altitude (proprietary Garmin sentence) */
  ok1(parser.ParseLine("$PGRMZ,100,m,3*11", nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 100));

  /* Magnetic Heading ok */
  ok1(parser.ParseLine("$HCHDM,182.7,M*25", nmea_info));
  ok1(nmea_info.heading_available);
  ok1(equals(nmea_info.heading, 182.7));

  /* Magnetic Heading bad char */
  ok1(!parser.ParseLine("$HCHDM,1x2.7,M*25", nmea_info));

  /* Magnetic Heading bad checksum */
  ok1(!parser.ParseLine("$HCHDM,182.7,M*26", nmea_info));

  ok1(parser.ParseLine("$WIMWV,12.1,T,10.1,M,A*24", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 12.1));
  ok1(nmea_info.external_wind.norm == 10.1);
}

static void
TestTasman()
{
  NMEAParser parser;

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(parser.ParseLine("$PTAS1,200,200,02426,000*25", nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 0));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, Units::ToSysUnit(426, Unit::FEET)));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 0));

  ok1(parser.ParseLine("$PTAS1,234,000,00426,062*26", nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, Units::ToSysUnit(3.4, Unit::KNOTS)));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, Units::ToSysUnit(-1574, Unit::FEET)));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, Units::ToSysUnit(62, Unit::KNOTS)));
}

static void
TestFLARM()
{
  NMEAParser parser;

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

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
  Device *device = altair_pro_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$PTFRS,1,0,0,0,0,0,0,0,5,1,10,0,3,1338313437,0,0,0,,,2*4E",
                        nmea_info));

  ok1(nmea_info.engine_noise_level_available);
  ok1(nmea_info.engine_noise_level == 5);
  ok1(!nmea_info.voltage_available);

  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$PTFRS,1,0,0,0,0,0,0,0,342,1,10,0,3,1338313438,0,0,12743,,,2*42",
                        nmea_info));

  ok1(nmea_info.engine_noise_level_available);
  ok1(nmea_info.engine_noise_level == 342);
  ok1(nmea_info.voltage_available);
  ok1(equals(nmea_info.voltage, 12.743));
}

static void
TestBlueFly()
{
  NullPort null;
  Device *device = bluefly_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

  // repeat input to get stable filter output
  ok1(device->ParseNMEA("PRS 00017CBA", nmea_info));
  ok1(device->ParseNMEA("PRS 00017CBA", nmea_info));
  ok1(device->ParseNMEA("PRS 00017CBA", nmea_info));
  ok1(device->ParseNMEA("PRS 00017CBA", nmea_info));
  ok1(device->ParseNMEA("PRS 00017CBA", nmea_info));
  ok1(device->ParseNMEA("PRS 00017CBA", nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 97466));

  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("PRS 00017CCA", nmea_info));
  ok1(device->ParseNMEA("PRS 00017CCA", nmea_info));
  ok1(device->ParseNMEA("PRS 00017CCA", nmea_info));
  ok1(device->ParseNMEA("PRS 00017CCA", nmea_info));
  ok1(device->ParseNMEA("PRS 00017CCA", nmea_info));
  ok1(device->ParseNMEA("PRS 00017CCA", nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 97482));

  ok1(device->ParseNMEA("BAT 1068", nmea_info)); //4.2V
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 100.0));
  ok1(device->ParseNMEA("BAT EFE", nmea_info)); //3.84V
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 50.0));
  ok1(device->ParseNMEA("BAT ED8", nmea_info)); //3.80V
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 37.3333));

  delete device;
}

static void
TestBorgeltB50()
{
  NullPort null;
  Device *device = b50_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

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
  ok1(nmea_info.switch_state.flight_mode == SwitchState::FlightMode::CIRCLING);
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature.ToKelvin(), 245.15));

  delete device;
}

static void
TestCAI302()
{
  NullPort null;
  Device *device = cai302_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

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
  nmea_info.clock = 1;

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
  ok1(equals(nmea_info.temperature.ToKelvin(),
             Temperature::FromCelsius(11.7).ToKelvin()));
  ok1(nmea_info.humidity_available);
  ok1(equals(nmea_info.humidity, 21.4));
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 100.0));
  ok1(nmea_info.dyn_pressure_available);
  ok1(equals(nmea_info.dyn_pressure.GetPascal(), 2.8));

  delete device;
}

static void
TestEye()
{
  NullPort null;
  std::unique_ptr<Device> device(eye_driver.CreateOnPort(dummy_config, null));
  ok1(device);

  NMEAInfo nmea_info;


  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$PEYA,1015.5,1020.5,3499,1012.3,265,12,176,+05.4,+15.2,095,1650,+05.1,+3.9*3a",
                        nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetHectoPascal(), 1015.5));
  ok1(nmea_info.pitot_pressure_available);
  ok1(equals(nmea_info.pitot_pressure.GetHectoPascal(), 1020.5));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 3499));
  ok1(nmea_info.settings.qnh_available);
  ok1(equals(nmea_info.settings.qnh.GetHectoPascal(), 1012.3));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 265));
  ok1(equals(nmea_info.external_wind.norm,
             Units::ToSysUnit(12, Unit::KILOMETER_PER_HOUR)));
  ok1(nmea_info.airspeed_available);
  ok1(nmea_info.airspeed_real);
  ok1(equals(nmea_info.true_airspeed,
             Units::ToSysUnit(176, Unit::KILOMETER_PER_HOUR)));
  ok1(nmea_info.noncomp_vario_available);
  ok1(equals(nmea_info.noncomp_vario, 5.4));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature.ToKelvin(),
             Temperature::FromCelsius(15.2).ToKelvin()));
  ok1(nmea_info.humidity_available);
  ok1(equals(nmea_info.humidity, 95));


  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$PEYI,+110,+020,+135,+130,+140,+0.12,+1.03,+9.81,+12,248,246,+02.3,*16",
                        nmea_info));
  ok1(nmea_info.attitude.bank_angle_available);
  ok1(equals(nmea_info.attitude.bank_angle, 110));
  ok1(nmea_info.attitude.pitch_angle_available);
  ok1(equals(nmea_info.attitude.pitch_angle, 20));
  ok1(nmea_info.attitude.heading_available);
  ok1(equals(nmea_info.attitude.heading, 248));
  ok1(nmea_info.acceleration.available);
  ok1(nmea_info.acceleration.real);
  ok1(equals(nmea_info.acceleration.g_load, 9.864654074));
}

static void
TestFlymasterF1()
{
  NullPort null;
  Device *device = flymaster_f1_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$VARIO,999.98,-12,12.4,12.7,0,21.3,25.5*66",
                        nmea_info));
  ok1(!nmea_info.airspeed_available);
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -1.2));
  ok1(!nmea_info.voltage_available);
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature.ToKelvin(),
             Temperature::FromCelsius(21.3).ToKelvin()));
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
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("_PRS 00017CBA", nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 97466));

  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("_PRS 00018BCD", nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 101325));

  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("_BAT 0", nmea_info));
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 0));

  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("_BAT 7", nmea_info));
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 70));

  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("_BAT A", nmea_info));
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 100));

  delete device;
}

static void
TestFlytec()
{
  NullPort null;
  Device *device = flytec_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$BRSF,063,-013,-0035,1,193,00351,535,485*33",
                        nmea_info));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 17.5));

  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$VMVABD,1234.5,M,0547.0,M,-0.0,,,MS,63.0,KH,22.4,C*51",
                        nmea_info));
  ok1(nmea_info.gps_altitude_available);
  ok1(equals(nmea_info.gps_altitude, 1234.5));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 547.0));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 17.5));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature.ToKelvin(), 295.55));

  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$FLYSEN,,,,,,,,,V,,101450,02341,0334,02000,,,,,,,,,*72",
                        nmea_info));
  ok1(!nmea_info.date_time_utc.IsDatePlausible());
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
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$FLYSEN,,,,,,,,,,V,,101450,02341,0334,02000,,,,,,,,,*5e",
                        nmea_info));
  ok1(!nmea_info.date_time_utc.IsDatePlausible());
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
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$FLYSEN,241211,201500,4700.840,N,00818.457,E,092,"
                        "01100,01234,A,09,097517,01321,-001,01030,P,023,,038,"
                        "088,00090,00088,800,,*29", nmea_info));
  ok1(nmea_info.date_time_utc.IsDatePlausible());
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
  ok1(equals(nmea_info.temperature.ToKelvin(),
             Temperature::FromCelsius(23).ToKelvin()));

  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$FLYSEN,241211,201500,4700.840,N,00818.457,E,092,"
                        "01100,01234,V,09,097517,01321,-001,01030,P,023,017,038,"
                        ",00090,00088,800,,*38", nmea_info));
  ok1(nmea_info.date_time_utc.IsDatePlausible());
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
  ok1(equals(nmea_info.temperature.ToKelvin(),
             Temperature::FromCelsius(17).ToKelvin()));

  delete device;
}

static void
TestLeonardo()
{
  NullPort null;
  Device *device = leonardo_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

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
  ok1(equals(nmea_info.temperature.ToKelvin(), 302.15));

  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 45));
  ok1(equals(nmea_info.external_wind.norm, 6.94444444));

  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$c,+2025,-2,+18*5C", nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 2025));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -0.02));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed,
             Units::ToSysUnit(18, Unit::KILOMETER_PER_HOUR)));
  ok1(!nmea_info.netto_vario_available);

  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$D,+7,100554,+25,18,+31,,0,-356,+25,+11,115,96*6A",
                        nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 0.7));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetHectoPascal(), 1005.54));
  ok1(nmea_info.netto_vario_available);
  ok1(equals(nmea_info.netto_vario, 2.5));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 5));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature.ToKelvin(), 304.15));

  nmea_info.Reset();
  nmea_info.clock = 1;

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
  ok1(nmea_info.airspeed_available);
  ok1(nmea_info.airspeed_real);
  ok1(equals(nmea_info.indicated_airspeed,
             Units::ToSysUnit(45, Unit::KILOMETER_PER_HOUR)));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 65));
  ok1(equals(nmea_info.external_wind.norm, 7.777777));
  ok1(nmea_info.voltage_available);
  ok1(equals(nmea_info.voltage, 3.82));

  nmea_info.Reset();
  nmea_info.clock = 1;

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
TestLevilAHRS()
{
  NullPort null;
  Device *device = levil_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

  // All angles in tenth of degrees
  ok1(device->ParseNMEA("$RPYL,127,729,3215,99,88,1376,0,", nmea_info));
  ok1(nmea_info.attitude.bank_angle_available);
  ok1(equals(nmea_info.attitude.bank_angle, 12.7));
  ok1(nmea_info.attitude.pitch_angle_available);
  ok1(equals(nmea_info.attitude.pitch_angle, 72.9));
  ok1(nmea_info.attitude.heading_available);
  ok1(equals(nmea_info.attitude.heading, 321.5));
  ok1(nmea_info.acceleration.available);
  ok1(nmea_info.acceleration.real);
  ok1(equals(nmea_info.acceleration.g_load, 1.376));

  // speed in kn, alt in ft, vs in ft/min
  ok1(device->ParseNMEA("$APENV1,94,1500,0,0,0,0,", nmea_info));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.indicated_airspeed, 48.357777777));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 457.2));
  // vertical speed not implemented

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
  nmea_info.clock = 1;

  /* empty sentence */
  ok1(device->ParseNMEA("$LXWP0,N,,,,,,,,,,,*6d", nmea_info));
  ok1(!nmea_info.pressure_altitude_available);
  ok1(!nmea_info.baro_altitude_available);
  ok1(!nmea_info.airspeed_available);
  ok1(!nmea_info.total_energy_vario_available);
  ok1(!nmea_info.external_wind_available);


  nmea_info.Reset();
  nmea_info.clock = 1;

  /* altitude and wind */
  ok1(device->ParseNMEA("$LXWP0,N,,1266.5,,,,,,,,248,23.1*55", nmea_info));

  if (condor) {
    ok1(!nmea_info.pressure_altitude_available);
    ok1(nmea_info.baro_altitude_available);
    ok1(equals(nmea_info.baro_altitude, 1266.5));
  } else {
    ok1(nmea_info.pressure_altitude_available);
    ok1(!nmea_info.baro_altitude_available);
    ok1(equals(nmea_info.pressure_altitude, 1266.5));
  }

  ok1(!nmea_info.airspeed_available);
  ok1(!nmea_info.total_energy_vario_available);

  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.norm, 23.1 / 3.6));
  ok1(equals(nmea_info.external_wind.bearing, condor ? 68 : 248));


  nmea_info.Reset();
  nmea_info.clock = 1;

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

  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.norm, 10.1 / 3.6));
  ok1(equals(nmea_info.external_wind.bearing, condor ? 354 : 174));


  nmea_info.Reset();
  nmea_info.clock = 1;

  /* airspeed without altitude */
  ok1(device->ParseNMEA("$LXWP0,Y,222.3,,,,,,,,,,*55",
                        nmea_info));
  ok1(!nmea_info.pressure_altitude_available);
  ok1(!nmea_info.baro_altitude_available);
  // TODO: indicated airspeed shouldn't be available without altitude
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 222.3/3.6));
  ok1(equals(nmea_info.indicated_airspeed, 222.3/3.6));


  if (!condor) {
    ok1(device->ParseNMEA("$LXWP2,1.7,1.1,5,,,,*3e", nmea_info));
    ok1(nmea_info.settings.mac_cready_available);
    ok1(equals(nmea_info.settings.mac_cready, 1.7));
    ok1(nmea_info.settings.ballast_overload_available);
    ok1(equals(nmea_info.settings.ballast_overload, 1.1));
    ok1(nmea_info.settings.bugs_available);
    ok1(equals(nmea_info.settings.bugs, 0.95));
    ok1(!nmea_info.settings.volume_available);

    ok1(device->ParseNMEA("$LXWP2,,,,,,,76*0c", nmea_info));
    ok1(nmea_info.settings.volume_available);
    ok1(nmea_info.settings.volume == 76);


    nmea_info.Reset();
    nmea_info.clock = 1;

    // Test LX160 (sw 3.04) variant (different bugs notation)
    ok1(device->ParseNMEA("$LXWP2,1.1,1.00,1.00,2.14,-3.87,2.38*3E", nmea_info));
    ok1(nmea_info.settings.mac_cready_available);
    ok1(equals(nmea_info.settings.mac_cready, 1.1));
    ok1(nmea_info.settings.ballast_overload_available);
    ok1(equals(nmea_info.settings.ballast_overload, 1.0));
    ok1(nmea_info.settings.bugs_available);
    ok1(equals(nmea_info.settings.bugs, 1.0));
    ok1(!nmea_info.settings.volume_available);

    ok1(device->ParseNMEA("$LXWP2,1.1,1.50,1.10,1.83,-3.87,2.91*34", nmea_info));
    ok1(nmea_info.settings.mac_cready_available);
    ok1(equals(nmea_info.settings.mac_cready, 1.1));
    ok1(nmea_info.settings.ballast_overload_available);
    ok1(equals(nmea_info.settings.ballast_overload, 1.5));
    ok1(nmea_info.settings.bugs_available);
    ok1(equals(nmea_info.settings.bugs, 0.9));
    ok1(!nmea_info.settings.volume_available);

    ok1(device->ParseNMEA("$LXWP2,0.0,1.20,1.05,2.00,-3.87,2.61*30", nmea_info));
    ok1(nmea_info.settings.mac_cready_available);
    ok1(equals(nmea_info.settings.mac_cready, 0.0));
    ok1(nmea_info.settings.ballast_overload_available);
    ok1(equals(nmea_info.settings.ballast_overload, 1.2));
    ok1(nmea_info.settings.bugs_available);
    ok1(equals(nmea_info.settings.bugs, 0.95));
    ok1(!nmea_info.settings.volume_available);

    ok1(device->ParseNMEA("$LXWP2,1.5,1.00,2.5,2.14,-3.87,2.38*0C", nmea_info));
    ok1(nmea_info.settings.mac_cready_available);
    ok1(equals(nmea_info.settings.mac_cready, 1.5));
    ok1(nmea_info.settings.ballast_overload_available);
    ok1(equals(nmea_info.settings.ballast_overload, 1.0));
    ok1(nmea_info.settings.bugs_available);
    ok1(equals(nmea_info.settings.bugs, 0.975));
    ok1(!nmea_info.settings.volume_available);


    nmea_info.Reset();
    nmea_info.clock = 1;

    LXDevice &lx_device = *(LXDevice *)device;
    ok1(!lx_device.IsV7());
    ok1(!lx_device.IsNano());
    ok1(!lx_device.IsLX16xx());

    lx_device.ResetDeviceDetection();
    ok1(device->ParseNMEA("$LXWP1,V7,12345,1.0,1.0,12345*6f", nmea_info));
    ok1(lx_device.IsV7());
    ok1(!lx_device.IsNano());
    ok1(!lx_device.IsLX16xx());

    lx_device.ResetDeviceDetection();
    ok1(device->ParseNMEA("$LXWP1,NANO,12345,1.0,1.0,12345*00", nmea_info));
    ok1(!lx_device.IsV7());
    ok1(lx_device.IsNano());
    ok1(!lx_device.IsLX16xx());

    lx_device.ResetDeviceDetection();
    ok1(device->ParseNMEA("$LXWP1,1606,4294967295,1.90,1.00,4294967295*06", nmea_info));
    ok1(!lx_device.IsV7());
    ok1(!lx_device.IsNano());
    ok1(lx_device.IsLX16xx());

    ok1(nmea_info.device.product == "1606");
    ok1(nmea_info.device.serial == "4294967295");
    ok1(nmea_info.device.software_version == "1.90");
    ok1(nmea_info.device.hardware_version == "1.00");


    ok1(device->ParseNMEA("$LXWP3,47.76,0,2.0,5.0,15,30,2.5,1.0,0,100,0.1,,0*08", nmea_info));
    ok1(nmea_info.settings.qnh_available);
    ok1(equals(nmea_info.settings.qnh.GetHectoPascal(), 1015));
  }

  delete device;
}

static void
TestLXV7()
{
  NullPort null;
  Device *device = lx_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo basic;
  basic.Reset();
  basic.clock = 1;

  LXDevice &lx_device = *(LXDevice *)device;
  lx_device.ResetDeviceDetection();

  ok1(device->ParseNMEA("$PLXVF,,1.00,0.87,-0.12,-0.25,90.2,244.3,*64", basic));
  ok1(basic.netto_vario_available);
  ok1(equals(basic.netto_vario, -0.25));
  ok1(basic.airspeed_available);
  ok1(equals(basic.indicated_airspeed, 90.2));
  ok1(basic.pressure_altitude_available);
  ok1(equals(basic.pressure_altitude, 244.3));

  ok1(lx_device.IsV7());
  lx_device.ResetDeviceDetection();

  ok1(device->ParseNMEA("$PLXVS,23.1,0,12.3,*71", basic));
  ok1(basic.temperature_available);
  ok1(equals(basic.temperature.ToKelvin(), 296.25));
  ok1(basic.switch_state.flight_mode == SwitchState::FlightMode::CIRCLING);
  ok1(basic.voltage_available);
  ok1(equals(basic.voltage, 12.3));

  ok1(lx_device.IsV7());

  delete device;
}

static void
TestILEC()
{
  NullPort null;
  Device *device = ilec_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

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
  Device *device = vega_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

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
TestOpenVario()
{
  NullPort null;
  Device *device = open_vario_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

  // Empty sentence is handled by device driver
  ok1(device->ParseNMEA("$POV*49", nmea_info));

  // Checksums are validated
  ok1(!device->ParseNMEA("$POV*48", nmea_info));

  // TE vario is read
  ok1(device->ParseNMEA("$POV,E,2.15*14", nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 2.15));
  nmea_info.Reset();

  // Static pressure is read
  ok1(device->ParseNMEA("$POV,P,1018.35*39", nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetHectoPascal(), 1018.35));
  nmea_info.Reset();

  // Dynamic pressure is read
  ok1(device->ParseNMEA("$POV,Q,23.3*04", nmea_info));
  ok1(nmea_info.dyn_pressure_available);
  ok1(equals(nmea_info.dyn_pressure.GetPascal(), 23.3));
  nmea_info.Reset();

  // Total pressure is read
  ok1(device->ParseNMEA("$POV,R,1025.17*35", nmea_info));
  ok1(nmea_info.pitot_pressure_available);
  ok1(equals(nmea_info.pitot_pressure.GetHectoPascal(), 1025.17));
  nmea_info.Reset();

  // Multiple pressures are read
  ok1(device->ParseNMEA("$POV,P,1018.35,Q,23.3,R,1025.17*08", nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetHectoPascal(), 1018.35));
  ok1(nmea_info.dyn_pressure_available);
  ok1(equals(nmea_info.dyn_pressure.GetPascal(), 23.3));
  ok1(nmea_info.pitot_pressure_available);
  ok1(equals(nmea_info.pitot_pressure.GetHectoPascal(), 1025.17));
  nmea_info.Reset();

  // Airspeed is read
  ok1(device->ParseNMEA("$POV,S,123.45*05", nmea_info));
  ok1(nmea_info.airspeed_available);
  ok1(nmea_info.airspeed_real);
  ok1(equals(nmea_info.true_airspeed, 123.45 / 3.6));
  nmea_info.Reset();

  // Temperature is read
  ok1(device->ParseNMEA("$POV,T,23.52*35", nmea_info));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature.ToKelvin(),
             Temperature::FromCelsius(23.52).ToKelvin()));
  nmea_info.Reset();

  delete device;
}

static void
TestWesterboer()
{
  NullPort null;
  Device *device = westerboer_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

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
  ok1(equals(nmea_info.temperature.ToKelvin(),
             Temperature::FromCelsius(29.5).ToKelvin()));

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
  Device *device = zander_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

  /* baro altitude enabled */
  ok1(device->ParseNMEA("$PZAN1,02476,123456*04", nmea_info));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 2476));

  ok1(device->ParseNMEA("$PZAN2,123,9850*03", nmea_info));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 34.1667));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -1.5));

  nmea_info.Reset();
  nmea_info.clock = 1;
  ok1(device->ParseNMEA("$PZAN3,+,026,V,321,035,A,321,035,V*44", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = 1;
  ok1(device->ParseNMEA("$PZAN3,+,026,V,321,035,V,321,035,V*53", nmea_info));
  ok1(!nmea_info.external_wind_available);

  nmea_info.Reset();
  nmea_info.clock = 1;
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,A*2f", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = 1;
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,A,V*55", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = 1;
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,V,A*55", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = 1;
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,A,A*42", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = 1;
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,V*38", nmea_info));
  ok1(!nmea_info.external_wind_available);

  nmea_info.Reset();
  nmea_info.clock = 1;
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,V,V*42", nmea_info));
  ok1(!nmea_info.external_wind_available);

  ok1(device->ParseNMEA("$PZAN4,1.5,+,20,39,45*15", nmea_info));
  ok1(nmea_info.settings.mac_cready_available);
  ok1(equals(nmea_info.settings.mac_cready, 1.5));

  nmea_info.Reset();
  nmea_info.clock = 1;
  ok1(device->ParseNMEA("$PZAN5,,MUEHL,123.4,KM,T,234*24", nmea_info));
  ok1(nmea_info.switch_state.flight_mode == SwitchState::FlightMode::UNKNOWN);

  nmea_info.Reset();
  nmea_info.clock = 1;
  ok1(device->ParseNMEA("$PZAN5,SF,MUEHL,123.4,KM,T,234*31", nmea_info));
  ok1(nmea_info.switch_state.flight_mode == SwitchState::FlightMode::CRUISE);

  nmea_info.Reset();
  nmea_info.clock = 1;
  ok1(device->ParseNMEA("$PZAN5,VA,MUEHL,123.4,KM,T,234*33", nmea_info));
  ok1(nmea_info.switch_state.flight_mode == SwitchState::FlightMode::CIRCLING);

  delete device;
}

static void
TestVaulter()
{
  NullPort null;
  Device *device = vaulter_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

  ok1(device->ParseNMEA("$PITV3,-16.0,-23.9,147.9,23.03,1.01*1C", nmea_info));
  ok1(nmea_info.attitude.bank_angle_available);
  ok1(equals(nmea_info.attitude.bank_angle, -16.0));
  ok1(nmea_info.attitude.pitch_angle_available);
  ok1(equals(nmea_info.attitude.pitch_angle, -23.9));
  ok1(nmea_info.attitude.heading_available);
  ok1(equals(nmea_info.attitude.heading, 147.9));

  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.indicated_airspeed, 23.03));

  ok1(nmea_info.acceleration.available);
  ok1(nmea_info.acceleration.real);
  ok1(equals(nmea_info.acceleration.g_load, 1.01));

  ok1(device->ParseNMEA("$PITV4,-0.04,0.57,-0.44,-102.0,-74.8,-73.7*3F", nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -0.04));

  ok1(device->ParseNMEA("$PITV5,6.8,29.2,0.995,0.03,0,1.54*01", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 29.2));
  ok1(equals(nmea_info.external_wind.norm, 6.8));

  ok1(nmea_info.switch_state.flight_mode == SwitchState::FlightMode::CIRCLING);
  ok1(equals(nmea_info.settings.mac_cready, 1.54));

  delete device;
}

static void
TestXCTracer()
{
  NullPort null;
  Device *device = xctracer_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = 1;

  /* empty sentence */
  ok1(device->ParseNMEA("$LXWP0,N,,,,,,,,,,,*6d", nmea_info));
  ok1(!nmea_info.pressure_altitude_available);
  ok1(!nmea_info.baro_altitude_available);
  ok1(!nmea_info.airspeed_available);
  ok1(!nmea_info.total_energy_vario_available);
  ok1(!nmea_info.external_wind_available);

  /* bad checksum */
  ok1(!device->ParseNMEA("$XCTRC*6d", nmea_info));

  nmea_info.Reset();
  nmea_info.clock = 1;

  /* altitude and !wind */
  ok1(device->ParseNMEA("$LXWP0,N,,1266.5,,,,,,,,248,23.1*55", nmea_info));
  ok1(nmea_info.pressure_altitude_available);
  ok1(!nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 1266.5));

  ok1(!nmea_info.airspeed_available);
  ok1(!nmea_info.total_energy_vario_available);
  ok1(!nmea_info.external_wind_available);

  nmea_info.Reset();
  nmea_info.clock = 1;

  /* !airspeed and vario available */
  ok1(device->ParseNMEA("$LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1*47",nmea_info));
  ok1(nmea_info.pressure_altitude_available);
  ok1(!nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.pressure_altitude,1665.5));
  ok1(!nmea_info.airspeed_available);

  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 1.71));

  ok1(!nmea_info.external_wind_available);

  /* the XCTRC sentence */
  ok1(device->ParseNMEA("$XCTRC,2015,8,11,10,56,23,80,48.62825,8.104885,"
      "129.4,11.01,322.76,-5.05,,,,997.79,77*53",nmea_info));

  /* invalid date and time must be ignored */
  ok1(device->ParseNMEA("$XCTRC,3015,13,33*77",nmea_info));
  ok1(device->ParseNMEA("$XCTRC,,,,25,-1,99,33*69",nmea_info));

  /* now check the correct values */
  ok1(nmea_info.date_time_utc.year == 2015);
  ok1(nmea_info.date_time_utc.month == 8);
  ok1(nmea_info.date_time_utc.day == 11);
  ok1(nmea_info.date_time_utc.hour == 10);
  ok1(nmea_info.date_time_utc.minute == 56);
  ok1(nmea_info.date_time_utc.second == 23);
  ok1(equals(nmea_info.time, 10 * 3600 + 56 * 60 + 23.80));

  ok1(nmea_info.location_available);
  ok1(equals(nmea_info.location.longitude, 8.104885));
  ok1(equals(nmea_info.location.latitude, 48.62825));

  ok1(nmea_info.gps_altitude_available);
  ok1(equals(nmea_info.gps_altitude, 129.4));

  ok1(nmea_info.ground_speed_available);
  ok1(equals(nmea_info.ground_speed, 11.01));

  ok1(nmea_info.track_available);
  ok1(equals(nmea_info.track,Angle::Degrees(322.76)));

  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -5.05));

  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level,77));

  delete device;
}

#ifdef __clang__
/* true, the nullptr cast below is a bad kludge */
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

static void
TestDeclare(const struct DeviceRegister &driver)
{
  FaultInjectionPort port(nullptr, *(DataHandler *)nullptr);
  Device *device = driver.CreateOnPort(dummy_config, port);
  ok1(device != NULL);

  LoggerSettings logger_settings;
  logger_settings.pilot_name = _T("Foo Bar");
  Plane plane;
  plane.registration = _T("D-3003");
  plane.competition_id = _T("33");
  plane.type = _T("Cirrus");

  Declaration declaration(logger_settings, plane, NULL);
  const GeoPoint gp(Angle::Degrees(7.7061111111111114),
                    Angle::Degrees(51.051944444444445));
  Waypoint wp(gp);
  wp.name = _T("Foo");
  wp.elevation = 123;
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
  FaultInjectionPort port(nullptr, *(DataHandler *)nullptr);
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
  plan_tests(811);

  TestGeneric();
  TestTasman();
  TestFLARM();
  TestAltairRU();
  TestBlueFly();
  TestBorgeltB50();
  TestCAI302();
  TestCProbe();
  TestEye();
  TestFlymasterF1();
  TestFlytec();
  TestLeonardo();
  TestLevilAHRS();
  TestLX(lx_driver);
  TestLX(condor_driver, true);
  TestLXV7();
  TestILEC();
  TestOpenVario();
  TestVega();
  TestWesterboer();
  TestZander();
  TestFlyNet();
  TestVaulter();
  TestXCTracer();

  /* XXX the Triadis drivers have too many dependencies, not enabling
     for now */
  //TestDeclare(altair_pro_driver);
  TestDeclare(cai302_driver);
  TestDeclare(ew_driver);
  TestDeclare(ew_microrecorder_driver);
  TestDeclare(posigraph_driver);
  TestDeclare(lx_driver);
  TestDeclare(imi_driver);
  TestDeclare(flarm_driver);
  //TestDeclare(vega_driver);

  /* XXX Volkslogger doesn't do well with this test case */
  //TestDeclare(volkslogger_driver);

  TestFlightList(cai302_driver);
  TestFlightList(lx_driver);
  TestFlightList(imi_driver);

  return exit_status();
}
