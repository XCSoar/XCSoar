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

#include "RaspStyle.hpp"
#include "Screen/Ramp.hpp"
#include "Terrain/RasterRenderer.hpp"

static constexpr ColorRamp rasp_colors[6][NUM_COLOR_RAMP_LEVELS] = {
  { // Blue to red       // vertical speed
    {   0, { 0, 0, 255 }}, // -200
    { 100, { 0, 195, 255 }}, // -100
    { 200, { 52, 192, 11 }}, // 0
    { 250, { 182, 233, 4 }}, // 40
    { 300, { 255, 233, 0 }}, // 80
    { 360, { 255, 209, 0 }}, // 120
    { 420, { 255, 155, 0 }}, // 160
    { 480, { 255, 109, 0 }}, // 200
    { 540, { 255, 35, 0 }}, // 240
    { 600, { 255, 00, 0 }}, // 300
    {1000, { 0xFF, 0x00, 0x00 }},
    {8000, { 0xFF, 0x00, 0x00 }},
    {9000, { 0xFF, 0x00, 0x00 }}
  },
  {
    {0, { 0xFF, 0xFF, 0xFF }},
    {250, { 0x80, 0x80, 0xFF }},
    {500, { 0x80, 0xFF, 0xFF }},
    {750, { 0xFF, 0xFF, 0x80 }},
    {1000, { 0xFF, 0x80, 0x80 }},
    {1250, { 0xFF, 0x80, 0x80 }},
    {2000, { 0xFF, 0xA0, 0xA0 }},
    {3000, { 0xFF, 0xA0, 0xA0 }},
    {4000, { 0xFF, 0x00, 0x00 }},
    {5000, { 0xFF, 0x00, 0x00 }},
    {6000, { 0xFF, 0x00, 0x00 }},
    {7000, { 0xFF, 0x00, 0x00 }},
    {8000, { 0xFF, 0x00, 0x00 }}
  },
  {
    {0, { 0xFF, 0xFF, 0xFF }},
    {750, { 0x80, 0x80, 0xFF }},
    {1500, { 0x80, 0xFF, 0xFF }},
    {2250, { 0xFF, 0xFF, 0x80 }},
    {3000, { 0xFF, 0x80, 0x80 }},
    {3500, { 0xFF, 0x80, 0x80 }},
    {6000, { 0xFF, 0xA0, 0xA0 }},
    {8000, { 0xFF, 0xA0, 0xA0 }},
    {9000, { 0xFF, 0x00, 0x00 }},
    {9500, { 0xFF, 0x00, 0x00 }},
    {9600, { 0xFF, 0x00, 0x00 }},
    {9700, { 0xFF, 0x00, 0x00 }},
    {20000, { 0xFF, 0x00, 0x00 }}
  },
  { // Blue to Gray, 8 steps
    {   0, { 0, 153, 204 }},
    {  12, { 102, 229, 255 }},
    {  25, { 153, 255, 255 }},
    {  37, { 204, 255, 255 }},
    {  50, { 229, 229, 229 }},
    {  62, { 173, 173, 173 }},
    {  75, { 122, 122, 122 }},
    { 100, { 81, 81, 81 }},
    {5000, { 71, 71, 71 }},
    {6000, { 0xFF, 0x00, 0x00 }},
    {7000, { 0xFF, 0x00, 0x00 }},
    {8000, { 0xFF, 0x00, 0x00 }},
    {9000, { 0xFF, 0x00, 0x00 }}
  },
  { // sfctemp, blue to orange to red
    {   0, { 7, 90, 255 }},
    {  30, { 50, 118, 255 }},
    {  70, { 89, 144, 255 }},
    {  73, { 140, 178, 255 }},
    {  76, { 191, 212, 255 }},
    {  79, { 229, 238, 255 }},
    {  82, { 247, 249, 255 }},
    {  85, { 255, 255, 204 }},
    {  88, { 255, 255, 153 }},
    {  91, { 255, 255, 0 }},
    {  95, { 255, 204, 0 }},
    { 100, { 255, 153, 0 }},
    { 120, { 255, 0, 0 }}
  },
  { // Blue to white to red       // vertical speed (convergence)
    {   0, { 7, 90, 255 }},
    { 100, { 50, 118, 255 }},
    { 140, { 89, 144, 255 }},
    { 160, { 140, 178, 255 }},
    { 180, { 191, 212, 255 }},
    { 190, { 229, 238, 255 }},
    { 200, { 247, 249, 255 }},
    { 210, { 255, 255, 204 }},
    { 220, { 255, 255, 153 }},
    { 240, { 255, 255, 0 }},
    { 260, { 255, 204, 0 }},
    { 300, { 255, 153, 0 }},
    {1000, { 255, 102, 0 }},
  },
};

const RaspStyle rasp_styles[] = {
  { _T("wstar"), rasp_colors[0],
    2, // max range 256*(2**2) = 1024 cm/s = 10 m/s
    false },
  { _T("wstar_bsratio"), rasp_colors[0],
    2, // max range 256*(2**2) = 1024 cm/s = 10 m/s
    false },
  { _T("blwindspd"), rasp_colors[1], 3, false },
  { _T("hbl"), rasp_colors[2], 4, false },
  { _T("dwcrit"), rasp_colors[2], 4, false },
  { _T("blcloudpct"), rasp_colors[3], 0, true },
  { _T("sfctemp"), rasp_colors[4], 0, false },
  { _T("hwcrit"), rasp_colors[2], 4, false },
  { _T("wblmaxmin"), rasp_colors[5],
    1, // max range 256*(1**2) = 512 cm/s = 5.0 m/s
    false },
  { _T("blcwbase"), rasp_colors[2], 4, false },
  { nullptr, rasp_colors[0], 2, false }
};
