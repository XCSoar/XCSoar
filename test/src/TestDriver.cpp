// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Config.hpp"
#include "Device/Declaration.hpp"
#include "Device/Driver.hpp"
#include "Device/Driver/AirControlDisplay.hpp"
#include "Device/Driver/AltairPro.hpp"
#include "Device/Driver/BlueFlyVario.hpp"
#include "Device/Driver/BorgeltB50.hpp"
#include "Device/Driver/CAI302.hpp"
#include "Device/Driver/CProbe.hpp"
#include "Device/Driver/Condor.hpp"
#include "Device/Driver/EW.hpp"
#include "Device/Driver/EWMicroRecorder.hpp"
#include "Device/Driver/Eye.hpp"
#include "Device/Driver/FLARM.hpp"
#include "Device/Driver/FlyNet.hpp"
#include "Device/Driver/FlymasterF1.hpp"
#include "Device/Driver/Flytec.hpp"
#include "Device/Driver/Generic.hpp"
#include "Device/Driver/ILEC.hpp"
#include "Device/Driver/IMI.hpp"
#include "Device/Driver/LX.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "Device/Driver/LX/LXNavDeclare.hpp"
#include "Device/Driver/LX_Eos.hpp"
#include "Device/Driver/Larus.hpp"
#include "Device/Driver/Leonardo.hpp"
#include "Device/Driver/LevilAHRS_G.hpp"
#include "Device/Driver/LoEFGREN.hpp"
#include "Device/Driver/OpenVario.hpp"
#include "Device/Driver/PosiGraph.hpp"
#include "Device/Driver/Vaulter.hpp"
#include "Device/Driver/Vega.hpp"
#include "Device/Driver/Volkslogger.hpp"
#include "Device/Driver/Westerboer.hpp"
#include "Device/Driver/XCTracer.hpp"
#include "Device/Driver/XCVario.hpp"
#include "Device/Driver/Zander.hpp"
#include "Device/Parser.hpp"
#include "Device/Port/NullPort.hpp"
#include "FLARM/Global.hpp"
#include "FLARM/TrafficDatabases.hpp"
#include "FLARM/MessagingRecord.hpp"
#include "Device/RecordedFlight.hpp"
#include "Device/device.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Ptr.hpp"
#include "FaultInjectionPort.hpp"
#include "Input/InputEvents.hpp"
#include "Logger/Settings.hpp"
#include "NMEA/Info.hpp"
#include "Operation/Operation.hpp"
#include "Plane/Plane.hpp"
#include "Protection.hpp"
#include "TestUtil.hpp"
#include "Units/System.hpp"
#include "io/NullDataHandler.hpp"
#include "util/ByteOrder.hxx"
#include "util/PackedFloat.hxx"

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};
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
  ok1(equals(nmea_info.time, TimeStamp{FloatDuration{8 * 3600 + 23 * 60 + 10.141}}));

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
  ok1(nmea_info.attitude.heading_available);
  ok1(equals(nmea_info.attitude.heading, 182.7));

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  ok1(parser.ParseLine("$PFLAU,3,1,1,1,0*50",
                                      nmea_info));
  ok1(nmea_info.flarm.status.rx == 3);
  ok1(nmea_info.flarm.status.tx);
  ok1(nmea_info.flarm.status.gps == FlarmStatus::GPSStatus::GPS_2D);
  ok1(nmea_info.flarm.status.alarm_level == FlarmTraffic::AlarmType::NONE);
  ok1(nmea_info.flarm.traffic.GetActiveTrafficCount() == 0);
  ok1(!nmea_info.flarm.traffic.new_traffic);
  ok1(!nmea_info.flarm.status.has_extended);

  // PFLAU with all 10 fields: alarm=2, bearing=45, type=3, vert=-200,
  // dist=1500, ID=DDA85C
  ok1(parser.ParseLine("$PFLAU,5,1,2,1,2,45,3,-200,1500,DDA85C*5D",
                       nmea_info));
  ok1(nmea_info.flarm.status.rx == 5);
  ok1(nmea_info.flarm.status.alarm_level == FlarmTraffic::AlarmType::IMPORTANT);
  ok1(nmea_info.flarm.status.has_extended);
  ok1(nmea_info.flarm.status.relative_bearing == 45);
  ok1(nmea_info.flarm.status.alarm_type == 3);
  ok1(nmea_info.flarm.status.relative_vertical == -200);
  ok1(nmea_info.flarm.status.relative_distance == 1500);
  ok1(nmea_info.flarm.status.target_id == FlarmId::Parse("DDA85C", NULL));

  // PFLAU alarm=3 with negative bearing
  ok1(parser.ParseLine("$PFLAU,8,1,2,1,3,-30,2,150,800,DEADFF*63",
                       nmea_info));
  ok1(nmea_info.flarm.status.alarm_level == FlarmTraffic::AlarmType::URGENT);
  ok1(nmea_info.flarm.status.has_extended);
  ok1(nmea_info.flarm.status.relative_bearing == -30);
  ok1(nmea_info.flarm.status.alarm_type == 2);
  ok1(nmea_info.flarm.status.relative_vertical == 150);
  ok1(nmea_info.flarm.status.relative_distance == 800);
  ok1(nmea_info.flarm.status.target_id == FlarmId::Parse("DEADFF", NULL));

  // PFLAU with empty bearing fields (no alarm target)
  ok1(parser.ParseLine("$PFLAU,3,1,2,1,0,,0,,*63",
                       nmea_info));
  ok1(nmea_info.flarm.status.alarm_level == FlarmTraffic::AlarmType::NONE);
  ok1(!nmea_info.flarm.status.has_extended);

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
    ok1(traffic->source == FlarmTraffic::SourceType::FLARM);
    ok1(!traffic->rssi_available);
    ok1(!traffic->no_track);
  } else {
    skip(19, 0, "traffic == NULL");
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

  ok1(parser.ParseLine("$PFLAA,0,1206,574,21,2,DDAED5,196,,32,1.0,C*62",
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
    ok1(traffic->type == FlarmTraffic::AircraftType::AIRSHIP);
    ok1(!traffic->stealth);
  } else {
    skip(15, 0, "traffic == NULL");
  }

  // PFLAA v7+ with source=ADS-B, RSSI=-85, NoTrack=0
  ok1(parser.ParseLine("$PFLAA,0,200,-300,15,2,AABB01,180,5,30,0.8,1,2,-85,0*4B",
                       nmea_info));

  id = FlarmId::Parse("AABB01", NULL);
  traffic = nmea_info.flarm.traffic.FindTraffic(id);
  if (ok1(traffic != NULL)) {
    ok1(traffic->source == FlarmTraffic::SourceType::ADSB);
    ok1(traffic->rssi_available);
    ok1(traffic->rssi == -85);
    ok1(!traffic->no_track);
    ok1(traffic->type == FlarmTraffic::AircraftType::GLIDER);
    ok1(equals(traffic->track, 180));
  } else {
    skip(7, 0, "traffic == NULL");
  }

  // PFLAA v7+ with source=Mode-S only (no RSSI, no NoTrack)
  ok1(parser.ParseLine("$PFLAA,0,50,80,5,2,AABB02,90,,20,,8,5*64",
                       nmea_info));

  id = FlarmId::Parse("AABB02", NULL);
  traffic = nmea_info.flarm.traffic.FindTraffic(id);
  if (ok1(traffic != NULL)) {
    ok1(traffic->source == FlarmTraffic::SourceType::MODES);
    ok1(!traffic->rssi_available);
    ok1(!traffic->no_track);
    ok1(traffic->type == FlarmTraffic::AircraftType::POWERED_AIRCRAFT);
  } else {
    skip(4, 0, "traffic == NULL");
  }

  // PFLAA stealth with NoTrack=1
  ok1(parser.ParseLine("$PFLAA,0,50,80,5,2,AABB03,,,,,1,0,,1*53",
                       nmea_info));

  id = FlarmId::Parse("AABB03", NULL);
  traffic = nmea_info.flarm.traffic.FindTraffic(id);
  if (ok1(traffic != NULL)) {
    ok1(traffic->source == FlarmTraffic::SourceType::FLARM);
    ok1(traffic->stealth);
    ok1(traffic->no_track);
    ok1(!traffic->rssi_available);
  } else {
    skip(4, 0, "traffic == NULL");
  }

  // PFLAA v7+ with out-of-range source (9 -> defaults to FLARM)
  ok1(parser.ParseLine("$PFLAA,0,50,80,5,2,AABB04,90,,20,,8,9*6E",
                       nmea_info));

  id = FlarmId::Parse("AABB04", NULL);
  traffic = nmea_info.flarm.traffic.FindTraffic(id);
  if (ok1(traffic != NULL)) {
    ok1(traffic->source == FlarmTraffic::SourceType::FLARM);
  } else {
    skip(1, 0, "traffic == NULL");
  }

  // Ensure a database instance exists before PFLAM messages are parsed
  if (traffic_databases == nullptr)
    traffic_databases = new TrafficDatabases();

  ok1(parser.ParseLine("$PFLAM,U,2,DDAED5,AREG,48422D534941*0B", 
                        nmea_info));
  ok1(parser.ParseLine("$PFLAM,U,2,DDAED5,ACALL,5A4D*2F", 
                        nmea_info));
  ok1(parser.ParseLine("$PFLAM,U,2,DDAED5,PNAME,4F7276696C6C6520577269676874*43", 
                        nmea_info));
  ok1(parser.ParseLine("$PFLAM,U,2,DDAED5,ATYPE,436573736E6120313732*44", 
                        nmea_info));
  ok1(parser.ParseLine("$PFLAM,U,2,DDAED5,VHF,118.455,121.500,,*17",
                        nmea_info));

  id = FlarmId::Parse("DDAED5", NULL);
  auto mr = traffic_databases->flarm_messages.FindRecordById(id);
  if (ok1(mr.has_value())) {
    ok1(StringIsEqual(mr->registration.c_str(), "HB-SIA"));
    ok1(StringIsEqual(mr->callsign.c_str(), "ZM"));
    ok1(StringIsEqual(mr->pilot.c_str(), "Orville Wright"));
    ok1(StringIsEqual(mr->plane_type.c_str(), "Cessna 172"));
    ok1(mr->frequency.IsDefined());
    ok1(mr->frequency.GetKiloHertz() == 118455);
  } else {
    skip(6, 0, "messaging record missing");
  }

  // Clean up to avoid side effects across tests
  delete traffic_databases;
  traffic_databases = nullptr;
}

static void
TestAltairRU()
{
  NullPort null;
  std::unique_ptr<Device> device{altair_pro_driver.CreateOnPort(dummy_config, null)};
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  ok1(device->ParseNMEA("$PTFRS,1,0,0,0,0,0,0,0,5,1,10,0,3,1338313437,0,0,0,,,2*4E",
                        nmea_info));

  ok1(nmea_info.engine_noise_level_available);
  ok1(nmea_info.engine_noise_level == 5);
  ok1(!nmea_info.voltage_available);

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  ok1(device->ParseNMEA("_PRS 00017CBA", nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 97466));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  ok1(device->ParseNMEA("_PRS 00018BCD", nmea_info));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetPascal(), 101325));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  ok1(device->ParseNMEA("_BAT 0", nmea_info));
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 0));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  ok1(device->ParseNMEA("_BAT 7", nmea_info));
  ok1(nmea_info.battery_level_available);
  ok1(equals(nmea_info.battery_level, 70));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  ok1(device->ParseNMEA("$BRSF,063,-013,-0035,1,193,00351,535,485*33",
                        nmea_info));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 17.5));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
TestLoEFGREN()
{
  NullPort null;
  Device *device = loe_fgren_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  // Test positive vario (climb)
  ok1(device->ParseNMEA("$PLOF,250,80,150*32", nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 2.5));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.indicated_airspeed,
             Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR)));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature.ToKelvin(),
             Temperature::FromCelsius(15).ToKelvin()));

  // Test negative vario (sink) and negative temperature
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{2}};
  ok1(device->ParseNMEA("$PLOF,-150,60,-50*0E", nmea_info));
  ok1(equals(nmea_info.total_energy_vario, -1.5));
  ok1(equals(nmea_info.temperature.ToKelvin(),
             Temperature::FromCelsius(-5).ToKelvin()));

  // Test invalid sentences
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{3}};
  ok1(!device->ParseNMEA("$VARIO,999.98,-12*66", nmea_info));
  ok1(!device->ParseNMEA("$PLOF,250,80,150*FF", nmea_info));
  ok1(!nmea_info.total_energy_vario_available);

  delete device;
}

static void
TestLarus()
{
  NullPort null;
  Device *device = larus_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  // $PLARA attitude
  ok1(device->ParseNMEA("$PLARA,42.1,5.6,335.5*78", nmea_info));
  ok1(nmea_info.attitude.bank_angle_available);
  ok1(equals(nmea_info.attitude.bank_angle, 42.1));
  ok1(nmea_info.attitude.pitch_angle_available);
  ok1(equals(nmea_info.attitude.pitch_angle, 5.6));
  ok1(nmea_info.attitude.heading_available);
  ok1(equals(nmea_info.attitude.heading, 335.5));

  // $PLARB battery voltage (version < 0.1.4)
  ok1(device->ParseNMEA("$PLARB,13.00*4D", nmea_info));
  ok1(nmea_info.voltage_available);
  ok1(equals(nmea_info.voltage, 13.0));

  // $PLARB battery voltage and temperature (version >= 0.1.4)
  ok1(device->ParseNMEA("$PLARB,12.33,-23.8*5A", nmea_info));
  ok1(nmea_info.voltage_available);
  ok1(equals(nmea_info.voltage, 12.33));
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature.ToCelsius(), -23.8));

  // $PLARV tek vario, av_vario, baro height and tas (version < 0.1.4)
  ok1(device->ParseNMEA("$PLARV,1.90,1.96,1284,94*5D", nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 1.9));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 1284.0));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, Units::ToSysUnit(94, Unit::KILOMETER_PER_HOUR)));

  // $PLARV tek vario, av_vario, baro height, tas and gload (version >= 0.1.4)
  ok1(device->ParseNMEA("$PLARV,1.46,2.98,2608,90,002.23*6D", nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 1.46));
  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 2608.0));
  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, Units::ToSysUnit(90, Unit::KILOMETER_PER_HOUR)));
  ok1(nmea_info.acceleration.available);
  ok1(equals(nmea_info.acceleration.g_load, 2.23));

  // $PLARW wind
  ok1(device->ParseNMEA("$PLARW,73,23,A,A*5D", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 73.0));
  ok1(equals(nmea_info.external_wind.norm, Units::ToSysUnit(23, Unit::KILOMETER_PER_HOUR)));

  // $PLARS water ballast, bugs, mc, qnh, cir
  ok1(device->ParseNMEA("$PLARS,L,BAL,0.331*5C", nmea_info));
  ok1(nmea_info.settings.ballast_fraction_available);
  ok1(equals(nmea_info.settings.ballast_fraction, 0.331));
  ok1(device->ParseNMEA("$PLARS,L,BUGS,8*07", nmea_info));
  ok1(nmea_info.settings.bugs_available);
  ok1(equals(nmea_info.settings.bugs, 0.92));
  ok1(device->ParseNMEA("$PLARS,L,MC,1.8*15", nmea_info));
  ok1(nmea_info.settings.mac_cready_available);
  ok1(equals(nmea_info.settings.mac_cready, 1.8));
  ok1(device->ParseNMEA("$PLARS,L,QNH,1015.0*70", nmea_info));
  ok1(nmea_info.settings.qnh_available);
  ok1(equals(nmea_info.settings.qnh.GetHectoPascal(), 1015));
  ok1(device->ParseNMEA("$PLARS,L,CIR,1*55", nmea_info));
  ok1(nmea_info.switch_state.flight_mode == SwitchState::FlightMode::CIRCLING);
  ok1(device->ParseNMEA("$PLARS,L,CIR,0*54", nmea_info));
  ok1(nmea_info.switch_state.flight_mode == SwitchState::FlightMode::CRUISE);

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
TestLX(const struct DeviceRegister &driver, bool condor=false, bool reciprocal_wind=false)
{
  NullPort null;
  Device *device = driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  /* empty sentence */
  ok1(device->ParseNMEA("$LXWP0,N,,,,,,,,,,,*6d", nmea_info));
  ok1(!nmea_info.pressure_altitude_available);
  ok1(!nmea_info.baro_altitude_available);
  ok1(!nmea_info.airspeed_available);
  ok1(!nmea_info.total_energy_vario_available);
  ok1(!nmea_info.external_wind_available);


  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  /* altitude and wind */
  ok1(device->ParseNMEA("$LXWP0,N,,1266.5,,,,,,,,248,23.1*55", nmea_info));

  if (condor) {
    ok1(!nmea_info.pressure_altitude_available);
    ok1(nmea_info.baro_altitude_available);
    ok1(equals(nmea_info.baro_altitude, 1266.5));
    ok1(equals(nmea_info.external_wind.bearing, reciprocal_wind ? 68 : 248));
  } else {
    ok1(nmea_info.pressure_altitude_available);
    ok1(!nmea_info.baro_altitude_available);
    ok1(equals(nmea_info.pressure_altitude, 1266.5));
  }

  ok1(!nmea_info.airspeed_available);
  ok1(!nmea_info.total_energy_vario_available);

  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.norm, 23.1 / 3.6));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  ok1(equals(nmea_info.external_wind.bearing, reciprocal_wind ? 354 : 174));


  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
    nmea_info.clock = TimeStamp{FloatDuration{1}};

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
    nmea_info.clock = TimeStamp{FloatDuration{1}};

    LXDevice &lx_device = *(LXDevice *)device;
    ok1(!lx_device.IsV7());
    ok1(!lx_device.IsNano());
    ok1(!lx_device.IsLX16xx());
    ok1(!lx_device.IsSVario());

    lx_device.ResetDeviceDetection();
    ok1(device->ParseNMEA("$LXWP1,V7,12345,1.0,1.0,12345*6f", nmea_info));
    ok1(lx_device.IsV7());
    ok1(!lx_device.IsNano());
    ok1(!lx_device.IsLX16xx());
    ok1(!lx_device.IsSVario());

    lx_device.ResetDeviceDetection();
    ok1(device->ParseNMEA("$LXWP1,NANO,12345,1.0,1.0,12345*00", nmea_info));
    ok1(!lx_device.IsV7());
    ok1(lx_device.IsNano());
    ok1(!lx_device.IsLX16xx());
    ok1(!lx_device.IsSVario());

    lx_device.ResetDeviceDetection();
    ok1(device->ParseNMEA("$LXWP1,NINC,12345,1.0,1.0,12345*04", nmea_info));
    ok1(!lx_device.IsV7());
    ok1(!lx_device.IsNano());
    ok1(!lx_device.IsLX16xx());
    ok1(lx_device.IsSVario());

    lx_device.ResetDeviceDetection();
    ok1(device->ParseNMEA("$LXWP1,S8x,12345,1.0,1.0,12345*1D", nmea_info));
    ok1(!lx_device.IsV7());
    ok1(!lx_device.IsNano());
    ok1(!lx_device.IsLX16xx());
    ok1(lx_device.IsSVario());

    lx_device.ResetDeviceDetection();
    ok1(device->ParseNMEA("$LXWP1,1606,4294967295,1.90,1.00,4294967295*06", nmea_info));
    ok1(!lx_device.IsV7());
    ok1(!lx_device.IsNano());
    ok1(lx_device.IsLX16xx());
    ok1(!lx_device.IsSVario());

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
TestXCVario()
{
  NullPort null;
  Device *device = xcv_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  // $PXCV protocol version 1 (with ballast)
  ok1(device->ParseNMEA("!xcv,version,1*26", nmea_info));

  ok1(device->ParseNMEA(
      "$PXCV,1.23,2.5,05,1.20,0,15.3,1013.2,1012.8,3.4,-1.2,0.98,0.02,1.01*08",
      nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 1.23));
  ok1(nmea_info.settings.mac_cready_available);
  ok1(equals(nmea_info.settings.mac_cready, 2.5));
  ok1(nmea_info.settings.bugs_available);
  ok1(equals(nmea_info.settings.bugs, 0.95)); // 5% degradation
  ok1(nmea_info.settings.ballast_overload_available);
  ok1(equals(nmea_info.settings.ballast_overload, 1.20));
  ok1(nmea_info.switch_state.flight_mode == SwitchState::FlightMode::CRUISE);
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature.ToCelsius(), 15.3));
  ok1(nmea_info.settings.qnh_available);
  ok1(equals(nmea_info.settings.qnh.GetHectoPascal(), 1013.2));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetHectoPascal(), 1012.8));

  // $PXCV protocol version 2 (no ballast)
  ok1(device->ParseNMEA("!xcv,version,2*25", nmea_info));

  ok1(device->ParseNMEA(
      "$PXCV,-0.1,0.50,0,,1,27.4,1023.7,956.0,0.0,0.5,31.9,0.70,0.01,1.13*2C",
      nmea_info));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, -0.1));
  ok1(nmea_info.settings.mac_cready_available);
  ok1(equals(nmea_info.settings.mac_cready, 0.5));
  ok1(nmea_info.settings.bugs_available);
  ok1(equals(nmea_info.settings.bugs, 1.0)); // 0% degradation
  ok1(!nmea_info.settings
           .ballast_fraction_available); // Not available in protocol v2
  ok1(nmea_info.switch_state.flight_mode == SwitchState::FlightMode::CIRCLING);
  ok1(nmea_info.temperature_available);
  ok1(equals(nmea_info.temperature.ToCelsius(), 27.4));
  ok1(nmea_info.settings.qnh_available);
  ok1(equals(nmea_info.settings.qnh.GetHectoPascal(), 1023.7));
  ok1(nmea_info.static_pressure_available);
  ok1(equals(nmea_info.static_pressure.GetHectoPascal(), 956.0));

  delete device;
}

static void
TestLXEos()
{
  NullPort null;
  Device *device = lx_eos_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  ok1(device->ParseNMEA("$LXWP0,N,0.0,262.6,0.01,0.01,0.01,0.01,0.01,0.01,,,259,2.7*54",
                        nmea_info));

  // alt_offset is not yet known, baro altitude should be provided
  ok1(!nmea_info.pressure_altitude_available);
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 262.6));

  ok1(nmea_info.airspeed_available);
  ok1(equals(nmea_info.true_airspeed, 0.0));
  ok1(nmea_info.total_energy_vario_available);
  ok1(equals(nmea_info.total_energy_vario, 0.01));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.norm, 2.7 / 3.6));
  ok1(equals(nmea_info.external_wind.bearing, 259));

  ok1(device->ParseNMEA("$LXWP1,LX Eos,34949,1.7,1.4*7f", nmea_info));
  ok1(nmea_info.device.product == "LX Eos");
  ok1(nmea_info.device.serial == "34949");
  ok1(nmea_info.device.software_version == "1.7");
  ok1(nmea_info.device.hardware_version == "1.4");

  ok1(device->ParseNMEA("$LXWP2,1.5,1.11,13,2.96,-3.03,1.35,45*02", nmea_info));
  ok1(nmea_info.settings.mac_cready_available);
  ok1(equals(nmea_info.settings.mac_cready, 1.5));
  ok1(nmea_info.settings.bugs_available);
  ok1(equals(nmea_info.settings.bugs, 0.87));
  // Ballast won't be available, because driver doesn't know the polar

  ok1(device->ParseNMEA("$LXWP3,105,2,5.0,0,29,20,10.0,1.3,1,120,0,KA6e,0*70", nmea_info));
  ok1(nmea_info.settings.qnh_available);
  ok1(equals(nmea_info.settings.qnh.GetHectoPascal(), 1017));

  nmea_info.Reset();
  ok1(device->ParseNMEA("$LXWP0,N,0.0,260,,,,,,,,,,*5b",
                        nmea_info));
  // alt_offset and device type is known, pressure altitude should be provided
  ok1(nmea_info.pressure_altitude_available);
  ok1(!nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 260 - 32));

  nmea_info.Reset();
  ok1(device->ParseNMEA("$LXWP1,LX Era,34949,1.5,1.4*72", nmea_info));
  ok1(device->ParseNMEA("$LXWP0,N,0.0,260,,,,,,,,,,*5B",
                        nmea_info));
  // alt_offset is known, but device with firmware bug is connected, providing baro altitude
  ok1(!nmea_info.pressure_altitude_available);
  ok1(nmea_info.baro_altitude_available);

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
  basic.clock = TimeStamp{FloatDuration{1}};

  LXDevice &lx_device = *(LXDevice *)device;
  lx_device.ResetDeviceDetection();

  ok1(device->ParseNMEA("$PLXVF,,1.00,0.87,-0.12,-0.25,90.2,244.3,*64", basic));
  ok1(basic.netto_vario_available);
  ok1(equals(basic.netto_vario, -0.25));
  ok1(basic.airspeed_available);
  ok1(equals(basic.indicated_airspeed, 90.2));
  ok1(basic.pressure_altitude_available);
  ok1(equals(basic.pressure_altitude, 244.3));
  ok1(basic.acceleration.available);
  ok1(equals(basic.acceleration.g_load, 1.331));

  ok1(device->ParseNMEA("$PLXVS,23.1,0,12.3,*71", basic));
  ok1(basic.temperature_available);
  ok1(equals(basic.temperature.ToKelvin(), 296.25));
  ok1(basic.switch_state.flight_mode == SwitchState::FlightMode::CIRCLING);
  ok1(basic.voltage_available);
  ok1(equals(basic.voltage, 12.3));

  delete device;
}

static void
TestLXV7POLAR()
{
  NullPort null;
  Device *device = lx_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo basic;
  basic.Reset();
  basic.clock = TimeStamp{FloatDuration{1}};

  LXDevice &lx_device = *(LXDevice *)device;
  lx_device.ResetDeviceDetection();

  /* Test POLAR sentence parsing with all fields */
  ok1(device->ParseNMEA("$PLXV0,POLAR,W,1.780,-3.030,1.930,30.0,292,600,265,90,LS 7,0*21", basic));

  /* LX_V = 100 km/h = 27.778 m/s; a_si = a_lx / LX_V^2, b_si = b_lx / LX_V */
  constexpr double LX_V = 100.0 / 3.6;
  ok1(basic.settings.polar_coefficients_available);
  ok1(equals(basic.settings.polar_a, 1.780 / (LX_V * LX_V)));
  ok1(equals(basic.settings.polar_b, -3.030 / LX_V));
  ok1(equals(basic.settings.polar_c, 1.930));

  ok1(basic.settings.polar_load_available);
  ok1(equals(basic.settings.polar_load, 30.0));

  ok1(basic.settings.polar_reference_mass_available);
  ok1(equals(basic.settings.polar_reference_mass, 292.0));

  ok1(basic.settings.polar_maximum_mass_available);
  ok1(equals(basic.settings.polar_maximum_mass, 600.0));

  ok1(basic.settings.polar_empty_weight_available);
  ok1(equals(basic.settings.polar_empty_weight, 265.0));

  ok1(basic.settings.polar_pilot_weight_available);
  ok1(equals(basic.settings.polar_pilot_weight, 90.0));

  /* Test POL variant */
  basic.Reset();
  basic.clock = TimeStamp{FloatDuration{2}};
  ok1(device->ParseNMEA("$PLXV0,POL,W,1.240,-1.960,1.280,36.0,400,600,325,70,LS 8,0*3A", basic));

  ok1(basic.settings.polar_coefficients_available);
  ok1(equals(basic.settings.polar_a, 1.240 / (LX_V * LX_V)));
  ok1(equals(basic.settings.polar_b, -1.960 / LX_V));
  ok1(equals(basic.settings.polar_c, 1.280));
  ok1(equals(basic.settings.polar_load, 36.0));
  ok1(equals(basic.settings.polar_reference_mass, 400.0));
  ok1(equals(basic.settings.polar_maximum_mass, 600.0));
  ok1(equals(basic.settings.polar_empty_weight, 325.0));
  ok1(equals(basic.settings.polar_pilot_weight, 70.0));

  /* Test with zero pilot weight */
  basic.Reset();
  basic.clock = TimeStamp{FloatDuration{3}};
  ok1(device->ParseNMEA("$PLXV0,POLAR,W,1.780,-3.030,1.930,30.0,292,600,265,0,LS 7,0*18", basic));

  ok1(basic.settings.polar_coefficients_available);
  ok1(basic.settings.polar_pilot_weight_available);
  ok1(equals(basic.settings.polar_pilot_weight, 0.0));

  delete device;
}

static void
TestLXRadioTransponder()
{
  NullPort null;
  Device *device = lx_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo basic;
  basic.Reset();
  basic.clock = TimeStamp{FloatDuration{1}};

  /* Active frequency with station name */
  ok1(device->ParseNMEA("$PLXVC,RADIO,A,COMM,128800,CELJE*27", basic));
  ok1(basic.settings.has_active_frequency.IsValid());
  ok1(equals(basic.settings.active_frequency.GetKiloHertz(), 128800));
  ok1(basic.settings.active_freq_name.equals("CELJE"));

  /* Standby frequency without station name */
  ok1(device->ParseNMEA("$PLXVC,RADIO,A,SBY,121500*0E", basic));
  ok1(basic.settings.has_standby_frequency.IsValid());
  ok1(equals(basic.settings.standby_frequency.GetKiloHertz(), 121500));

  /* Active frequency without station name */
  basic.Reset();
  basic.clock = TimeStamp{FloatDuration{2}};
  ok1(device->ParseNMEA("$PLXVC,RADIO,A,COMM,118000*45", basic));
  ok1(basic.settings.has_active_frequency.IsValid());
  ok1(equals(basic.settings.active_frequency.GetKiloHertz(), 118000));

  /* Transponder squawk code (2000 display = 02000 octal = 1024 decimal) */
  basic.Reset();
  basic.clock = TimeStamp{FloatDuration{3}};
  ok1(device->ParseNMEA("$PLXVC,XPDR,A,SQUAWK,2000*06", basic));
  ok1(basic.settings.has_transponder_code.IsValid());
  ok1(equals(basic.settings.transponder_code.GetCode(),
             TransponderCode{02000}.GetCode()));

  /* Transponder mode ALT */
  ok1(device->ParseNMEA("$PLXVC,XPDR,A,MODE,ALT*54", basic));
  ok1(basic.settings.has_transponder_mode.IsValid());
  ok1(equals(basic.settings.transponder_mode.mode,
             TransponderMode::Mode::ALT));

  /* Emergency squawk (7700 display = 07700 octal = 4032 decimal) */
  ok1(device->ParseNMEA("$PLXVC,XPDR,A,SQUAWK,7700*04", basic));
  ok1(basic.settings.has_transponder_code.IsValid());
  ok1(equals(basic.settings.transponder_code.GetCode(),
             TransponderCode{07700}.GetCode()));

  /* Transponder mode SBY */
  ok1(device->ParseNMEA("$PLXVC,XPDR,A,MODE,SBY*45", basic));
  ok1(basic.settings.has_transponder_mode.IsValid());
  ok1(equals(basic.settings.transponder_mode.mode,
             TransponderMode::Mode::SBY));

  delete device;
}

static void
TestLXNavDeclare()
{
  /* Test coordinate formatting */

  /* Lat: 48 deg 46.667' N */
  {
    GeoPoint gp(Angle::Degrees(10.264717),
                Angle::Degrees(48.77778));
    const auto lat = LXNavDeclare::FormatLat(gp);
    ok1(lat.starts_with("4846."));
    ok1(lat.back() == 'N');

    const auto lon = LXNavDeclare::FormatLon(gp);
    ok1(lon.starts_with("01015."));
    ok1(lon.back() == 'E');
  }

  /* Southern / Western hemisphere */
  {
    GeoPoint gp(Angle::Degrees(-43.5), Angle::Degrees(-22.9));
    const auto lat = LXNavDeclare::FormatLat(gp);
    ok1(lat.back() == 'S');
    ok1(lat.starts_with("22"));

    const auto lon = LXNavDeclare::FormatLon(gp);
    ok1(lon.back() == 'W');
    ok1(lon.starts_with("043"));
  }

  /* Test OZ style mapping */
  ok1(LXNavDeclare::GetOZStyle(true, false) == 2);
  ok1(LXNavDeclare::GetOZStyle(false, false) == 1);
  ok1(LXNavDeclare::GetOZStyle(false, true) == 3);

  /* Test A12 bearing computation */
  {
    const GeoPoint start(Angle::Degrees(10.0), Angle::Degrees(48.0));
    const GeoPoint tp1(Angle::Degrees(10.0), Angle::Degrees(47.0));
    const GeoPoint tp2(Angle::Degrees(11.0), Angle::Degrees(47.0));
    const GeoPoint finish(Angle::Degrees(11.0), Angle::Degrees(48.0));

    /* Start -> next should be roughly 180 degrees (due south) */
    const Angle a12_start = LXNavDeclare::ComputeA12(
      start, start, tp1, true, false);
    ok1(a12_start.AsBearing().Degrees() > 170.0);
    ok1(a12_start.AsBearing().Degrees() < 190.0);

    /* Finish: bearing from finish toward prev */
    const Angle a12_finish = LXNavDeclare::ComputeA12(
      tp2, finish, finish, false, true);
    ok1(a12_finish.AsBearing().Degrees() > 170.0);
    ok1(a12_finish.AsBearing().Degrees() < 200.0);

    /* Intermediate: bisector of incoming and outgoing legs */
    const Angle a12_mid = LXNavDeclare::ComputeA12(
      start, tp1, tp2, false, false);
    ok1(a12_mid.AsBearing().Degrees() > 20.0);
    ok1(a12_mid.AsBearing().Degrees() < 70.0);
  }

  /* Test FormatOZLine with a cylinder start */
  {
    LoggerSettings logger_settings;
    logger_settings.pilot_name = "Test Pilot";
    Plane plane;
    plane.registration = "D-1234";
    plane.competition_id = "AB";
    plane.type = "ASW-27";

    Declaration decl(logger_settings, plane, nullptr);

    Waypoint wp_start(GeoPoint(Angle::Degrees(10.265),
                                Angle::Degrees(48.778)));
    wp_start.name = "START";
    wp_start.elevation = 585;
    wp_start.has_elevation = true;

    Waypoint wp_tp1(GeoPoint(Angle::Degrees(9.702),
                              Angle::Degrees(47.192)));
    wp_tp1.name = "TP1";
    wp_tp1.elevation = 572;
    wp_tp1.has_elevation = true;

    Waypoint wp_finish(GeoPoint(Angle::Degrees(11.552),
                                 Angle::Degrees(47.735)));
    wp_finish.name = "FINISH";
    wp_finish.elevation = 420;
    wp_finish.has_elevation = true;

    decl.Append(wp_start);
    decl.Append(wp_tp1);
    decl.Append(wp_finish);

    ok1(decl.Size() == 3);

    /* Start OZ: index=-1, Style=2 (ozNext), cylinder */
    const auto oz0 = LXNavDeclare::FormatOZLine(decl, 0);
    ok1(oz0.starts_with("LLXVOZ=-1,"));
    ok1(oz0.find("Style=2") != std::string::npos);
    ok1(oz0.find("R1=1500m") != std::string::npos);
    ok1(oz0.find("A1=180.0") != std::string::npos);
    ok1(oz0.find("Near=0") != std::string::npos);
    ok1(oz0.find("Line=1") == std::string::npos);
    ok1(oz0.find("AAT=1") == std::string::npos);

    /* Intermediate OZ: index=0, Style=1 (ozSymmetric), cylinder */
    const auto oz1 = LXNavDeclare::FormatOZLine(decl, 1);
    ok1(oz1.starts_with("LLXVOZ=0,"));
    ok1(oz1.find("Style=1") != std::string::npos);
    ok1(oz1.find("R1=1500m") != std::string::npos);

    /* Finish OZ: index=1, Style=3 (ozPrev), Near=1 */
    const auto oz2 = LXNavDeclare::FormatOZLine(decl, 2);
    ok1(oz2.starts_with("LLXVOZ=1,"));
    ok1(oz2.find("Style=3") != std::string::npos);
    ok1(oz2.find("Near=1") != std::string::npos);

    ok1(oz0.find("Lat=") != std::string::npos);
    ok1(oz0.find("Lon=") != std::string::npos);
    ok1(oz1.find("Lat=") != std::string::npos);
    ok1(oz2.find("Lon=") != std::string::npos);
  }

  /* Test FormatOZLine with a LINE shape */
  {
    Declaration decl_line({}, Plane{}, nullptr);

    Waypoint wp1(GeoPoint(Angle::Degrees(10.0), Angle::Degrees(48.0)));
    wp1.name = "S";

    Waypoint wp2(GeoPoint(Angle::Degrees(11.0), Angle::Degrees(47.0)));
    wp2.name = "F";

    Declaration::TurnPoint tp1(wp1);
    tp1.shape = Declaration::TurnPoint::LINE;
    tp1.radius = 3000;
    tp1.sector_angle = Angle::QuarterCircle();
    decl_line.turnpoints.push_back(tp1);

    Declaration::TurnPoint tp2(wp2);
    tp2.shape = Declaration::TurnPoint::CYLINDER;
    tp2.radius = 500;
    decl_line.turnpoints.push_back(tp2);

    const auto oz_line = LXNavDeclare::FormatOZLine(decl_line, 0);
    ok1(oz_line.find("Line=1") != std::string::npos);
    ok1(oz_line.find("R1=3000m") != std::string::npos);
    ok1(oz_line.find("A1=90.0") != std::string::npos);

    const auto oz_cyl = LXNavDeclare::FormatOZLine(decl_line, 1);
    ok1(oz_cyl.find("Line=1") == std::string::npos);
    ok1(oz_cyl.find("R1=500m") != std::string::npos);
  }

  /* Test FormatOZLine with AAT point */
  {
    Declaration decl_aat({}, Plane{}, nullptr);
    decl_aat.is_aat_task = true;

    Waypoint wp1(GeoPoint(Angle::Degrees(10.0), Angle::Degrees(48.0)));
    wp1.name = "S";

    Waypoint wp2(GeoPoint(Angle::Degrees(11.0), Angle::Degrees(47.5)));
    wp2.name = "AAT1";

    Waypoint wp3(GeoPoint(Angle::Degrees(10.5), Angle::Degrees(48.0)));
    wp3.name = "F";

    Declaration::TurnPoint tp_s(wp1);
    decl_aat.turnpoints.push_back(tp_s);

    Declaration::TurnPoint tp_aat(wp2);
    tp_aat.shape = Declaration::TurnPoint::CYLINDER;
    tp_aat.radius = 5100;
    tp_aat.is_aat = true;
    decl_aat.turnpoints.push_back(tp_aat);

    Declaration::TurnPoint tp_f(wp3);
    tp_f.radius = 500;
    decl_aat.turnpoints.push_back(tp_f);

    const auto oz_aat = LXNavDeclare::FormatOZLine(decl_aat, 1);
    ok1(oz_aat.find("AAT=1") != std::string::npos);
    ok1(oz_aat.find("R1=5100m") != std::string::npos);
    ok1(oz_aat.starts_with("LLXVOZ=0,"));

    const auto oz_s = LXNavDeclare::FormatOZLine(decl_aat, 0);
    ok1(oz_s.find("AAT=1") == std::string::npos);

    const auto oz_f = LXNavDeclare::FormatOZLine(decl_aat, 2);
    ok1(oz_f.find("AAT=1") == std::string::npos);
  }

  /* Test FormatOZLine with keyhole (inner radius) */
  {
    Declaration decl_kh({}, Plane{}, nullptr);

    Waypoint wp1(GeoPoint(Angle::Degrees(10.0), Angle::Degrees(48.0)));
    wp1.name = "S";
    Waypoint wp2(GeoPoint(Angle::Degrees(11.0), Angle::Degrees(47.0)));
    wp2.name = "KH";
    Waypoint wp3(GeoPoint(Angle::Degrees(10.5), Angle::Degrees(48.0)));
    wp3.name = "F";

    decl_kh.turnpoints.push_back(Declaration::TurnPoint(wp1));

    Declaration::TurnPoint tp_kh(wp2);
    tp_kh.shape = Declaration::TurnPoint::DAEC_KEYHOLE;
    tp_kh.radius = 10000;
    tp_kh.sector_angle = Angle::QuarterCircle();
    tp_kh.inner_radius = 500;
    decl_kh.turnpoints.push_back(tp_kh);

    decl_kh.turnpoints.push_back(Declaration::TurnPoint(wp3));

    const auto oz_kh = LXNavDeclare::FormatOZLine(decl_kh, 1);
    ok1(oz_kh.find("R1=10000m") != std::string::npos);
    ok1(oz_kh.find("R2=500m") != std::string::npos);
    ok1(oz_kh.find("A2=180.0") != std::string::npos);
    ok1(oz_kh.find("A1=90.0") != std::string::npos);
  }

  /* Test C-record with elevation */
  {
    Waypoint wp(GeoPoint(Angle::Degrees(13.776),
                          Angle::Degrees(47.811)));
    wp.name = "EBENSEE";
    wp.elevation = 420;
    wp.has_elevation = true;

    Declaration::TurnPoint tp(wp);
    const auto c_record = LXNavDeclare::FormatTurnPointCRecord(tp);

    ok1(c_record.starts_with("C"));
    ok1(c_record.find("EBENSEE") != std::string::npos);
    ok1(c_record.find("::420.00000") != std::string::npos);
  }

  /* Test C-record without elevation (defaults to 0) */
  {
    Waypoint wp(GeoPoint(Angle::Degrees(10.0),
                          Angle::Degrees(48.0)));
    wp.name = "NOELEVATION";

    Declaration::TurnPoint tp(wp);
    const auto c_record = LXNavDeclare::FormatTurnPointCRecord(tp);

    ok1(c_record.find("::0.00000") != std::string::npos);
  }
}

static void
TestILEC()
{
  NullPort null;
  Device *device = ilec_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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

  // Relative humidity is read
  ok1(device->ParseNMEA("$POV,H,58.42*24", nmea_info));
  ok1(nmea_info.humidity_available);
  ok1(equals(nmea_info.humidity, 58.42));

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  ok1(device->ParseNMEA("$PZAN3,+,026,V,321,035,A,321,035,V*44", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  ok1(device->ParseNMEA("$PZAN3,+,026,V,321,035,V,321,035,V*53", nmea_info));
  ok1(!nmea_info.external_wind_available);

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,A*2f", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,A,V*55", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,V,A*55", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,A,A*42", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 321));
  ok1(equals(nmea_info.external_wind.norm, 9.72222));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,V*38", nmea_info));
  ok1(!nmea_info.external_wind_available);

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  ok1(device->ParseNMEA("$PZAN3,+,026,A,321,035,V,V*42", nmea_info));
  ok1(!nmea_info.external_wind_available);

  ok1(device->ParseNMEA("$PZAN4,1.5,+,20,39,45*15", nmea_info));
  ok1(nmea_info.settings.mac_cready_available);
  ok1(equals(nmea_info.settings.mac_cready, 1.5));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  ok1(device->ParseNMEA("$PZAN5,,MUEHL,123.4,KM,T,234*24", nmea_info));
  ok1(nmea_info.switch_state.flight_mode == SwitchState::FlightMode::UNKNOWN);

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  ok1(device->ParseNMEA("$PZAN5,SF,MUEHL,123.4,KM,T,234*31", nmea_info));
  ok1(nmea_info.switch_state.flight_mode == SwitchState::FlightMode::CRUISE);

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  /* altitude and !wind */
  ok1(device->ParseNMEA("$LXWP0,N,,1266.5,,,,,,,,248,23.1*55", nmea_info));
  ok1(nmea_info.pressure_altitude_available);
  ok1(!nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 1266.5));

  ok1(!nmea_info.airspeed_available);
  ok1(!nmea_info.total_energy_vario_available);
  ok1(!nmea_info.external_wind_available);

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

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
  ok1(equals(nmea_info.time, TimeStamp{FloatDuration{10 * 3600 + 56 * 60 + 23.80}}));

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

static void
TestACD()
{
  NullPort null;
  Device *device = acd_driver.CreateOnPort(dummy_config, null);
  ok1(device != NULL);

  NMEAInfo nmea_info;

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  /* test XPDR response */
  ok1(device->ParseNMEA("$PAAVS,XPDR,7000,1,0,1697,0,0*68",nmea_info));
  ok1(nmea_info.settings.has_transponder_code);
  ok1(nmea_info.settings.has_transponder_mode);
  ok1(equals(nmea_info.settings.transponder_code.GetCode(),
             TransponderCode{07000}.GetCode()));
  ok1(StringIsEqual(nmea_info.settings.transponder_mode.GetModeString(),
                    "ALT"));
  ok1(equals(nmea_info.settings.transponder_mode.mode, TransponderMode::Mode::ALT));

  ok1(device->ParseNMEA("$PAAVS,XPDR,7260,1,0,1762,1,0*66",nmea_info));
  ok1(equals(nmea_info.settings.transponder_mode.mode, TransponderMode::Mode::IDENT));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  ok1(!(device->ParseNMEA("$PAAVS,XPDR,9999,,,1697,,*6E",nmea_info)));
  ok1(!(nmea_info.settings.transponder_code.IsDefined()));
  ok1(!(nmea_info.settings.transponder_mode.IsDefined()));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  /* test ALT response */
  ok1(device->ParseNMEA("$PAAVS,ALT,526.23,457.33,100500*0E",
                        nmea_info));

  ok1(nmea_info.pressure_altitude_available);
  ok1(equals(nmea_info.pressure_altitude, 526.23));
  ok1(nmea_info.baro_altitude_available);
  ok1(equals(nmea_info.baro_altitude, 457.33));
  ok1(nmea_info.settings.qnh_available);
  ok1(equals(nmea_info.settings.qnh.GetPascal(), 100500));

  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};

  /* test COM response */
  ok1(device->ParseNMEA("$PAAVS,COM,130330,122500,100,75,1,1,0,0*0D",
                        nmea_info));

  ok1(nmea_info.settings.active_frequency.GetKiloHertz() == 130330);
  ok1(nmea_info.settings.standby_frequency.GetKiloHertz() == 122500);
  ok1(equals(nmea_info.settings.volume, 100));

  delete device;
}

static void
TestDeclare(const struct DeviceRegister &driver)
{
  NullDataHandler handler;
  FaultInjectionPort port(nullptr, handler);
  Device *device = driver.CreateOnPort(dummy_config, port);
  ok1(device != NULL);

  LoggerSettings logger_settings;
  logger_settings.pilot_name = "Foo Bar";
  Plane plane;
  plane.registration = "D-3003";
  plane.competition_id = "33";
  plane.type = "Cirrus";

  Declaration declaration(logger_settings, plane, NULL);
  const GeoPoint gp(Angle::Degrees(7.7061111111111114),
                    Angle::Degrees(51.051944444444445));
  Waypoint wp(gp);
  wp.name = "Foo";
  wp.elevation = 123;
  wp.has_elevation = true;
  declaration.Append(wp);
  declaration.Append(wp);
  declaration.Append(wp);
  declaration.Append(wp);

  NullOperationEnvironment env;

  for (unsigned i = 0; i < 1024; ++i) {
    inject_port_fault = i;
    bool success;
    try {
      success = device->Declare(declaration, NULL, env);
    } catch (...) {
      success = false;
    }
    if (success || !port.running ||
        port.baud_rate != FaultInjectionPort::DEFAULT_BAUD_RATE)
      break;
  }

  try {
    device->EnableNMEA(env);
  } catch (...) {
  }

  ok1(port.baud_rate == FaultInjectionPort::DEFAULT_BAUD_RATE);

  delete device;
}

static void
TestFlightList(const struct DeviceRegister &driver)
{
  NullDataHandler handler;
  FaultInjectionPort port(nullptr, handler);
  Device *device = driver.CreateOnPort(dummy_config, port);
  ok1(device != NULL);

  NullOperationEnvironment env;

  for (unsigned i = 0; i < 1024; ++i) {
    inject_port_fault = i;
    RecordedFlightList flight_list;
    bool success;
    try {
      success = device->ReadFlightList(flight_list, env);
    } catch (...) {
      success = false;
    }
    if (success || !port.running ||
        port.baud_rate != FaultInjectionPort::DEFAULT_BAUD_RATE)
      break;
  }

  try {
    device->EnableNMEA(env);
  } catch (...) {
  }

  ok1(port.baud_rate == FaultInjectionPort::DEFAULT_BAUD_RATE);

  delete device;
}

/**
 * Test that NMEAParser::ReadTime() preserves sub-second precision
 * from the NMEA time field (HHMMSS.SS).
 *
 * Before the fix, ReadTime() routed through BrokenTime which only
 * stores integer seconds, losing the fractional part.
 * See: https://github.com/XCSoar/XCSoar/issues/2207
 */
static void
TestSubSecondPrecision()
{
  NMEAParser parser;

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  nmea_info.alive.Update(nmea_info.clock);

  /* parse a GPRMC sentence with .50 fractional seconds */
  ok1(parser.ParseLine("$GPRMC,082310.50,A,5103.5403,N,00741.5742,E,055.3,022.4,230610,000.3,W*46",
                        nmea_info));

  /* the integer part must be correct */
  ok1(nmea_info.date_time_utc.hour == 8);
  ok1(nmea_info.date_time_utc.minute == 23);
  ok1(nmea_info.date_time_utc.second == 10);

  /* the sub-second part must be preserved in info.time */
  ok1(equals(nmea_info.time, TimeStamp{FloatDuration{8 * 3600 + 23 * 60 + 10.50}}));
  const auto time_50 = nmea_info.time;

  /* parse a second fix at .00 of the next second */
  nmea_info.clock = TimeStamp{FloatDuration{2}};
  ok1(parser.ParseLine("$GPRMC,082311.00,A,5103.5403,N,00741.5742,E,055.3,022.4,230610,000.3,W*42",
                        nmea_info));

  ok1(equals(nmea_info.time, TimeStamp{FloatDuration{8 * 3600 + 23 * 60 + 11.00}}));

  /* the difference between the two fixes must be exactly 0.50 seconds,
     not 1.0 seconds (which would indicate truncation to integer seconds) */
  const double delta = (nmea_info.time - time_50).count();
  ok1(fabs(delta - 0.50) < 0.01);
}

/**
 * Test that the MWV parser checks the status field (field 5).
 * Status 'V' means data invalid and should be rejected.
 */
static void
TestMWVStatus()
{
  NMEAParser parser;

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  nmea_info.alive.Update(nmea_info.clock);

  /* MWV with status A (valid) — must be accepted */
  ok1(parser.ParseLine("$WIMWV,12.1,T,10.1,M,A*24", nmea_info));
  ok1(nmea_info.external_wind_available);

  /* clear the wind so we can test the invalid case */
  nmea_info.external_wind_available.Clear();

  /* MWV with status V (invalid) — must be rejected */
  ok1(parser.ParseLine("$WIMWV,12.1,T,10.1,M,V*33", nmea_info));
  ok1(!nmea_info.external_wind_available);
}

/**
 * Test that the MWV parser distinguishes between Relative (R) and
 * True (T) wind reference.  Relative wind (referenced to vessel heading)
 * should not be stored as true wind bearing.
 */
static void
TestMWVRelativeTrue()
{
  NMEAParser parser;

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  nmea_info.alive.Update(nmea_info.clock);

  /* MWV with True reference — must be accepted */
  ok1(parser.ParseLine("$WIMWV,45.0,T,10.1,M,A*27", nmea_info));
  ok1(nmea_info.external_wind_available);
  ok1(equals(nmea_info.external_wind.bearing, 45.0));

  /* clear the wind */
  nmea_info.external_wind_available.Clear();

  /* MWV with Relative reference — must be rejected (or handled differently)
     since XCSoar's external_wind expects a true bearing */
  ok1(parser.ParseLine("$WIMWV,45.0,R,10.1,M,A*21", nmea_info));
  ok1(!nmea_info.external_wind_available);
}

/**
 * Test that NMEAInfo::Complement() correctly copies stall_ratio AND
 * updates stall_ratio_available.
 */
static void
TestStallRatioComplement()
{
  NMEAInfo a, b;
  a.Reset();
  b.Reset();

  a.clock = TimeStamp{FloatDuration{1}};
  b.clock = TimeStamp{FloatDuration{1}};

  /* 'a' has no stall ratio */
  ok1(!a.stall_ratio_available);

  /* 'b' provides a stall ratio */
  b.stall_ratio = 0.75;
  b.stall_ratio_available.Update(b.clock);
  b.alive.Update(b.clock);
  ok1(b.stall_ratio_available);

  /* Complement 'a' with 'b' — both value and flag must be copied */
  a.Complement(b);
  ok1(a.stall_ratio_available);
  ok1(equals(a.stall_ratio, 0.75));
}

/**
 * Test that temperature_available and humidity_available are now
 * Validity objects that properly expire.
 */
static void
TestTemperatureHumidityValidity()
{
  NMEAInfo info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  /* initially not valid */
  ok1(!info.temperature_available);
  ok1(!info.humidity_available);

  /* after Update(), they become valid */
  info.temperature = Temperature::FromCelsius(20);
  info.temperature_available.Update(info.clock);
  info.humidity = 50;
  info.humidity_available.Update(info.clock);
  ok1(info.temperature_available);
  ok1(info.humidity_available);

  /* advance clock well past the 30s expiry timeout */
  info.clock = TimeStamp{FloatDuration{60}};
  info.Expire();
  ok1(!info.temperature_available);
  ok1(!info.humidity_available);

  /* Test Complement: temperature/humidity from second source should be
     adopted when primary has none */
  NMEAInfo a, b;
  a.Reset();
  b.Reset();
  a.clock = TimeStamp{FloatDuration{1}};
  b.clock = TimeStamp{FloatDuration{1}};

  b.alive.Update(b.clock);
  b.temperature = Temperature::FromCelsius(25);
  b.temperature_available.Update(b.clock);
  b.humidity = 65;
  b.humidity_available.Update(b.clock);

  ok1(!a.temperature_available);
  ok1(!a.humidity_available);
  a.Complement(b);
  ok1(a.temperature_available);
  ok1(equals(a.temperature.ToCelsius(), 25));
  ok1(a.humidity_available);
  ok1(equals(a.humidity, 65));
}

/**
 * Test that ReadGeoAngle handles NMEA fields without a decimal point
 * gracefully (no crash or undefined behavior).
 */
static void
TestReadGeoAngleNoDot()
{
  NMEAParser parser;
  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  nmea_info.alive.Update(nmea_info.clock);

  /* A malformed GGA with latitude/longitude missing decimal points
     should not crash.  GGA does not update location (that's RMC's job),
     but ReadGeoPoint must handle the missing dots without UB. */
  ok1(parser.ParseLine("$GPGGA,120000,12345,N,12345,E,1,04,1.0,100.0,M,0.0,M,,*45", nmea_info));
  ok1(!nmea_info.location_available);
}

/**
 * Test the GLL (Geographic Position - Latitude/Longitude) parser.
 */
static void
TestGLL()
{
  NMEAParser parser;

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  nmea_info.alive.Update(nmea_info.clock);

  /* first, advance time with an RMC so TimeHasAdvanced works */
  ok1(parser.ParseLine("$GPRMC,082309.00,A,5103.5403,N,00741.5742,E,055.3,022.4,230610,000.3,W*4B",
                        nmea_info));

  /* GLL with valid status — should update location */
  ok1(parser.ParseLine("$GPGLL,5103.5403,N,00741.5742,E,082311.00,A*0E",
                        nmea_info));
  ok1(nmea_info.location_available);
  ok1(equals(nmea_info.location.latitude, 51.059));
  ok1(equals(nmea_info.location.longitude, 7.693));
  ok1(nmea_info.date_time_utc.hour == 8);
  ok1(nmea_info.date_time_utc.minute == 23);
  ok1(nmea_info.date_time_utc.second == 11);
  ok1(nmea_info.gps.real);

  /* GLL with invalid status V — should clear location_available */
  ok1(parser.ParseLine("$GPGLL,5103.5403,N,00741.5742,E,082314.00,V*1C",
                        nmea_info));
  ok1(!nmea_info.location_available);

  /* GLL with valid status again — location restored */
  ok1(parser.ParseLine("$GPGLL,5103.5403,N,00741.5742,E,082317.00,A*08",
                        nmea_info));
  ok1(nmea_info.location_available);
}

/**
 * Test the GSA (GPS DOP and Active Satellites) parser.
 */
static void
TestGSA()
{
  NMEAParser parser;

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  nmea_info.alive.Update(nmea_info.clock);

  /* advance time first */
  ok1(parser.ParseLine("$GPRMC,082318.00,A,5103.5403,N,00741.5742,E,055.3,022.4,230610,000.3,W*4B",
                        nmea_info));

  /* GSA with 3D fix, 12 satellites, and DOP values */
  ok1(parser.ParseLine("$GPGSA,A,3,01,02,03,04,05,06,07,08,09,10,11,12,1.2,0.8,0.9*33",
                        nmea_info));
  ok1(nmea_info.gps.satellite_ids_available);
  ok1(nmea_info.gps.satellite_ids[0] == 1);
  ok1(nmea_info.gps.satellite_ids[5] == 6);
  ok1(nmea_info.gps.satellite_ids[11] == 12);
  ok1(equals(nmea_info.gps.pdop, 1.2));
  ok1(equals(nmea_info.gps.hdop, 0.8));
  ok1(equals(nmea_info.gps.vdop, 0.9));

  /* GSA with no fix (mode 1) — should clear location_available */
  nmea_info.location_available.Update(nmea_info.clock);
  ok1(nmea_info.location_available);
  ok1(parser.ParseLine("$GPGSA,A,1,,,,,,,,,,,,99.9,99.9,99.9*25",
                        nmea_info));
  ok1(!nmea_info.location_available);

  /* GSA with 2D fix and partial satellites */
  ok1(parser.ParseLine("$GPGSA,M,2,01,02,03,,,,,,,,,,2.5,1.3,2.1*39",
                        nmea_info));
  ok1(nmea_info.gps.satellite_ids[0] == 1);
  ok1(nmea_info.gps.satellite_ids[1] == 2);
  ok1(nmea_info.gps.satellite_ids[2] == 3);
  ok1(nmea_info.gps.satellite_ids[3] == 0);
  ok1(equals(nmea_info.gps.pdop, 2.5));
  ok1(equals(nmea_info.gps.hdop, 1.3));
  ok1(equals(nmea_info.gps.vdop, 2.1));
}

/**
 * Test parser robustness with malformed, truncated, and edge-case
 * NMEA sentences.
 */
static void
TestMalformedInput()
{
  NMEAParser parser;

  NMEAInfo nmea_info;
  nmea_info.Reset();
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  nmea_info.alive.Update(nmea_info.clock);

  /* sentences that don't start with $ are rejected */
  ok1(!parser.ParseLine("GPRMC,082310,A,5103.5403,N,00741.5742,E,055.3,022.4,230610,000.3,W*6C",
                         nmea_info));

  /* empty string */
  ok1(!parser.ParseLine("", nmea_info));

  /* just a dollar sign */
  ok1(!parser.ParseLine("$", nmea_info));

  /* too short sentence type (less than 6 chars) */
  ok1(!parser.ParseLine("$GP*00", nmea_info));

  /* bad checksum — should be rejected */
  ok1(!parser.ParseLine("$HCHDM,182.7,M*26", nmea_info));

  /* missing checksum entirely — ParseLine should reject */
  ok1(!parser.ParseLine("$GPRMC,082321.00,A,5103.5403,N", nmea_info));

  /* RMC with all empty fields (except V status) */
  ok1(parser.ParseLine("$GPRMC,,V,,,,,,,,,*31", nmea_info));
  ok1(!nmea_info.location_available);

  /* advance time so subsequent sentences can be parsed */
  ok1(parser.ParseLine("$GPRMC,082322.00,A,5103.5403,N,00741.5742,E,055.3,022.4,230610,000.3,W*42",
                        nmea_info));
  ok1(nmea_info.location_available);

  /* GGA with fix quality 0 (no fix) and empty position */
  ok1(parser.ParseLine("$GPGGA,082323.00,,,,,0,00,,,M,,M,,*40",
                        nmea_info));

  /* GGA with extreme altitude — should still parse */
  ok1(parser.ParseLine("$GPGGA,082324.00,5103.5403,N,00741.5742,E,1,04,1.0,99999.0,M,47.0,M,,*6F",
                        nmea_info));
  ok1(nmea_info.gps_altitude_available);
  ok1(equals(nmea_info.gps_altitude, 99999.0));

  /* RMC with extreme ground speed — should parse without crash */
  ok1(parser.ParseLine("$GPRMC,082325.00,A,5103.5403,N,00741.5742,E,99999.9,022.4,230610,000.3,W*46",
                        nmea_info));
  ok1(nmea_info.ground_speed_available);

  /* RMC with V status — location should be cleared */
  ok1(parser.ParseLine("$GPRMC,082326.00,V,,,,,,,230610,,*14",
                        nmea_info));
  ok1(!nmea_info.location_available);

  /* MWV with missing wind speed — should not crash */
  ok1(!parser.ParseLine("$WIMWV,12.1,T,,M,A*3A", nmea_info));

  /* MWV with missing bearing — should not crash */
  ok1(!parser.ParseLine("$WIMWV,,T,10.1,M,A*38", nmea_info));

  /* HDM with empty heading — should not crash, clears heading */
  ok1(parser.ParseLine("$HCHDM,,M*07", nmea_info));
  ok1(!nmea_info.attitude.heading_available);

  /* GSA with completely empty fields — should not crash */
  ok1(parser.ParseLine("$GPGSA,,,,,,,,,,,,,,,,,*6E", nmea_info));
}

int main()
{
  plan_tests(1032 /* drivers */ + 21 /* PFLAU extended */
             + 26 /* PFLAA v7+ */
             + 106 /* LXNav protocol 1.05 */
             + 8 /* SubSecond */ + 4 /* MWVStatus */
             + 5 /* MWVRelativeTrue */ + 4 /* StallRatio */
             + 12 /* TempHumidityValidity */ + 2 /* ReadGeoAngleNoDot */
             + 13 /* GLL */ + 20 /* GSA */ + 23 /* MalformedInput */);
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
  TestLarus();
  TestLeonardo();
  TestLevilAHRS();
  TestLX(lx_driver);
  TestLX(condor_driver, true, true);
  TestLX(condor3_driver, true, false);
  TestLXEos();
  TestLXV7();
  TestLXV7POLAR();
  TestLXRadioTransponder();
  TestLXNavDeclare();
  TestILEC();
  TestOpenVario();
  TestVega();
  TestWesterboer();
  TestZander();
  TestFlyNet();
  TestVaulter();
  TestXCTracer();
  TestACD();
  TestXCVario();
  TestLoEFGREN();

  /* XXX the Triadis drivers have too many dependencies, not enabling
     for now */
  //TestDeclare(altair_pro_driver);
  TestDeclare(cai302_driver);
  TestDeclare(ew_driver);
  TestDeclare(ew_microrecorder_driver);
  TestDeclare(posigraph_driver);
  TestDeclare(lx_driver);
  TestDeclare(lx_eos_driver);
  TestDeclare(imi_driver);
  TestDeclare(flarm_driver);
  //TestDeclare(vega_driver);

  /* XXX Volkslogger doesn't do well with this test case */
  //TestDeclare(volkslogger_driver);

  TestFlightList(cai302_driver);
  TestFlightList(lx_driver);
  TestFlightList(lx_eos_driver);
  TestFlightList(imi_driver);

  TestSubSecondPrecision();
  TestMWVStatus();
  TestMWVRelativeTrue();
  TestStallRatioComplement();
  TestTemperatureHumidityValidity();
  TestReadGeoAngleNoDot();
  TestGLL();
  TestGSA();
  TestMalformedInput();

  return exit_status();
}
