/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

// Value used for comparison in climb tone
static constexpr unsigned X_NONE = 0;
static constexpr unsigned X_MACCREADY = 1;
static constexpr unsigned X_AVERAGE = 2;

// Condition for detecting lift in cruise mode
static constexpr unsigned Y_NONE = 0;
static constexpr unsigned Y_RELATIVE_ZERO = 1;
static constexpr unsigned Y_RELATIVE_MACCREADY_HALF = 2;
static constexpr unsigned Y_GROSS_ZERO = 3;
static constexpr unsigned Y_NET_MACCREADY_HALF = 4;
static constexpr unsigned Y_RELATIVE_MACCREADY = 5;
static constexpr unsigned Y_NET_MACCREADY = 6;

// Beep types
static constexpr unsigned BEEPTYPE_SILENCE = 0;
static constexpr unsigned BEEPTYPE_SHORT = 1;
static constexpr unsigned BEEPTYPE_MEDIUM = 2;
static constexpr unsigned BEEPTYPE_LONG = 3;
static constexpr unsigned BEEPTYPE_CONTINUOUS = 4;
static constexpr unsigned BEEPTYPE_SHORTDOUBLE = 5;

// Pitch value schemes
static constexpr unsigned PITCH_CONST_HI = 0;
static constexpr unsigned PITCH_CONST_MEDIUM = 1;
static constexpr unsigned PITCH_CONST_LO = 2;
static constexpr unsigned PITCH_SPEED_PERCENT = 3;
static constexpr unsigned PITCH_SPEED_ERROR = 4;
static constexpr unsigned PITCH_VARIO_GROSS = 5;
static constexpr unsigned PITCH_VARIO_NET = 6;
static constexpr unsigned PITCH_VARIO_RELATIVE = 7;
static constexpr unsigned PITCH_VARIO_GROSSRELATIVE = 8;

// Beep period value schemes
static constexpr unsigned PERIOD_CONST_HI = 0;
static constexpr unsigned PERIOD_CONST_MEDIUM = 1;
static constexpr unsigned PERIOD_CONST_LO = 2;
static constexpr unsigned PERIOD_SPEED_PERCENT = 3;
static constexpr unsigned PERIOD_SPEED_ERROR = 4;
static constexpr unsigned PERIOD_VARIO_GROSS = 5;
static constexpr unsigned PERIOD_VARIO_NET = 6;
static constexpr unsigned PERIOD_VARIO_RELATIVE = 7;
static constexpr unsigned PERIOD_VARIO_GROSSRELATIVE = 8;
static constexpr unsigned PERIOD_CONST_INTERMITTENT = 9;

// Scaling schemes applied to pitch and period
static constexpr unsigned SCALE_LINEAR = 0;
static constexpr unsigned SCALE_LOWEND = 1;
static constexpr unsigned SCALE_HIGHEND = 2;
static constexpr unsigned SCALE_LINEAR_NEG = 3;
static constexpr unsigned SCALE_LOWEND_NEG = 4;
static constexpr unsigned SCALE_HIGHEND_NEG = 5;
