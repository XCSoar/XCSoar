// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspStyle.hpp"
#include "ui/canvas/Ramp.hpp"
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

// Thermalmap.info Color schemes

static constexpr ColorRamp rasp_colors_verticalspeed[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 5000 cm/s
     Multiplyfactor 1000
     Blue to white to Purple */

  {250, {74, 85, 213}},
  {1250, {89, 137, 255}},
  {2250, {24, 160, 255}},
  {3250, {55, 196, 250}},
  {3750, {13, 217, 248}},
  {4250, {0, 239, 243}},
  {5000, {255, 255, 255}},
  {5500, {255, 215, 0}},
  {6000, {255, 165, 0}},
  {7000, {255, 73, 50}},
  {7500, {255, 65, 110}},
  {8500, {250, 51, 204}},
  {9500, {163, 0, 211}},
};

static constexpr ColorRamp rasp_colors_thermalstrength[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 0
     Multiplyfactor 100
     Blue to Green to Yellow to Red to Purple */

  {74, {255, 255, 255}},
  {75, {0, 128, 255}},
  {100, {0, 160, 255}},
  {150, {64, 224, 255}},
  {175, {64, 255, 255}},
  {200, {64, 255, 192}},
  {250, {128, 255, 64}},
  {275, {192, 255, 64}},
  {300, {255, 255, 64}},
  {325, {255, 224, 64}},
  {400, {255, 32, 64}},
  {425, {255, 96, 192}},
  {475, {185, 39, 190}},
};

static constexpr ColorRamp rasp_colors_thermalheight[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 0
     Multiplyfactor 1
     Blue to Green to Orange to Red */

  {199, {255, 255, 255}},
  {200, {65, 71, 173}},
  {400, {71, 119, 239}},
  {600, {56, 165, 251}},
  {800, {27, 208, 213}},
  {1000, {38, 237, 166}},
  {1200, {100, 253, 106}},
  {1400, {164, 252, 60}},
  {1600, {211, 232, 53}},
  {1800, {245, 198, 58}},
  {2000, {254, 153, 44}},
  {2200, {243, 99, 21}},
  {2800, {122, 4, 3}},
};

static constexpr ColorRamp rasp_colors_temperature[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 50
     Multiplyfactor 100
     Blue to Green to Orange to Red */

  {-4950, {23, 16, 207}},
  {-850, {35, 63, 249}},
  {-550, {48, 120, 250}},
  {-250, {66, 178, 251}},
  {50, {84, 235, 253}},
  {350, {106, 255, 222}},
  {650, {125, 242, 160}},
  {1250, {210, 241, 74}},
  {1550, {252, 233, 60}},
  {1850, {248, 180, 47}},
  {2150, {246, 144, 38}},
  {3050, {244, 36, 23}},
  {3350, {243, 0, 18}},
};

static constexpr ColorRamp rasp_colors_rain[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 0
     Multiplyfactor 10
     Blue to Green to Orange to Red to Purple to Blue */

  {19, {255, 255, 255}},
  {20, {103, 255, 254}},
  {50, {88, 204, 252}},
  {100, {83, 204, 47}},
  {150, {127, 255, 62}},
  {200, {193, 255, 108}},
  {300, {254, 254, 65}},
  {500, {238, 131, 35}},
  {700, {199, 0, 13}},
  {800, {155, 0, 8}},
  {900, {245, 54, 250}},
  {1200, {146, 4, 150}},
  {2000, {57, 15, 249}},
};

static constexpr ColorRamp rasp_colors_pfd_ls4[NUM_COLOR_RAMP_LEVELS] = {

  /* PFD - LS4, DuoDiscus
     Offset 0
     Multiplyfactor 1
     Blue to Green to Orange to Red */

  {23, {255, 255, 255}},
  {24, {255, 255, 255}},
  {25, {33, 13, 41}},
  {50, {48, 18, 59}},
  {100, {70, 98, 216}},
  {200, {53, 171, 248}},
  {300, {27, 229, 181}},
  {400, {116, 254, 93}},
  {500, {201, 239, 52}},
  {600, {251, 185, 56}},
  {700, {245, 105, 24}},
  {800, {201, 41, 3}},
  {900, {122, 4, 3}},
};

static constexpr ColorRamp rasp_colors_bl_avg_windspeed[NUM_COLOR_RAMP_LEVELS] = {

  /* Windspeed, WindShear, BL Avg Windspeed
     Offset 100
     Multiplyfactor 1
     Blue to Green to Yellow to Orange to Purple */

  {0, {40, 140, 255}},
  {200, {0, 220, 225}},
  {400, {0, 234, 156}},
  {600, {240, 245, 3}},
  {800, {255, 219, 0}},
  {1000, {255, 152, 0}},
  {1200, {247, 120, 0}},
  {1400, {224, 97, 40}},
  {1800, {220, 81, 50}},
  {2400, {205, 58, 70}},
  {3000, {180, 26, 90}},
  {3600, {150, 40, 120}},
  {4200, {122, 44, 122}},
};

static constexpr ColorRamp rasp_colors_xcspeed_ls4[NUM_COLOR_RAMP_LEVELS] = {

  /* XCSpeed - LS4,Duo Discus
     Offset 0
     Multiplyfactor 1
     Blue to Green to Orange to Red */

  {58, {255, 255, 255}},
  {59, {255, 255, 255}},
  {60, {48, 18, 59}},
  {70, {69, 91, 205}},
  {80, {62, 156, 254}},
  {90, {24, 215, 203}},
  {100, {72, 248, 130}},
  {110, {164, 252, 60}},
  {120, {226, 220, 56}},
  {130, {254, 163, 49}},
  {140, {239, 89, 17}},
  {150, {194, 36, 3}},
  {160, {122, 4, 3}},
};

static constexpr ColorRamp rasp_colors_xcspeed_k8[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 0
     Multiplyfactor 1
     Blue to Green to Orange to Red */

  {25, {255, 255, 255}},
  {26, {255, 255, 255}},
  {27, {255, 255, 255}},
  {28, {255, 255, 255}},
  {29, {255, 255, 255}},
  {30, {24, 215, 203}},
  {40, {72, 248, 130}},
  {50, {164, 252, 60}},
  {60, {226, 220, 56}},
  {70, {254, 163, 49}},
  {80, {239, 89, 17}},
  {90, {194, 36, 3}},
  {100, {122, 4, 3}},
};

static constexpr ColorRamp rasp_colors_surface_heat_flux[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 10000
     Multiplyfactor 10
     Blue to White to Red */

  {0, {11, 5, 130}},
  {5500, {34, 43, 160}},
  {6500, {78, 119, 215}},
  {7000, {98, 157, 236}},
  {7500, {122, 191, 252}},
  {8500, {193, 236, 254}},
  {9500, {233, 252, 255}},
  {10000, {244, 253, 233}},
  {11000, {252, 242, 144}},
  {11500, {251, 220, 106}},
  {12500, {248, 183, 53}},
  {13500, {244, 138, 39}},
  {14500, {227, 0, 16}},
};

static constexpr ColorRamp rasp_colors_sealevel_pressure[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 1000
     Multiplyfactor 20
     Blue to White to Red */

  {20400, {11, 5, 130}},
  {20560, {77, 118, 215}},
  {20640, {107, 174, 246}},
  {20720, {155, 212, 253}},
  {20880, {231, 251, 255}},
  {20960, {246, 251, 215}},
  {21040, {252, 245, 149}},
  {21120, {250, 212, 92}},
  {21200, {249, 185, 54}},
  {21360, {237, 82, 30}},
  {21440, {227, 0, 30}},
  {21520, {227, 0, 102}},
  {21680, {227, 0, 255}},
};

static constexpr ColorRamp rasp_colors_cloudfraction_low[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 0
     Multiplyfactor 1000
     White to Green */

  {-2, {255, 255, 255}},
  {90, {255, 255, 255}},
  {91, {234, 246, 232}},
  {182, {213, 238, 209}},
  {273, {191, 229, 186}},
  {364, {166, 218, 163}},
  {455, {134, 202, 140}},
  {545, {102, 187, 118}},
  {636, {69, 171, 96}},
  {727, {49, 147, 78}},
  {818, {33, 121, 61}},
  {909, {16, 94, 44}},
  {1000, {0, 68, 27}},
};

static constexpr ColorRamp rasp_colors_cloudfraction_mid[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 0
     Multiplyfactor 1000
     White to Blue */

  {0, {255, 255, 255}},
  {90, {255, 255, 255}},
  {91, {233, 242, 248}},
  {182, {211, 230, 242}},
  {273, {189, 217, 235}},
  {364, {163, 202, 227}},
  {455, {132, 184, 218}},
  {545, {101, 165, 208}},
  {636, {70, 147, 199}},
  {727, {51, 123, 178}},
  {818, {37, 98, 155}},
  {909, {22, 73, 131}},
  {1000, {8, 48, 107}},
};

static constexpr ColorRamp rasp_colors_cloudfraction_high[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset -3
     Multiplyfactor 1000
     White to Red */

  {0, {255, 255, 255}},
  {90, {255, 255, 255}},
  {91, {254, 230, 222}},
  {182, {253, 205, 188}},
  {273, {253, 180, 155}},
  {364, {250, 153, 125}},
  {455, {245, 123, 99}},
  {545, {240, 93, 74}},
  {636, {235, 63, 49}},
  {727, {208, 44, 36}},
  {818, {173, 29, 29}},
  {909, {138, 15, 21}},
  {1000, {103, 0, 13}},
};

static constexpr ColorRamp rasp_colors_cloudfraction_accumulated[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 0
     Multiplyfactor 10
     White to Red */

  {0, {255, 255, 255}},
  {83, {241, 241, 241}},
  {167, {228, 228, 228}},
  {250, {214, 214, 214}},
  {333, {200, 200, 200}},
  {417, {187, 187, 187}},
  {500, {173, 173, 173}},
  {583, {159, 159, 159}},
  {667, {146, 146, 146}},
  {750, {132, 132, 132}},
  {833, {118, 118, 118}},
  {917, {105, 105, 105}},
  {1000, {91, 91, 91}},
};

const RaspStyle rasp_styles[] = {
  { "wstar", rasp_colors[0],
    2, // max range 256*(2**2) = 1024 cm/s = 10 m/s
    false },
  { "wstar_bsratio", rasp_colors[0],
    2, // max range 256*(2**2) = 1024 cm/s = 10 m/s
    false },
  { "blwindspd", rasp_colors[1], 3, false },
  { "hbl", rasp_colors[2], 4, false },
  { "dwcrit", rasp_colors[2], 4, false },
  { "blcloudpct", rasp_colors[3], 0, true },
  { "sfctemp", rasp_colors[4], 0, false },
  { "hwcrit", rasp_colors[2], 4, false },
  { "wblmaxmin", rasp_colors[5],
    1, // max range 256*(1**2) = 512 cm/s = 5.0 m/s
    false },
  { "blcwbase", rasp_colors[2], 4, false },

  // Thermalmap.info Colorschemes and Rasp file names
  { "ThermalStrength", rasp_colors_thermalstrength, 4, false },
  { "ThermalHeight", rasp_colors_thermalheight, 4, false },
  { "Convergence", rasp_colors_verticalspeed, 5, false },
  { "Wave_1000m", rasp_colors_verticalspeed, 5, false },
  { "Wave_2000m", rasp_colors_verticalspeed, 5, false },
  { "Wave_3000m", rasp_colors_verticalspeed, 5, false },
  { "Wave_4000m", rasp_colors_verticalspeed, 5, false },
  { "Wave_5000m", rasp_colors_verticalspeed, 5, false },
  { "Wave_6000m", rasp_colors_verticalspeed, 5, false },
  { "Wave_7000m", rasp_colors_verticalspeed, 5, false },
  { "Temperature2m", rasp_colors_temperature, 4, false },
  { "Rain", rasp_colors_rain, 3, false },
  { "PFD_Day_DuoDiscus", rasp_colors_pfd_ls4, 3, false },
  { "PFD_Day_LS4", rasp_colors_pfd_ls4, 3, false },
  { "PFD_Day_K8", rasp_colors_pfd_ls4, 3, false },
  { "Windspeed_10m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_500m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_1000m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_1500m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_2000m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_2500m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_3000m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_3500m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_4000m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_4500m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_5000m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_5500m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_6000m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_6500m", rasp_colors_bl_avg_windspeed, 3, false },
  { "Windspeed_7000m", rasp_colors_bl_avg_windspeed, 3, false },
  { "VerticalWindShear", rasp_colors_bl_avg_windspeed, 3, false },
  { "BL_AverageWindSpeed", rasp_colors_bl_avg_windspeed, 3, false },
  { "XCSpeed_LS4", rasp_colors_xcspeed_ls4, 3, false },
  { "XCSpeed_DuoDiscus", rasp_colors_xcspeed_ls4, 3, false },
  { "XCSpeed_K8", rasp_colors_xcspeed_k8, 3, false },
  { "SurfaceHeatFlux", rasp_colors_surface_heat_flux, 6, false },
  { "SeaLevelPressure", rasp_colors_sealevel_pressure, 7, false },
  { "Cloudfraction_Low", rasp_colors_cloudfraction_low, 4, false },
  { "Cloudfraction_Mid", rasp_colors_cloudfraction_mid, 4, false },
  { "Cloudfraction_High", rasp_colors_cloudfraction_high, 4, false },
  { "Cloudfraction_Accumulated", rasp_colors_cloudfraction_accumulated, 4, false },

  { nullptr, rasp_colors[0], 2, false }
};
