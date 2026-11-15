// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Renderer/HorizonRenderer.hpp"
#include "TestUtil.hpp"
#include "util/StringAPI.hxx"
#include "time/Stamp.hpp"

static MoreData
Sample() noexcept
{
  MoreData data{};
  data.clock = TimeStamp{FloatDuration{1}};
  return data;
}

static HorizonRenderer::HorizonReadout
Read(const MoreData &basic, const TerrainInfo &terrain,
     HorizonSettings::Speed speed = HorizonSettings::Speed::INDICATED,
     HorizonSettings::Course course = HorizonSettings::Course::HEADING,
     HorizonSettings::Altitude altitude =
       HorizonSettings::Altitude::ABOVE_GROUND) noexcept
{
  HorizonSettings settings;
  settings.SetDefaults();
  settings.speed = speed;
  settings.course = course;
  settings.altitude = altitude;
  return HorizonRenderer::Readout(basic, terrain, settings);
}

static bool
Title(const char *title, const char *expected) noexcept
{
  return title != nullptr && StringIsEqual(title, expected);
}

int
main()
{
  plan_tests(36);

  Units::current.speed_unit = Unit::KILOMETER_PER_HOUR;
  Units::current.altitude_unit = Unit::METER;

  TerrainInfo no_terrain{};
  no_terrain.altitude_agl_valid = false;

  TerrainInfo terrain = no_terrain;
  terrain.altitude_agl_valid = true;
  terrain.altitude_agl = 700;

  MoreData both = Sample();
  both.airspeed_available.Update(both.clock);
  both.indicated_airspeed = 25;
  both.true_airspeed = 30;
  both.ground_speed_available.Update(both.clock);
  both.ground_speed = 10;
  both.gps_altitude_available.Update(both.clock);
  both.gps_altitude = 1500;
  both.baro_altitude_available.Update(both.clock);
  both.baro_altitude = 1450;
  both.track_available.Update(both.clock);
  both.track = Angle::Degrees(80);
  both.attitude.heading_available.Update(both.clock);
  both.attitude.heading = Angle::Degrees(-30);

  /* the defaults are those of an attitude indicator */
  const auto def = Read(both, terrain);
  ok1(def.speed && equals(*def.speed, 90));
  ok1(Title(def.speed_title, "IAS"));
  ok1(equals(def.speed_step, 10));
  ok1(def.altitude && equals(*def.altitude, 700));
  ok1(Title(def.altitude_title, "AGL"));
  ok1(equals(def.altitude_step, 50));
  ok1(def.course && equals(*def.course, 330));
  ok1(Title(def.course_title, "HDG"));

  /* the choices of the settings */
  const auto tas = Read(both, terrain, HorizonSettings::Speed::TRUE_AIRSPEED);
  ok1(tas.speed && equals(*tas.speed, 108));
  ok1(Title(tas.speed_title, "TAS"));

  const auto gs = Read(both, terrain, HorizonSettings::Speed::GROUND);
  ok1(gs.speed && equals(*gs.speed, 36));
  ok1(Title(gs.speed_title, "GS"));

  const auto trk = Read(both, terrain, HorizonSettings::Speed::INDICATED,
                        HorizonSettings::Course::TRACK);
  ok1(trk.course && equals(*trk.course, 80));
  ok1(Title(trk.course_title, "TRK"));

  const auto baro = Read(both, terrain, HorizonSettings::Speed::INDICATED,
                         HorizonSettings::Course::HEADING,
                         HorizonSettings::Altitude::BARO);
  ok1(baro.altitude && equals(*baro.altitude, 1450));
  ok1(Title(baro.altitude_title, "Baro"));

  const auto gps = Read(both, terrain, HorizonSettings::Speed::INDICATED,
                        HorizonSettings::Course::HEADING,
                        HorizonSettings::Altitude::GPS);
  ok1(gps.altitude && equals(*gps.altitude, 1500));
  ok1(Title(gps.altitude_title, "GPS"));

  /* a missing value stays missing, nothing else takes its place */
  MoreData ground = Sample();
  ground.ground_speed_available.Update(ground.clock);
  ground.ground_speed = 10;
  ground.gps_altitude_available.Update(ground.clock);
  ground.gps_altitude = 1500;
  ground.track_available.Update(ground.clock);
  ground.track = Angle::Degrees(80);
  const auto missing = Read(ground, no_terrain);
  ok1(!missing.speed);
  ok1(!missing.altitude);
  ok1(!missing.course);
  ok1(Title(missing.speed_title, "IAS"));

  const auto no_baro = Read(ground, no_terrain,
                            HorizonSettings::Speed::TRUE_AIRSPEED,
                            HorizonSettings::Course::HEADING,
                            HorizonSettings::Altitude::BARO);
  ok1(!no_baro.speed);
  ok1(!no_baro.altitude);

  const auto empty = Read(Sample(), no_terrain,
                          HorizonSettings::Speed::GROUND,
                          HorizonSettings::Course::TRACK,
                          HorizonSettings::Altitude::GPS);
  ok1(!empty.speed);
  ok1(!empty.altitude);
  ok1(!empty.course);

  Units::current.altitude_unit = Unit::FEET;
  const auto feet = Read(both, terrain);
  ok1(feet.altitude &&
      equals(*feet.altitude, Units::ToUserAltitude(700)));
  ok1(equals(feet.altitude_step, 100));

  Units::current.altitude_unit = Unit::FLIGHT_LEVEL;
  const auto level = Read(both, terrain);
  ok1(equals(level.altitude_step, 1));

  Units::current.speed_unit = Unit::METER_PER_SECOND;
  const auto ms = Read(both, terrain);
  ok1(ms.speed && equals(*ms.speed, 25));
  ok1(equals(ms.speed_step, 5));

  Units::current.speed_unit = Unit::KNOTS;
  const auto kt = Read(both, terrain);
  ok1(kt.speed && equals(*kt.speed, Units::ToUserSpeed(25)));
  ok1(equals(kt.speed_step, 10));

  const auto kt_gs = Read(both, terrain, HorizonSettings::Speed::GROUND);
  ok1(kt_gs.speed && equals(*kt_gs.speed, Units::ToUserSpeed(10)));

  Units::current.altitude_unit = Unit::METER;
  const auto gps_m = Read(both, no_terrain, HorizonSettings::Speed::INDICATED,
                          HorizonSettings::Course::HEADING,
                          HorizonSettings::Altitude::GPS);
  ok1(gps_m.altitude && equals(*gps_m.altitude, 1500));

  return exit_status();
}
