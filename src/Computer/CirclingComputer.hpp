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

#ifndef XCSOAR_CIRCLING_COMPUTER_HPP
#define XCSOAR_CIRCLING_COMPUTER_HPP

struct CirclingInfo;
struct NMEAInfo;
struct MoreData;
struct DerivedInfo;
struct SETTINGS_COMPUTER;

/**
 * Detect when the aircraft begins or ends circling.
 *
 * Dependencies: #FlyingComputer.
 */
class CirclingComputer {
public:
  /**
   * Calculates the turn rate
   */
  void TurnRate(CirclingInfo &circling_info,
                const NMEAInfo &basic, const NMEAInfo &last_basic,
                const DerivedInfo &calculated,
                const DerivedInfo &last_calculated);

  /**
   * Calculates the turn rate and the derived features.
   * Determines the current flight mode (cruise/circling).
   */
  void Turning(CirclingInfo &circling_info,
               const MoreData &basic, const MoreData &last_basic,
               const DerivedInfo &calculated,
               const DerivedInfo &last_calculated,
               const SETTINGS_COMPUTER &settings_computer);
};

#endif
