/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_CIRCLING_INFO_HPP
#define XCSOAR_CIRCLING_INFO_HPP

#include "Engine/Navigation/GeoPoint.hpp"
#include "Math/fixed.hpp"

/**
 * Enumeration for cruise/circling mode detection
 * 
 */
typedef enum {
  CRUISE= 0,                    /**< Established cruise mode */
  WAITCLIMB,                    /**< In cruise, pending transition to climb */
  CLIMB,                        /**< Established climb mode */
  WAITCRUISE                    /**< In climb, pending transition to cruise */
} CirclingMode_t;

/**
 * Data for tracking of climb/cruise mode and transition points
 * 
 */
struct CIRCLING_INFO
{
  /** Turn rate based on track */
  fixed TurnRate;

  /** Turn rate based on heading (including wind) */
  fixed TurnRateWind;

  /** Turn rate after low pass filter */
  fixed SmoothedTurnRate;

  /** StartLocation of the current/last climb */
  GeoPoint ClimbStartLocation;
  /** StartAltitude of the current/last climb */
  fixed ClimbStartAlt;
  /** StartTime of the current/last climb */
  fixed ClimbStartTime;

  /** StartLocation of the current/last cruise */
  GeoPoint CruiseStartLocation;
  /** StartAltitude of the current/last cruise */
  fixed CruiseStartAlt;
  /** StartTime of the current/last cruise */
  fixed CruiseStartTime;

  /** Start/End time of the turn (used for flight mode determination) */
  fixed TurnStartTime;
  /** Start/End location of the turn (used for flight mode determination) */
  GeoPoint TurnStartLocation;
  /** Start/End altitude of the turn (used for flight mode determination) */
  fixed TurnStartAltitude;
  /** Start/End energy height of the turn (used for flight mode determination) */
  fixed TurnStartEnergyHeight;

  /** Current TurnMode (Cruise, Climb or somewhere between) */
  CirclingMode_t TurnMode;

  /** True if in circling mode, False otherwise */
  bool Circling;

  /** Circling/Cruise ratio in percent */
  fixed PercentCircling;

  /** Time spent in cruise mode */
  fixed timeCruising;
  /** Time spent in circling mode */
  fixed timeCircling;

  /** Minimum altitude since start of task */
  fixed MinAltitude;

  /** Maximum height gain (from MinAltitude) during task */
  fixed MaxHeightGain;

  /** Total height climbed during task */
  fixed TotalHeightClimb;

  void ClearPartial();
  void Clear();
};

#endif
