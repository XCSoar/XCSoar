// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspStyle.hpp"
#include "ui/canvas/Ramp.hpp"
#include "Terrain/RasterRenderer.hpp"

/**
 * Compute the minimum height_scale needed so that the full range
 * of the color ramp is reachable through the 255-bucket color
 * table (max lookup value = 254 << height_scale).
 */
static constexpr unsigned
HeightScaleForRamp(const ColorRamp &ramp) noexcept
{
  const int max_h = ramp.ramp_table[ramp.num_entries - 1].h;
  unsigned scale = 0;
  while ((254 << scale) < max_h)
    ++scale;
  return scale;
}

/**
 * Create a RaspStyle with height_scale auto-computed from
 * the color ramp's range.
 */
static constexpr RaspStyle
CalcRaspStyle(const char *name, const ColorRamp &ramp,
          bool do_water = false) noexcept
{
  return { name, &ramp, HeightScaleForRamp(ramp), do_water };
}

bool
RaspStyle::HasAlpha() const noexcept
{
  return color_ramp != nullptr && color_ramp->has_alpha;
}

// Classic RASP Blipmap color schemes

static constexpr ColorRampEntry rasp_colors[6][NUM_COLOR_RAMP_LEVELS] = {
  { // Blue to red       // vertical speed (cm/s scale)
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
  {  // Blue to Yellow to red,  boundary layer height (m scale)
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
  { // Blue to Gray, 8 steps, cloud cover (percentage scale)
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
  { // sfctemp, blue to orange to red (Fahrenheit scale)
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
  { // Blue to white to red       // vertical speed (convergence, cm/s scale)
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

static constexpr ColorRampEntry rasp_colors_verticalspeed[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 5000 cm/s
     Multiplyfactor 100
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

static constexpr ColorRampEntryAlpha rasp_colors_verticalspeed_alpha[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 5000 cm/s
     Multiplyfactor 100
     Blue to Transparent to Purple */

  { 250, { 74,  85, 213, 255}},
  {1250, { 89, 137, 255, 255}},
  {2250, { 24, 160, 255, 255}},
  {3250, { 55, 196, 250, 200}},
  {3750, { 13, 217, 248, 128}},
  {4250, {  0, 239, 243,  80}},
  {5000, {255, 255, 255,   0}},
  {5500, {255, 215,   0, 100}},
  {6000, {255, 165,   0, 180}},
  {7000, {255,  73,  50, 225}},
  {7500, {255,  65, 110, 255}},
  {8500, {250,  51, 204, 255}},
  {9500, {163,   0, 211, 255}},
};


static constexpr ColorRampEntry rasp_colors_thermalstrength[NUM_COLOR_RAMP_LEVELS] = {

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

static constexpr ColorRampEntry rasp_colors_thermalheight[NUM_COLOR_RAMP_LEVELS] = {

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

static constexpr ColorRampEntry rasp_colors_temperature[NUM_COLOR_RAMP_LEVELS] = {

  /* Temperature (Celsius Scale)
     Offset 50
     Multiplyfactor 100
     Blue to Green to Orange to Red 
     
     This is the legacy color scale of ThermalMap.info. Since the current data
     path clips the negative values from the .jp2 file, negative
     temperatures are never plotted, however */

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

static constexpr ColorRampEntry rasp_colors_temperature_norm[NUM_COLOR_RAMP_LEVELS] = {

  /* Temperature (Celsius Scale)
     Blue to Green to Orange to Red 
     
     Shifted color scale that maps
     -100 C ->     0
        0 C -> 10000
      100 C -> 20000
     */

  { 5050, {  23,  16, 207}},
  { 9150, {  35,  63, 249}},
  { 9450, {48, 120, 250}},
  { 9750, {66, 178, 251}},
  {10050, {84, 235, 253}},
  {10350, {106, 255, 222}},
  {10650, {125, 242, 160}},
  {11250, {210, 241, 74}},
  {11550, {252, 233, 60}},
  {11850, {248, 180, 47}},
  {12150, {246, 144, 38}},
  {13050, {244, 36, 23}},
  {13350, {243, 0, 18}},
};

static constexpr ColorRampEntry rasp_colors_rain[NUM_COLOR_RAMP_LEVELS] = {

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

static constexpr ColorRampEntryAlpha rasp_colors_rain_alpha[NUM_COLOR_RAMP_LEVELS] = {

  /* Offset 0
     Multiplyfactor 10
     Transparent to blue to Green to Orange to Red to Purple to Blue */

  { 19, {103, 255, 254,   0}},
  { 20, {103, 255, 254,  64}},
  { 50, { 88, 204, 252, 110}},
  {100, { 83, 204,  47, 160}},
  {150, {127, 255,  62, 200}},
  {200, {193, 255, 108}},
  {300, {254, 254,  65}},
  {500, {238, 131,  35}},
  {700, {199,   0,  13}},
  {800, {155,   0,   8}},
  {900, {245,  54, 250}},
  {1200,{146,   4, 150}},
  {2000,{ 57,  15, 249}},
};

static constexpr ColorRampEntry rasp_colors_pfd_ls4[NUM_COLOR_RAMP_LEVELS] = {

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

static constexpr ColorRampEntry rasp_colors_bl_avg_windspeed[NUM_COLOR_RAMP_LEVELS] = {

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

static constexpr ColorRampEntry rasp_colors_xcspeed_ls4[NUM_COLOR_RAMP_LEVELS] = {

  /* XCSpeed - LS4,Duo Discus (km/h scale)
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

static constexpr ColorRampEntry rasp_colors_xcspeed_k8[NUM_COLOR_RAMP_LEVELS] = {

  /* XCSpeed - K8 (km/h scale)
     Offset 0
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

static constexpr ColorRampEntry rasp_colors_surface_heat_flux[NUM_COLOR_RAMP_LEVELS] = {

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

static constexpr ColorRampEntry rasp_colors_sealevel_pressure[NUM_COLOR_RAMP_LEVELS] = {

  /* Sea level pressure (hPa scale)
     Offset 1000
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

static constexpr ColorRampEntry rasp_colors_cloudfraction_low[NUM_COLOR_RAMP_LEVELS] = {

  /* Cloud fraction low level (fraction scale)
     Offset 0
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

static constexpr ColorRampEntryAlpha rasp_colors_cloudfraction_low_alpha[NUM_COLOR_RAMP_LEVELS] = {

  /* Cloud fraction low level (fraction scale)
     Offset 0
     Multiplyfactor 1000
     White to Green */

  { -2, {255, 255, 255,   0}},
  { 90, {255, 255, 255,  40}},
  { 91, {234, 246, 232,  75}},
  {182, {213, 238, 209, 150}},
  {273, {191, 229, 186, 200}},
  {364, {166, 218, 163, 220}},
  {455, {134, 202, 140}},
  {545, {102, 187, 118}},
  {636, { 69, 171,  96}},
  {727, { 49, 147,  78}},
  {818, { 33, 121,  61}},
  {909, { 16,  94,  44}},
  {1000,{  0,  68,  27}},
};


static constexpr ColorRampEntry rasp_colors_cloudfraction_mid[NUM_COLOR_RAMP_LEVELS] = {

  /* Cloud fraction mid level (fraction scale)
     Offset 0
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

static constexpr ColorRampEntryAlpha rasp_colors_cloudfraction_mid_alpha[NUM_COLOR_RAMP_LEVELS] = {

  /* Cloud fraction mid level (fraction scale)
     Offset 0
     Multiplyfactor 1000
     Transparent to White to Blue */

  {  0, {255, 255, 255,   0}},
  { 90, {255, 255, 255, 100}},
  {200, {233, 242, 248, 150}},
  {420, {211, 230, 242, 200}},
  {480, {189, 217, 235, 255}},
  {550, {163, 202, 227}},
  {615, {132, 184, 218}},
  {685, {101, 165, 208}},
  {736, { 70, 147, 199}},
  {777, { 51, 123, 178}},
  {818, { 37,  98, 155}},
  {909, { 22,  73, 131}},
  {1000,{  8,  48, 107}},
};

static constexpr ColorRampEntry rasp_colors_cloudfraction_high[NUM_COLOR_RAMP_LEVELS] = {

  /* Cloud fraction high level (fraction scale)
     Offset -3
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

static constexpr ColorRampEntryAlpha rasp_colors_cloudfraction_high_alpha[NUM_COLOR_RAMP_LEVELS] = {

  /* Cloud fraction high level (fraction scale)
     Offset -3
     Multiplyfactor 1000
     Transparent to White to Red */

  {  0, {255, 255, 255,   0}},
  { 90, {254, 230, 222,  64}},
  { 91, {254, 230, 222,  64}},
  {182, {253, 205, 188, 125}},
  {273, {253, 180, 155, 175}},
  {364, {250, 153, 125, 200}},
  {455, {245, 123,  99, 225}},
  {545, {240,  93,  74}},
  {636, {235,  63,  49}},
  {727, {208,  44,  36}},
  {818, {173,  29,  29}},
  {909, {138,  15,  21}},
  {1000,{103,   0,  13}},
};

static constexpr ColorRampEntry rasp_colors_cloudfraction_accumulated[NUM_COLOR_RAMP_LEVELS] = {

  /* Cloud fraction, all levels combined (fraction scale)
     Offset 0
     Multiplyfactor 10
     White to Grey */

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

static constexpr ColorRampEntryAlpha rasp_colors_cloudfraction_accumulated_alpha[NUM_COLOR_RAMP_LEVELS] = {

  /* Cloud fraction, all levels combined (fraction scale)
     Offset 0
     Multiplyfactor 10
     Transparent to White to Grey*/

  {  0, {255, 255, 255,   0}},
  { 83, {255, 255, 255,  64}},
  {167, {255, 255, 255, 100}},
  {250, {255, 255, 255, 125}},
  {333, {255, 255, 255, 150}},
  {417, {255, 255, 255, 175}},
  {500, {173, 173, 173, 200}},
  {583, {159, 159, 159, 255}},
  {657, {146, 146, 146}},
  {720, {132, 132, 132}},
  {800, {118, 118, 118}},
  {883, {105, 105, 105}},
  {1000, {91, 91, 91}},
};

// ColorRamp wrapper objects for classic Blipmap color schemes

static constexpr ColorRamp ramp_classic[] = {
  { false, NUM_COLOR_RAMP_LEVELS, rasp_colors[0], nullptr },
  { false, NUM_COLOR_RAMP_LEVELS, rasp_colors[1], nullptr },
  { false, NUM_COLOR_RAMP_LEVELS, rasp_colors[2], nullptr },
  { false, NUM_COLOR_RAMP_LEVELS, rasp_colors[3], nullptr },
  { false, NUM_COLOR_RAMP_LEVELS, rasp_colors[4], nullptr },
  { false, NUM_COLOR_RAMP_LEVELS, rasp_colors[5], nullptr },
};

// ColorRamp wrapper objects for thermalmap.info color schemes

static constexpr ColorRamp ramp_verticalspeed = {
  true, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_verticalspeed, rasp_colors_verticalspeed_alpha
};

static constexpr ColorRamp ramp_thermalstrength = {
  false, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_thermalstrength, nullptr
};

static constexpr ColorRamp ramp_thermalheight = {
  false, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_thermalheight, nullptr
};

static constexpr ColorRamp ramp_temperature = {
  false, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_temperature, nullptr
};

static constexpr ColorRamp ramp_temperature_norm = {
  false, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_temperature_norm, nullptr
};

static constexpr ColorRamp ramp_rain = {
  true, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_rain, rasp_colors_rain_alpha
};

static constexpr ColorRamp ramp_pfd_ls4 = {
  false, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_pfd_ls4, nullptr
};

static constexpr ColorRamp ramp_bl_avg_windspeed = {
  false, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_bl_avg_windspeed, nullptr
};

static constexpr ColorRamp ramp_xcspeed_ls4 = {
  false, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_xcspeed_ls4, nullptr
};

static constexpr ColorRamp ramp_xcspeed_k8 = {
  false, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_xcspeed_k8, nullptr
};

static constexpr ColorRamp ramp_surface_heat_flux = {
  false, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_surface_heat_flux, nullptr
};

static constexpr ColorRamp ramp_sealevel_pressure = {
  false, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_sealevel_pressure, nullptr
};

static constexpr ColorRamp ramp_cloudfraction_low = {
  true, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_cloudfraction_low, rasp_colors_cloudfraction_low_alpha
};

static constexpr ColorRamp ramp_cloudfraction_mid = {
  true, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_cloudfraction_mid, rasp_colors_cloudfraction_mid_alpha
};

static constexpr ColorRamp ramp_cloudfraction_high = {
  true, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_cloudfraction_high, rasp_colors_cloudfraction_high_alpha
};

static constexpr ColorRamp ramp_cloudfraction_accumulated = {
  true, NUM_COLOR_RAMP_LEVELS,
  rasp_colors_cloudfraction_accumulated,
  rasp_colors_cloudfraction_accumulated_alpha
};

/* RASP styles
   List of styles, first Blipmap classic, then thermalmap.info styles
   List of tuples:
     * string fieldname,
     * pointer to ColorRamp wrapper
     * int n = value range level. A value of n means the highest value in
       the data (and the top end of the used colormap) has to be smaller than
       256*(2**n)-1
     * bool do_water enables "water masking", special handling of
       the value 255 (for n=0) as water indicator */

const RaspStyle rasp_styles[] = {
  { "wstar", &ramp_classic[0],
    2, // max range 256*(2**2) = 1024 cm/s = 10 m/s
    false },
  { "wstar_bsratio", &ramp_classic[0],
    2, // max range 256*(2**2) = 1024 cm/s = 10 m/s
    false },
  { "blwindspd", &ramp_classic[1], 3, false },
  { "hbl", &ramp_classic[2], 4, false },
  { "dwcrit", &ramp_classic[2], 4, false },
  { "blcloudpct", &ramp_classic[3], 0, true },
  { "sfctemp", &ramp_classic[4], 0, false },
  { "hwcrit", &ramp_classic[2], 4, false },
  { "wblmaxmin", &ramp_classic[5],
    1, // max range 256*(2**1) = 512 cm/s = 5.0 m/s
    false },
  { "blcwbase", &ramp_classic[2], 4, false },

  // Thermalmap.info Colorschemes and Rasp file names
  CalcRaspStyle("ThermalStrength", ramp_thermalstrength),
  CalcRaspStyle("ThermalHeight", ramp_thermalheight),
  CalcRaspStyle("Convergence", ramp_verticalspeed),
  CalcRaspStyle("Wave_1000m", ramp_verticalspeed),
  CalcRaspStyle("Wave_2000m", ramp_verticalspeed),
  CalcRaspStyle("Wave_3000m", ramp_verticalspeed),
  CalcRaspStyle("Wave_4000m", ramp_verticalspeed),
  CalcRaspStyle("Wave_5000m", ramp_verticalspeed),
  CalcRaspStyle("Wave_6000m", ramp_verticalspeed),
  CalcRaspStyle("Wave_7000m", ramp_verticalspeed),
  CalcRaspStyle("Temperature2m", ramp_temperature),
  CalcRaspStyle("Rain", ramp_rain),
  CalcRaspStyle("PFD_Day_DuoDiscus", ramp_pfd_ls4),
  CalcRaspStyle("PFD_Day_LS4", ramp_pfd_ls4),
  CalcRaspStyle("PFD_Day_K8", ramp_pfd_ls4),
  CalcRaspStyle("Windspeed_10m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_500m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_1000m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_1500m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_2000m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_2500m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_3000m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_3500m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_4000m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_4500m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_5000m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_5500m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_6000m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_6500m", ramp_bl_avg_windspeed),
  CalcRaspStyle("Windspeed_7000m", ramp_bl_avg_windspeed),
  CalcRaspStyle("VerticalWindShear", ramp_bl_avg_windspeed),
  CalcRaspStyle("BL_AverageWindSpeed", ramp_bl_avg_windspeed),
  CalcRaspStyle("XCSpeed_LS4", ramp_xcspeed_ls4),
  CalcRaspStyle("XCSpeed_DuoDiscus", ramp_xcspeed_ls4),
  CalcRaspStyle("XCSpeed_K8", ramp_xcspeed_k8),
  CalcRaspStyle("SurfaceHeatFlux", ramp_surface_heat_flux),
  CalcRaspStyle("SeaLevelPressure", ramp_sealevel_pressure),
  CalcRaspStyle("Cloudfraction_Low", ramp_cloudfraction_low),
  CalcRaspStyle("Cloudfraction_Mid", ramp_cloudfraction_mid),
  CalcRaspStyle("Cloudfraction_High", ramp_cloudfraction_high),
  CalcRaspStyle("Cloudfraction_Accumulated",
            ramp_cloudfraction_accumulated),

  { nullptr, &ramp_classic[0], 2, false }
};

const RaspStyle rasp_colormaps_general[] = {
  // Thermalmap.info color scheme codes
  CalcRaspStyle("vth", ramp_thermalstrength),
  CalcRaspStyle("hth", ramp_thermalheight),
  CalcRaspStyle("vve", ramp_verticalspeed),
  CalcRaspStyle("tce", ramp_temperature_norm),
  CalcRaspStyle("prr", ramp_rain),
  CalcRaspStyle("pfd", ramp_pfd_ls4),
  CalcRaspStyle("vho", ramp_bl_avg_windspeed),
  CalcRaspStyle("vxc", ramp_xcspeed_ls4),
  CalcRaspStyle("vx8", ramp_xcspeed_k8),
  CalcRaspStyle("hfl", ramp_surface_heat_flux),
  CalcRaspStyle("pre", ramp_sealevel_pressure),
  CalcRaspStyle("fcl", ramp_cloudfraction_low),
  CalcRaspStyle("fcm", ramp_cloudfraction_mid),
  CalcRaspStyle("fch", ramp_cloudfraction_high),
  CalcRaspStyle("fct", ramp_cloudfraction_accumulated),

  { nullptr, &ramp_classic[0], 2, false }
};
