// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAM/Client.hpp"
#include "NOTAM/Filter.hpp"
#include "NOTAM/NOTAMCache.hpp"
#include "NOTAM/Settings.hpp"
#include "Geo/AltitudeReference.hpp"
#include "Units/System.hpp"
#include "TestUtil.hpp"

#include <boost/json.hpp>
#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace std::chrono;

static std::string
MakeFeature(std::string_view properties_json,
            std::string_view geometry_json)
{
  return std::string("{\"type\":\"Feature\",\"properties\":{\"coreNOTAMData\":"
                     "{\"notam\":") +
    std::string(properties_json) +
    "}},\"geometry\":" +
    std::string(geometry_json) +
    "}";
}

static std::string
WrapItems(std::string_view items_json)
{
  return std::string("{\"items\":[") + std::string(items_json) + "]}";
}

static bool
ThrowsParse(std::string_view json)
{
  try {
    (void)NOTAMClient::ParseNOTAMGeoJSON(std::string(json));
    return false;
  } catch (const std::exception &) {
    return true;
  }
}

static bool
ThrowsResponseParse(std::string_view json)
{
  try {
    (void)NOTAMClient::ParseNOTAMResponse(boost::json::parse(json));
    return false;
  } catch (const std::exception &) {
    return true;
  }
}

static bool
MetadataValid(std::string_view json)
{
  const auto value = boost::json::parse(json);
  return NOTAMCache::ExtractMetadata(value.as_object()).valid;
}

int
main()
{
  plan_tests(44);

  const auto now = system_clock::now();

  {
    const auto notams = NOTAMClient::ParseNOTAMGeoJSON(WrapItems(MakeFeature(
      R"({"id":"n1","lastUpdated":"2026-04-09T10:00:00Z","number":"A1234/26","text":"Line 1","effectiveStart":"2026-04-09T08:00:00Z","effectiveEnd":"2026-04-09T12:00:00Z","lowerLimit":"   ","upperLimit":"","minimumFL":"055","maximumFL":"125","selectionCode":"QMRLC","location":"EDDF","traffic":"I","icaoLocation":"EDGG"})",
      R"({"type":"Point","coordinates":[8.0,50.0]})")));

    ok1(notams.size() == 1 && notams[0].geometry.type ==
        NOTAM::NOTAMGeometry::Type::POINT);
    ok1(notams.size() == 1 && equals(notams[0].geometry.center.longitude, 8.0));
    ok1(notams.size() == 1 && equals(notams[0].geometry.center.latitude, 50.0));
    ok1(notams.size() == 1 && notams[0].lower_altitude.reference ==
        AltitudeReference::STD);
    ok1(notams.size() == 1 &&
        equals(notams[0].lower_altitude.flight_level, 55));
    ok1(notams.size() == 1 && notams[0].upper_altitude.reference ==
        AltitudeReference::STD);
    ok1(notams.size() == 1 &&
        equals(notams[0].upper_altitude.flight_level, 125));
  }

  {
    const auto notams = NOTAMClient::ParseNOTAMGeoJSON(WrapItems(MakeFeature(
      R"({"id":"n1b","number":"A1235/26","text":"Windpark\nmarked","effectiveStart":"2026-04-09T08:00:00Z","effectiveEnd":"2026-04-09T12:00:00Z","minimumFl":"000","maximumFl":"016","traffic":"IV","radius":"001"})",
      R"({"type":"GeometryCollection","geometries":[{"type":"Point","coordinates":[10.95,51.2]}]})")));

    ok1(notams.size() == 1 && notams[0].geometry.type ==
        NOTAM::NOTAMGeometry::Type::CIRCLE);
    ok1(notams.size() == 1 &&
        equals(notams[0].geometry.center.longitude, 10.95));
    ok1(notams.size() == 1 &&
        equals(notams[0].geometry.center.latitude, 51.2));
    ok1(notams.size() == 1 &&
        equals(notams[0].geometry.radius_meters, 1852.0));
    ok1(notams.size() == 1 && notams[0].lower_altitude.reference ==
        AltitudeReference::AGL);
    ok1(notams.size() == 1 &&
        equals(notams[0].upper_altitude.flight_level, 16));
    ok1(notams.size() == 1 &&
        notams[0].text.find('\n') != std::string::npos);
  }

  {
    const auto notams = NOTAMClient::ParseNOTAMGeoJSON(WrapItems(MakeFeature(
      R"({"id":"n1c","number":"A1236/26","effectiveStart":"2026-04-09T08:00:00Z","effectiveEnd":"2026-04-09T12:00:00Z","radius":"002"})",
      R"({"type":"GeometryCollection","geometries":[{"type":"Point","coordinates":[10.0,51.0]},{"type":"Point","coordinates":[11.0,52.0]}]})")));

    ok1(notams.size() == 2);
    ok1(notams.size() == 2 && notams[0].geometry.type ==
        NOTAM::NOTAMGeometry::Type::CIRCLE);
    ok1(notams.size() == 2 && notams[1].geometry.type ==
        NOTAM::NOTAMGeometry::Type::CIRCLE);
    ok1(notams.size() == 2 &&
        equals(notams[0].geometry.center.longitude, 10.0) &&
        equals(notams[1].geometry.center.longitude, 11.0));
    ok1(notams.size() == 2 &&
        equals(notams[0].geometry.radius_meters, 2.0 * 1852.0) &&
        equals(notams[1].geometry.radius_meters, 2.0 * 1852.0));
  }

  {
    const auto notams = NOTAMClient::ParseNOTAMGeoJSON(WrapItems(MakeFeature(
      R"({"id":"n2","number":"B2345/26","effectiveStart":"2026-04-09T08:00:00Z","effectiveEnd":"2026-04-09T12:00:00Z","lowerLimit":"4500FT MSL","upperLimit":"FL095","minimumFL":"055","maximumFL":"125"})",
      R"({"type":"Point","coordinates":[7.0,49.0]})")));

    ok1(notams.size() == 1 &&
        notams[0].lower_altitude.reference == AltitudeReference::MSL);
    ok1(notams.size() == 1 &&
        equals(notams[0].lower_altitude.altitude,
               Units::ToSysUnit(4500, Unit::FEET)));
    ok1(notams.size() == 1 &&
        notams[0].upper_altitude.reference == AltitudeReference::STD);
    ok1(notams.size() == 1 &&
        equals(notams[0].upper_altitude.flight_level, 95));
  }

  {
    const auto notams = NOTAMClient::ParseNOTAMGeoJSON(WrapItems(
      MakeFeature(
        R"({"id":"n3","number":"C3456/26","effectiveStart":"2026-04-09T08:00:00Z","effectiveEnd":"2026-04-09T12:00:00Z","radius":"3.5"})",
        R"({"type":"Point","coordinates":[6.5,48.5]})") +
      ",{\"bad\":1}"));

    ok1(notams.size() == 1 && notams[0].geometry.type ==
        NOTAM::NOTAMGeometry::Type::CIRCLE);
    ok1(notams.size() == 1 &&
        equals(notams[0].geometry.radius_meters, 3.5 * 1852.0));
  }

  ok1(ThrowsParse(R"({"items":[{"bad":1}]})"));
  ok1(ThrowsResponseParse(
    R"({"delta":true,"removedIds":"broken","items":[]})"));
  ok1(ThrowsResponseParse(
    R"({"delta":true,"removedIds":["ok",7],"items":[]})"));
  ok1(ThrowsParse(WrapItems(MakeFeature(
    R"({"id":"bad-date","number":"E5678/26","effectiveStart":"2026-02-31T08:00:00Z","effectiveEnd":"2026-04-09T12:00:00Z"})",
    R"({"type":"Point","coordinates":[8.0,50.0]})"))));
  ok1(ThrowsParse(WrapItems(MakeFeature(
    R"({"id":"bad-suffix","number":"E5679/26","effectiveStart":"2026-04-09T08:00:00+02:00","effectiveEnd":"2026-04-09T12:00:00Z"})",
    R"({"type":"Point","coordinates":[8.0,50.0]})"))));
  ok1(!ThrowsParse(WrapItems(MakeFeature(
    R"({"id":"fractional","number":"E5680/26","effectiveStart":"2026-04-09T08:00:00.789Z","effectiveEnd":"2026-04-09T12:00:00.123Z"})",
    R"({"type":"Point","coordinates":[8.0,50.0]})"))));
  ok1(ThrowsParse(WrapItems(MakeFeature(
    R"({"id":"bad-perm-start","number":"E5681/26","effectiveStart":"PERM","effectiveEnd":"2026-04-09T12:00:00Z"})",
    R"({"type":"Point","coordinates":[8.0,50.0]})"))));
  ok1(!ThrowsParse(WrapItems(MakeFeature(
    R"({"id":"perm-end","number":"E5682/26","effectiveStart":"2026-04-09T08:00:00Z","effectiveEnd":"PERM"})",
    R"({"type":"Point","coordinates":[8.0,50.0]})"))));
  ok1(ThrowsParse(WrapItems(MakeFeature(
    R"({"id":"bad-polygon","number":"E5683/26","effectiveStart":"2026-04-09T08:00:00Z","effectiveEnd":"2026-04-09T12:00:00Z"})",
    R"({"type":"Polygon","coordinates":[[[8.0,50.0],[8.1,50.1],[8.0,50.0]]]})"))));
  ok1(MetadataValid(R"({"xcsoar_timestamp":1712664000,"xcsoar_location_lat":50,"xcsoar_location_lon":8,"xcsoar_radius_km":25})"));
  ok1(!MetadataValid(R"({"xcsoar_timestamp":1712664000})"));

  {
    NOTAMSettings settings;
    settings.show_ifr = false;
    settings.show_only_effective = true;
    settings.max_radius_m = 5000;
    settings.hidden_qcodes = "QMR";

    NOTAM notam;
    notam.number = "D4567/26";
    notam.traffic = "I";
    notam.start_time = now - hours(1);
    notam.end_time = now + hours(1);
    notam.feature_type = "QMRLC";
    notam.geometry.radius_meters = 1000;

    ok1(!NOTAMFilter::ShouldDisplay(notam, settings, now, false));

    notam.feature_type = "QWERT";
    ok1(!NOTAMFilter::ShouldDisplay(notam, settings, now, false));

    notam.traffic = "V";
    ok1(NOTAMFilter::ShouldDisplay(notam, settings, now, false));

    notam.traffic = "IV";
    ok1(NOTAMFilter::ShouldDisplay(notam, settings, now, false));

    notam.geometry.radius_meters = 6000;
    ok1(!NOTAMFilter::ShouldDisplay(notam, settings, now, false));

    notam.geometry.radius_meters = 1000;
    notam.start_time = now + hours(2);
    notam.end_time = now + hours(4);
    ok1(!NOTAMFilter::ShouldDisplay(notam, settings, now, false));

    notam.start_time = now - hours(1);
    notam.end_time = now + hours(1);
    ok1(NOTAMFilter::ShouldDisplay(notam, settings, now, false));
  }

  {
    NOTAM notam;
    notam.start_time = now - hours(1);
    notam.end_time = NOTAMTime::PermanentEndTime() + seconds(1);

    ok1(notam.IsActive(now));
  }

  return exit_status();
}
