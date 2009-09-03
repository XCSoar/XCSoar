/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#if !defined(XCSOAR_GLIDECOMPUTER_HPP)
#define XCSOAR_GLIDECOMPUTER_HPP

#include "XCSoar.h"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"
#include "FlightStatistics.hpp"
#include "AATDistance.h"
#include "OnLineContest.h"
#include "Audio/VegaVoice.h"
#include "GlideRatio.hpp"
#include "ThermalLocator.h"
#include "windanalyser.h"
#include "SnailTrail.hpp"

class GlideComputer {
public:
  static  ldrotary_s     rotaryLD;
  static  FlightStatistics     flightstats;
  static  AATDistance    aatdistance;
  static  OLCOptimizer   olc;
  static  ThermalLocator thermallocator;
  static  WindAnalyser   *windanalyser;
  static  SnailTrail     snail_trail;

  static void DoLogging(const NMEA_INFO *Basic, DERIVED_INFO *Calculated);

  // CalculationsAutoMc
  static void DoAutoMacCready(const NMEA_INFO *Basic, DERIVED_INFO *Calculated,
			      double mc_setting);

  //protected:
  static  VegaVoice    vegavoice;
  static  DERIVED_INFO Finish_Derived_Info;

};

#endif
