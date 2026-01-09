// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Converter.hpp"

#ifdef HAVE_HTTP
#include "NOTAM.hpp"
#include "LogFile.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"
#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Geo/AltitudeReference.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "TransponderCode.hpp"
#include "util/ConvertString.hpp"

#include <memory>

namespace NOTAMConverter {

/** Default radius for NOTAM point geometries */
static constexpr double DEFAULT_POINT_RADIUS_METERS = 500.0;
/** Maximum allowed radius for NOTAM circle geometries */
static constexpr double MAX_CIRCLE_RADIUS_METERS = 1000000.0;

[[gnu::const]]
GeoPoint
NOTAMPointToGeoPoint(const NOTAMPoint &point) noexcept
{
  return GeoPoint(Angle::Degrees(point.longitude),
                  Angle::Degrees(point.latitude));
}

/**
 * Convert a NOTAM record to an airspace object and add it to the container.
 *
 * Converts NOTAM geometry (point/circle/polygon) into airspace shapes,
 * applies default altitudes when missing/invalid, and maps NOTAM metadata to
 * airspace properties. Unsupported geometry types or invalid geometry (e.g.
 * too few polygon points or invalid circle radius) will cause a failure.
 *
 * @param notam Input NOTAM with populated geometry and altitude fields.
 *              Coordinates are expected in degrees (WGS84) for conversion.
 * @param airspaces Output container that takes ownership of the new airspace
 *                  on success.
 * @return true if the NOTAM was converted and added; false on failure.
 */
bool
ConvertNOTAMToAirspace(const struct NOTAM &notam,
                       Airspaces &airspaces) noexcept
{
  try {
    std::unique_ptr<AbstractAirspace> airspace;
    
    // Create airspace based on geometry type
    switch (notam.geometry.type) {
      case NOTAM::NOTAMGeometry::POINT:
        // Treat points as small circles
        airspace = std::make_unique<AirspaceCircle>(
          NOTAMPointToGeoPoint(notam.geometry.center),
          DEFAULT_POINT_RADIUS_METERS
        );
        break;
        
      case NOTAM::NOTAMGeometry::CIRCLE:
        // Validate radius before creating circle
        if (notam.geometry.radius_meters <= 0) {
          LogFormat("NOTAM: Invalid circle radius %.1f for NOTAM %s/%s, "
                    "skipping",
                    notam.geometry.radius_meters, notam.id.c_str(),
                    notam.number.c_str());
          return false;
        }
        if (notam.geometry.radius_meters > MAX_CIRCLE_RADIUS_METERS) {
          LogFormat("NOTAM: Circle radius %.1f exceeds max %.0f for "
                    "NOTAM %s/%s, skipping",
                    notam.geometry.radius_meters, MAX_CIRCLE_RADIUS_METERS,
                    notam.id.c_str(), notam.number.c_str());
          return false;
        }
        airspace = std::make_unique<AirspaceCircle>(
          NOTAMPointToGeoPoint(notam.geometry.center),
          notam.geometry.radius_meters
        );
        break;
        
      case NOTAM::NOTAMGeometry::POLYGON:
        if (notam.geometry.polygon_points.size() >= 3) {
          std::vector<GeoPoint> points;
          points.reserve(notam.geometry.polygon_points.size());
          
          for (const auto &notam_point : notam.geometry.polygon_points) {
            points.push_back(NOTAMPointToGeoPoint(notam_point));
          }
          
          airspace = std::make_unique<AirspacePolygon>(std::move(points));
        } else {
          LogFormat("NOTAM: Invalid polygon with %u points for NOTAM %s, "
                    "skipping (minimum 3 required)",
                    static_cast<unsigned>(
                      notam.geometry.polygon_points.size()),
                    notam.number.c_str());
          return false; // Invalid polygon
        }
        break;
        
      default:
        return false; // Unknown geometry type
    }
    
    if (!airspace) {
      return false;
    }
    
    // Use the altitude objects directly from NOTAM
    AirspaceAltitude base = notam.lower_altitude;
    AirspaceAltitude top = notam.upper_altitude;
    
    // Use default altitudes if invalid (altitude < 0 indicates invalid)
    if (base.altitude < 0) {
      base.altitude = 0; // Ground level
      base.reference = AltitudeReference::MSL;
    }
    
    if (top.altitude < 0) {
      top.altitude = 20000; // Default ceiling 20 km MSL
      top.reference = AltitudeReference::MSL;
    }
    
    // Set airspace properties with all required parameters
    const std::string name_str = notam.number.empty()
      ? ("NOTAM " + notam.id)
      : notam.number;
#ifdef _UNICODE
    // On Windows (Unicode build), convert UTF-8 to wide string
    const UTF8ToWideConverter name_converter(name_str.c_str());
    tstring name;
    if (name_converter.IsValid())
      name = name_converter.c_str();
    else {
      LogFormat("NOTAM: Invalid UTF-8 name for NOTAM %s/%s, using empty name",
                notam.id.c_str(), notam.number.c_str());
      name = _T("");
    }
#else
    // On Unix/Android/iOS, tstring is std::string, use directly
    tstring name = name_str;
#endif
    airspace->SetProperties(
      std::move(name), // name
      _T(""), // station_name
      TransponderCode(), // transponder_code
      UNCLASSIFIED, // class
      NOTAM, // type
      base, // base altitude
      top // top altitude
    );
    
    // Set radio frequency if available (extract from text/source)
    // TODO: Parse radio frequency from NOTAM text if available?
    
    // Add to airspace database
    airspaces.Add(std::move(airspace));
    
    return true;
    
  } catch (const std::exception &e) {
    LogFormat("NOTAM: Failed to convert NOTAM %s: %s",
              notam.number.c_str(), e.what());
    return false;
  }
}

/**
 * Convert all active NOTAMs into airspace objects and append them to the
 * output.
 *
 * @param notams Input list of NOTAMs to consider.
 * @param airspaces Output container receiving converted airspaces.
 * @return Number of NOTAMs successfully converted and added.
 */
unsigned
ConvertNOTAMsToAirspaces(const std::vector<struct NOTAM> &notams,
                         Airspaces &airspaces) noexcept
{
  unsigned count = 0;
  
  for (const auto &notam : notams) {
    // Only convert active NOTAMs
    if (notam.IsActive() && ConvertNOTAMToAirspace(notam, airspaces)) {
      count++;
    }
  }
  
  return count;
}

} // namespace NOTAMConverter

#endif // HAVE_HTTP
