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
#include <cmath>
#include <memory>
#include <optional>

namespace NOTAMConverter {

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
                       Airspaces &airspaces)
{
  auto airspace = BuildNOTAMAirspace(notam);
  if (!airspace)
    return false;

  airspaces.Add(std::move(airspace));
  return true;
}

AirspacePtr
BuildNOTAMAirspace(const struct NOTAM &notam)
{
  try {
    std::unique_ptr<AbstractAirspace> airspace;
    
    // Create airspace based on geometry type
    switch (notam.geometry.type) {
      case NOTAM::NOTAMGeometry::Type::POINT:
        // Treat points as small circles
        if (!notam.geometry.center.Check()) {
          LogFmt("NOTAM: Invalid point coordinates for NOTAM {}/{}, skipping",
                 notam.id.c_str(), notam.number.c_str());
          return {};
        }

        airspace = std::make_unique<AirspaceCircle>(
          notam.geometry.center,
          DEFAULT_POINT_RADIUS_METERS
        );
        break;
        
      case NOTAM::NOTAMGeometry::Type::CIRCLE:
        // Validate radius before creating circle
        if (!std::isfinite(notam.geometry.radius_meters) ||
            notam.geometry.radius_meters <= 0) {
          LogFmt("NOTAM: Invalid circle radius {:.1f} for NOTAM {}/{}, "
                 "skipping",
                 notam.geometry.radius_meters, notam.id.c_str(),
                 notam.number.c_str());
          return {};
        }
        if (notam.geometry.radius_meters > MAX_CIRCLE_RADIUS_METERS) {
          LogFmt("NOTAM: Circle radius {:.1f} exceeds max {:.0f} for "
                 "NOTAM {}/{}, skipping",
                 notam.geometry.radius_meters, MAX_CIRCLE_RADIUS_METERS,
                 notam.id.c_str(), notam.number.c_str());
          return {};
        }
        if (!notam.geometry.center.Check()) {
          LogFmt("NOTAM: Invalid circle coordinates for NOTAM {}/{}, skipping",
                 notam.id.c_str(), notam.number.c_str());
          return {};
        }

        airspace = std::make_unique<AirspaceCircle>(
          notam.geometry.center,
          notam.geometry.radius_meters
        );
        break;
        
      case NOTAM::NOTAMGeometry::Type::POLYGON:
        if (notam.geometry.polygon_points.size() >= 3) {
          for (const auto &point : notam.geometry.polygon_points) {
            if (!point.Check()) {
              LogFmt("NOTAM: Invalid polygon point coordinates for NOTAM {}/{}, "
                     "skipping",
                     notam.id.c_str(), notam.number.c_str());
              return {};
            }
          }

          airspace = std::make_unique<AirspacePolygon>(
            notam.geometry.polygon_points);
        } else {
          LogFmt("NOTAM: Invalid polygon with {} points for NOTAM {}/{}, "
                 "skipping (minimum 3 required)",
                 static_cast<unsigned>(
                   notam.geometry.polygon_points.size()),
                 notam.id.c_str(), notam.number.c_str());
          return {}; // Invalid polygon
        }
        break;
        
      default:
        LogFmt("NOTAM: Unknown geometry type {} for NOTAM {}/{}, skipping",
               static_cast<int>(notam.geometry.type),
               notam.id.c_str(), notam.number.c_str());
        return {}; // Unknown geometry type
    }
    
    if (!airspace) {
      return {};
    }
    
    // Use the altitude objects directly from NOTAM
    AirspaceAltitude base = notam.lower_altitude;
    AirspaceAltitude top = notam.upper_altitude;
    
    // Use default altitudes if the parser left the NOTAM limit unset.
    if (base.altitude == NOTAMAltitude::INVALID_ALTITUDE) {
      base = AirspaceAltitude{};
      base.altitude = 0; // Sea-level fallback for unknown lower limit
      base.reference = AltitudeReference::MSL;
    }
    
    if (top.altitude == NOTAMAltitude::INVALID_ALTITUDE) {
      top = AirspaceAltitude{};
      // Conservative fallback (~FL656) to avoid clipping valid NOTAMs.
      top.altitude = DEFAULT_CONSERVATIVE_FALLBACK_ALTITUDE_M;
      top.reference = AltitudeReference::MSL;
    }
    
    // Keep human-readable NOTAM content in name and stable identifier in
    // station_name for lookups/dialog details.
    std::string name = !notam.text.empty()
      ? notam.text
      : (!notam.number.empty() ? notam.number : ("NOTAM " + notam.id));
    std::string station_name = !notam.number.empty()
      ? notam.number
      : notam.id;
    airspace->SetProperties(
      std::move(name), // name
      std::move(station_name), // station_name
      TransponderCode(), // transponder_code
      AirspaceClass::UNCLASSIFIED, // class
      AirspaceClass::NOTAM, // type
      base, // base altitude
      top // top altitude
    );
    
    // Set radio frequency if available (extract from text/source)
    // TODO: Parse radio frequency from NOTAM text if available?
    
    return AirspacePtr(std::move(airspace));
    
  } catch (const std::exception &e) {
    LogFmt("NOTAM: Failed to convert NOTAM {}/{}: {}",
           notam.id.c_str(), notam.number.c_str(), e.what());
    return {};
  }
}

/**
 * Convert all NOTAMs into airspace objects and append them to the output.
 *
 * @param notams Input list of NOTAMs to convert.
 * @param airspaces Output container receiving converted airspaces.
 * @return Number of NOTAMs successfully converted and added to @p airspaces.
 */
unsigned
ConvertNOTAMsToAirspaces(const std::vector<struct NOTAM> &notams,
                         Airspaces &airspaces)
{
  unsigned count = 0;
  
  for (const auto &notam : notams) {
    if (ConvertNOTAMToAirspace(notam, airspaces)) {
      count++;
    }
  }
  
  return count;
}

} // namespace NOTAMConverter

#endif // HAVE_HTTP
