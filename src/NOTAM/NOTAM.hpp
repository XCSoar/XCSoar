// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceAltitude.hpp"
#include <chrono>
#include <vector>
#include <string>

/**
 * Simple geographic point for NOTAM geometry
 */
struct NOTAMPoint {
  double longitude; ///< Longitude in degrees
  double latitude;  ///< Latitude in degrees
};

/**
 * Represents a single NOTAM (Notice to Airmen)
 */
struct NOTAM {
  /** NOTAM unique identifier */
  std::string id;
  
  /** NOTAM number (e.g., A1234/24) */
  std::string number;
  
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
  
  /** Source of the NOTAM */
  std::string source;
  
  /** ICAO location code (e.g., EDGG) */
  std::string location;
  
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
  bool AffectsLocation(double longitude, double latitude, double altitude_meters = -1.0) const noexcept;
};