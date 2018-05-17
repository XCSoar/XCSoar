/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef CLIMBAVERAGECALCULATOR_HPP
#define CLIMBAVERAGECALCULATOR_HPP

class ClimbAverageCalculator
{
  static constexpr int MAX_HISTORY = 40;
  struct HistoryItem
  {
    double time;
    double altitude;

    HistoryItem() = default;

    constexpr HistoryItem(double _time, double _altitude)
      :time(_time), altitude(_altitude) {}

    bool IsDefined() const {
      return time >= 0;
    }

    void Reset() {
      time = -1;
    }
  };

  HistoryItem history[MAX_HISTORY];
  int newestValIndex;

public:
  double GetAverage(double time, double altitude, double average_time);
  void Reset();
  bool Expired(double now, double max_age) const;
};

#endif

