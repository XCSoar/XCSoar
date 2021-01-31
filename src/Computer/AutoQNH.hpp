/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_AUTO_QNH_HPP
#define XCSOAR_AUTO_QNH_HPP

struct NMEAInfo;
struct DerivedInfo;
struct ComputerSettings;
class Waypoints;

class AutoQNH {
  const unsigned QNH_TIME;

  unsigned countdown_autoqnh;

public:
  constexpr AutoQNH(const unsigned qnh_time = 10)
    : QNH_TIME(qnh_time), countdown_autoqnh(qnh_time)
  {};

  void Process(const NMEAInfo &basic, DerivedInfo &calculated,
               const ComputerSettings &settings_computer,
               const Waypoints &way_points);

  void Reset();

protected:
  bool IsFinished() const {
    return countdown_autoqnh > QNH_TIME;
  }

  bool CalculateQNH(const NMEAInfo &basic, DerivedInfo &calculated,
                    const Waypoints &way_points);
  void CalculateQNH(const NMEAInfo &basic, DerivedInfo &calculated,
                    double altitude);
};

#endif
