// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Converter.hpp"

#ifdef HAVE_HTTP
#include "NOTAM.hpp"
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

#include <memory>

namespace NOTAMConverter {

GeoPoint
NOTAMPointToGeoPoint(const NOTAMPoint &point)
{
  return GeoPoint(Angle::Degrees(point.longitude), Angle::Degrees(point.latitude));
}

static AirspaceClass
DetermineAirspaceClass(const struct NOTAM &notam)
{
  // Map NOTAM classifications to airspace classes
  const std::string &classification = notam.classification;
  
  // Common NOTAM types and their typical airspace classifications
  if (classification.find("RESTRICTED") != std::string::npos ||
      classification.find("R") == 0) {
    return RESTRICTED;
  } else if (classification.find("PROHIBITED") != std::string::npos ||
             classification.find("P") == 0) {
    return PROHIBITED;
  } else if (classification.find("DANGER") != std::string::npos ||
             classification.find("D") == 0) {
    return DANGER;
  } else if (classification.find("TMA") != std::string::npos) {
    return TMA;
  } else if (classification.find("CTR") != std::string::npos) {
    return CTR;
  } else if (classification.find("TEMPORARY") != std::string::npos ||
             classification.find("TMP") != std::string::npos) {
    return OTHER; // Use OTHER for temporary restrictions
  }
  
  // Default to OTHER for unrecognized types
  return OTHER;
}

bool 
ConvertNOTAMToAirspace(const struct NOTAM &notam, Airspaces &airspaces)
{
  try {
    std::unique_ptr<AbstractAirspace> airspace;
    
    // Create airspace based on geometry type
    switch (notam.geometry.type) {
      case NOTAM::NOTAMGeometry::POINT:
        // Treat points as small circles (500m radius)
        airspace = std::make_unique<AirspaceCircle>(
          NOTAMPointToGeoPoint(notam.geometry.center),
          500.0
        );
        break;
        
      case NOTAM::NOTAMGeometry::CIRCLE:
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
      top.altitude = 15000; // Default ceiling 15km MSL
      top.reference = AltitudeReference::MSL;
    }
    
    // Set airspace properties with all required parameters
    std::string name = notam.number.empty() ? ("NOTAM " + notam.id) : notam.number;
    airspace->SetProperties(
      std::move(name), // name
      "", // station_name
      TransponderCode(), // transponder_code
      DetermineAirspaceClass(notam), // class
      DetermineAirspaceClass(notam), // type
      base, // base altitude
      top // top altitude
    );
    
    // Set radio frequency if available (extract from text/source)
    // TODO: Parse radio frequency from NOTAM text if available
    
    // Add to airspace database
    airspaces.Add(std::move(airspace));
    
    return true;
    
  } catch (const std::exception &) {
    return false;
  }
}

unsigned
ConvertNOTAMsToAirspaces(const std::vector<struct NOTAM> &notams, Airspaces &airspaces)
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