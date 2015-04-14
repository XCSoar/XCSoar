/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_BLUEFLYVARIO_INTERNAL_HPP
#define XCSOAR_BLUEFLYVARIO_INTERNAL_HPP

#include "Device/Driver.hpp"
#include "Math/KalmanFilter1d.hpp"
#include "NMEA/Info.hpp"

class BlueFlyDevice : public AbstractDevice {
public:
  struct BlueFlySettings {
    unsigned version;

    fixed volume;
    static constexpr const char *VOLUME_NAME = "BVL";
    static constexpr unsigned VOLUME_MAX = 1000;
    static constexpr unsigned VOLUME_MULTIPLIER = 1000;

    unsigned output_mode;
    static constexpr const char *OUTPUT_MODE_NAME = "BOM";
    static constexpr unsigned OUTPUT_MODE_MAX = 3;

    void Parse(const char *name, unsigned long value);
};

private:
  BlueFlySettings settings;
  char *settings_keys;

  KalmanFilter1d kalman_filter;

  bool ParseBAT(const char *content, NMEAInfo &info);
  bool ParsePRS(const char *content, NMEAInfo &info);
  bool ParseBFV(const char *content, NMEAInfo &info);
  bool ParseBST(const char *content, NMEAInfo &info);
  bool ParseSET(const char *content, NMEAInfo &info);

public:
  BlueFlyDevice();
  ~BlueFlyDevice();

  virtual void LinkTimeout() override;
  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};

#endif
