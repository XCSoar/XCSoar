// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/RangeEstimate.hpp"
#include "FLARM/Traffic.hpp"
#include "NMEA/Info.hpp"
#include "Math/Angle.hpp"
#include "io/BufferedLineReader.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/MemoryReader.hxx"
#include "io/StringOutputStream.hxx"
#include "util/SpanCast.hxx"
#include "TestUtil.hpp"

#include <string>

static void
TestSectorIndex()
{
  ok1(FlarmRangeEstimate::SectorIndex(Angle::Degrees(0)) == 0);
  ok1(FlarmRangeEstimate::SectorIndex(Angle::Degrees(17.9)) == 0);
  ok1(FlarmRangeEstimate::SectorIndex(Angle::Degrees(18)) == 1);
  ok1(FlarmRangeEstimate::SectorIndex(Angle::Degrees(180)) == 10);
  ok1(FlarmRangeEstimate::SectorIndex(Angle::Degrees(359.9)) == 19);
  ok1(FlarmRangeEstimate::SectorIndex(Angle::Degrees(-9)) == 19);
  ok1(FlarmRangeEstimate::SectorIndex(Angle::Degrees(369)) == 0);
}

static void
TestSector()
{
  FlarmRangeEstimate::Sector sector;
  ok1(!sector.Percentile(0.9));

  for (unsigned i = 0; i < 9; ++i)
    sector.Add(1000);
  sector.Add(5000);

  ok1(sector.count == 10);
  ok1(sector.maximum == 5000);

  /* 9 of 10 reports lie in the bin 1000..1250 m */
  ok1(sector.Percentile(0.9) == 1250u);

  /* never beyond the largest distance received */
  ok1(sector.Percentile(1) == 5000u);

  /* beyond the last bin */
  sector.Add(40000);
  ok1(sector.maximum == 40000);
  ok1(sector.histogram[FlarmRangeEstimate::BINS - 1] == 1);
}

static FlarmTraffic &
AddTraffic(NMEAInfo &basic, uint32_t id, double north, double east)
{
  FlarmTraffic &traffic = basic.flarm.traffic.list.append();
  traffic.Clear();
  traffic.id = FlarmId::FromValue(id);
  traffic.source = FlarmTraffic::SourceType::FLARM;
  /* Clear() leaves it alone; the parser always sets it */
  traffic.stealth = false;
  traffic.relative_north = north;
  traffic.relative_east = east;
  traffic.valid.Update(basic.clock);
  return traffic;
}

static void
TestEstimator()
{
  FlarmRangeEstimate estimate;
  FlarmRangeEstimator estimator;

  NMEAInfo basic;
  basic.Reset();
  basic.clock = TimeStamp{FloatDuration{100}};
  basic.track = Angle::Degrees(90);
  basic.track_available.Update(basic.clock);

  /* 1 km east, straight ahead when flying east */
  FlarmTraffic &ahead = AddTraffic(basic, 0xDD1234, 0, 1000);

  /* behind, but from ADS-B, stealth or no-track: not counted */
  AddTraffic(basic, 0x400001, 0, -2000).source =
    FlarmTraffic::SourceType::ADSB;
  AddTraffic(basic, 0xDD0002, 0, -2000).stealth = true;
  AddTraffic(basic, 0xDD0003, 0, -2000).no_track = true;

  /* an ICAO address without the PFLAA Source field may be ADS-B */
  AddTraffic(basic, 0x400002, 0, -2000).id_type =
    FlarmTraffic::IdType::ICAO;

  /* clamped: beyond what the relative position can say */
  AddTraffic(basic, 0xDD0004, 0, 32767);

  estimator.Process(estimate, basic, true);
  ok1(estimate.GetCount() == 1);
  ok1(estimate.sectors[0].count == 1);
  ok1(estimate.sectors[0].maximum == 1000);

  /* the same report again: counted once */
  estimator.Process(estimate, basic, true);
  ok1(estimate.GetCount() == 1);

  /* a new report from the same target, now to the right */
  basic.clock = TimeStamp{FloatDuration{101}};
  ahead.relative_north = -3000;
  ahead.relative_east = 0;
  ahead.valid.Update(basic.clock);
  estimator.Process(estimate, basic, true);
  ok1(estimate.GetCount() == 2);
  ok1(estimate.sectors[FlarmRangeEstimate::SectorIndex(Angle::Degrees(90))]
      .maximum == 3000);

  /* on the ground: seen, but not counted, and not counted later */
  basic.clock = TimeStamp{FloatDuration{102}};
  ahead.valid.Update(basic.clock);
  estimator.Process(estimate, basic, false);
  estimator.Process(estimate, basic, true);
  ok1(estimate.GetCount() == 2);

  /* an ICAO address the device says came from its FLARM radio */
  basic.clock = TimeStamp{FloatDuration{102.5}};
  FlarmTraffic &icao = AddTraffic(basic, 0x400003, 0, 500);
  icao.id_type = FlarmTraffic::IdType::ICAO;
  icao.source_received = true;
  estimator.Process(estimate, basic, true);
  ok1(estimate.GetCount() == 3);

  /* without a ground track, the bearing is unknown */
  basic.clock = TimeStamp{FloatDuration{103}};
  ahead.valid.Update(basic.clock);
  basic.track_available.Clear();
  estimator.Process(estimate, basic, true);
  ok1(estimate.GetCount() == 3);
}

static void
TestSaveLoad()
{
  FlarmRangeEstimate estimate;
  estimate.Add(Angle::Degrees(10), 1000, std::nullopt);
  estimate.Add(Angle::Degrees(10), 4000,
               FlarmRangeEstimate::TimePoint{std::chrono::seconds{1700000000}});
  estimate.Add(Angle::Degrees(200), 26000,
               FlarmRangeEstimate::TimePoint{std::chrono::seconds{1700003600}});

  StringOutputStream sos;
  {
    BufferedOutputStream bos{sos};
    SaveFlarmRangeEstimate(estimate, bos);
    bos.Flush();
  }

  MemoryReader reader{AsBytes(std::string_view{sos.GetValue()})};
  BufferedLineReader line_reader{reader};
  FlarmRangeEstimate loaded;
  LoadFlarmRangeEstimate(loaded, line_reader);

  ok1(loaded.GetCount() == 3);
  ok1(loaded.sectors[0].count == 2 && loaded.sectors[0].maximum == 4000);
  ok1(loaded.sectors[0].histogram == estimate.sectors[0].histogram);
  ok1(loaded.sectors[11].maximum == 26000);
  ok1(loaded.first == estimate.first && loaded.last == estimate.last);

  /* a count that does not match the histogram, and a number beyond
     32 bits */
  std::string text = "sector 3 999 1000";
  for (unsigned i = 0; i < FlarmRangeEstimate::BINS; ++i)
    text += i == 4 ? " 2" : " 0";
  text += "\nsector 4 1 1000 4294967296";
  for (unsigned i = 1; i < FlarmRangeEstimate::BINS; ++i)
    text += " 0";
  text += "\n";

  MemoryReader bad_reader{AsBytes(std::string_view{text})};
  BufferedLineReader bad_line_reader{bad_reader};
  LoadFlarmRangeEstimate(loaded, bad_line_reader);
  ok1(loaded.sectors[3].count == 2);
  ok1(loaded.sectors[4].count == 0);
}

int
main()
{
  plan_tests(7 + 7 + 9 + 7);

  TestSectorIndex();
  TestSector();
  TestEstimator();
  TestSaveLoad();

  return exit_status();
}
