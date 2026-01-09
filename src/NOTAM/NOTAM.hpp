// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceAltitude.hpp"
#include <chrono>
#include <vector>
#include <string>

namespace NOTAMTime {

inline std::chrono::system_clock::time_point
PermanentEndTime() noexcept
{
  // 2100-01-01T00:00:00Z, far-future sentinel for PERM without overflow risk
  return std::chrono::system_clock::time_point{
    std::chrono::seconds{4102444800LL}
  };
}

} // namespace NOTAMTime

/**
 * Simple geographic point for NOTAM geometry
 */
struct NOTAMPoint {
  double longitude = 0.0; ///< Longitude in degrees
  double latitude = 0.0;  ///< Latitude in degrees
};

/**
 * Represents a single NOTAM (Notice to Airmen)
 */
struct NOTAM {
  /** NOTAM unique identifier */
  std::string id;

  /** Timestamp of last update (ISO8601 string from API)
  * Kept as ISO8601 string since it serves as a unique
  * identifier for delta updates */
  std::string last_updated;
  
  /** NOTAM number (e.g., A1234/24) */
  std::string number;
  
  /** NOTAM series (e.g., F, M, B, W) */
  std::string series;
  
  /** NOTAM type (R=Replace, N=New, C=Cancel) */
  std::string type;
  
  /** Detailed NOTAM text */
  std::string text;
  
  /** Start time of the NOTAM validity */
  std::chrono::system_clock::time_point start_time;
  
  /** End time of the NOTAM validity */
  std::chrono::system_clock::time_point end_time;
  
  /** Geographic area affected by the NOTAM */
  struct NOTAMGeometry {
    enum Type {
      POINT,
      CIRCLE,
      POLYGON,
      COUNT
    } type = POINT;
    
    /** Center point (for POINT and CIRCLE) or first point (for POLYGON) */
    NOTAMPoint center;
    
    /** Radius (only for CIRCLE type) */
    double radius_meters = 0.0;
    
    /** Additional points for polygon */
    std::vector<NOTAMPoint> polygon_points;
  } geometry;
  
  /** Lower altitude limit */
  AirspaceAltitude lower_altitude;
  
  /** Upper altitude limit */  
  AirspaceAltitude upper_altitude;
  
  /** NOTAM classification/type */
  std::string classification;
  
  /** Feature type (e.g., AIRSPACE, OBST, NAV, COM, MILITARY) */
  std::string feature_type;
  
  /** Minimum flight level (000-999) */
  std::string minimum_fl;
  
  /** Maximum flight level (000-999) */
  std::string maximum_fl;
  
  /** Source of the NOTAM */
  std::string source;
  
  /** ICAO location code (e.g., EDGG) */
  std::string location;
  
  /** Traffic type: I=IFR only, V=VFR only, IV=IFR and VFR */
  std::string traffic;
  
  /**
   * Check if this NOTAM is currently active
   */
  bool IsActive() const noexcept {
    const auto now = std::chrono::system_clock::now();
    return now >= start_time && now <= end_time;
  }
  
  /**
   * Check if this NOTAM affects the given location and altitude
   */
  bool AffectsLocation(double longitude, double latitude,
                       double altitude_meters = -1.0) const noexcept;
};
