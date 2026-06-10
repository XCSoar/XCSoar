// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspStyle.hpp"

#include <iterator>

// ---- Colormap value-space -> system-unit conversions ----
// These convert the values authored in each ColorMap into the
// canonical system unit expected by the matching UnitGroup
// (m/s for speeds, m for altitude, Kelvin for temperature, hPa for
// pressure).  system_value = colormap_value * scale + offset.
static constexpr float KNOTS_TO_MS = 0.514444f;
static constexpr float KMH_TO_MS = 1.0f / 3.6f;
static constexpr float CMS_TO_MS = 0.01f;
static constexpr float KM_TO_M = 1000.0f;
static constexpr float CELSIUS_TO_KELVIN = 273.15f;
// Fahrenheit -> Kelvin: K = (F + 459.67) / 1.8
static constexpr float FAHRENHEIT_TO_KELVIN_SCALE = 1.0f / 1.8f;
static constexpr float FAHRENHEIT_TO_KELVIN_OFFSET = 459.67f / 1.8f;

/**
 * Compute the minimum height_scale needed so that the full range
 * of the color map is reachable through the 255-bucket color
 * table (max lookup value = 254 << height_scale).
 */
static constexpr unsigned
HeightScaleForColorMap(const ColorMap &map,
                       float scale, float offset) noexcept
{
  float max_v = map.points[map.num_points - 1].value;
  int max_h = static_cast<int>(max_v * scale + offset);
  unsigned s = 0;
  while ((254 << s) < max_h)
    ++s;
  return s;
}

/**
 * Create a RaspStyle with height_scale auto-computed from
 * the color map's range and transform.
 */
static constexpr RaspStyle
MakeRaspStyle(const char *name,
              const ColorMap &map,
              const ColorMap &map_alpha,
              float scale, float offset,
              bool do_water = false,
              UnitGroup unit_group = UnitGroup::NONE,
              float value_scale = 1, float value_offset = 0) noexcept
{
  return {name, map, map_alpha, scale, offset,
          HeightScaleForColorMap(map, scale, offset),
          do_water, unit_group, value_scale, value_offset};
}

static constexpr RaspStyle
MakeRaspStyle(const char *name,
              const ColorMap &map,
              float scale, float offset,
              bool do_water = false,
              UnitGroup unit_group = UnitGroup::NONE,
              float value_scale = 1, float value_offset = 0) noexcept
{
  return MakeRaspStyle(name, map, {}, scale, offset, do_water,
                       unit_group, value_scale, value_offset);
}

// ---- Classic RASP Blipmap color schemes ----
// Identity transform: physical value = h value directly

// Blue to red (vertical speed, m/s)
// h = m_s * 100 + 200  (offset 200 = 0 m/s, classic RASP cm/s encoding)
static constexpr ColorPoint classic_vertical_speed[] = {
  {-2.0f, {  0,   0, 255}},
  {-1.0f, {  0, 195, 255}},
  { 0.0f, { 52, 192,  11}},
  { 0.5f, {182, 233,   4}},
  { 1.0f, {255, 233,   0}},
  { 1.6f, {255, 209,   0}},
  { 2.2f, {255, 155,   0}},
  { 2.8f, {255, 109,   0}},
  { 3.4f, {255,  35,   0}},
  { 4.0f, {255,   0,   0}},
};
static constexpr ColorMap classic_vertical_speed_map = {
  classic_vertical_speed, std::size(classic_vertical_speed)
};

// White to purple (wind speed, knots file encoding)
static constexpr ColorPoint classic_windspeed[] = {
  {   0.0f, {0xFF, 0xFF, 0xFF}},
  { 250.0f, {0x80, 0x80, 0xFF}},
  { 500.0f, {0x80, 0xFF, 0xFF}},
  { 750.0f, {0xFF, 0xFF, 0x80}},
  {1000.0f, {0xFF, 0x80, 0x80}},
  {1250.0f, {0xFF, 0x80, 0x80}},
  {2000.0f, {0xFF, 0xA0, 0xA0}},
  {3000.0f, {0xFF, 0xA0, 0xA0}},
  {4000.0f, {0xFF, 0x00, 0x00}},
  {5000.0f, {0xFF, 0x00, 0x00}},
  {6000.0f, {0xFF, 0x00, 0x00}},
  {7000.0f, {0xFF, 0x00, 0x00}},
  {8000.0f, {0xFF, 0x00, 0x00}},
};
static constexpr ColorMap classic_windspeed_map = {
  classic_windspeed, std::size(classic_windspeed)
};

// Blue to Yellow to red (boundary layer height, m)
static constexpr ColorPoint classic_height[] = {
  {    0.0f, {0xFF, 0xFF, 0xFF}},
  {  750.0f, {0x80, 0x80, 0xFF}},
  { 1500.0f, {0x80, 0xFF, 0xFF}},
  { 2250.0f, {0xFF, 0xFF, 0x80}},
  { 3000.0f, {0xFF, 0x80, 0x80}},
  { 3500.0f, {0xFF, 0x80, 0x80}},
  { 6000.0f, {0xFF, 0xA0, 0xA0}},
  { 8000.0f, {0xFF, 0xA0, 0xA0}},
  { 9000.0f, {0xFF, 0x00, 0x00}},
  { 9500.0f, {0xFF, 0x00, 0x00}},
  { 9600.0f, {0xFF, 0x00, 0x00}},
  { 9700.0f, {0xFF, 0x00, 0x00}},
  {20000.0f, {0xFF, 0x00, 0x00}},
};
static constexpr ColorMap classic_height_map = {
  classic_height, std::size(classic_height)
};

// Blue to Gray (cloud cover, percentage file encoding)
static constexpr ColorPoint classic_cloud[] = {
  {  0.0f, {  0, 153, 204}},
  { 12.0f, {102, 229, 255}},
  { 25.0f, {153, 255, 255}},
  { 37.0f, {204, 255, 255}},
  { 50.0f, {229, 229, 229}},
  { 62.0f, {173, 173, 173}},
  { 75.0f, {122, 122, 122}},
  {100.0f, { 81,  81,  81}},
  {5000.0f, { 71,  71,  71}},
  {6000.0f, {0xFF, 0x00, 0x00}},
  {7000.0f, {0xFF, 0x00, 0x00}},
  {8000.0f, {0xFF, 0x00, 0x00}},
  {9000.0f, {0xFF, 0x00, 0x00}},
};
static constexpr ColorMap classic_cloud_map = {
  classic_cloud, std::size(classic_cloud)
};

// Blue to orange to red (surface temperature, Fahrenheit)
static constexpr ColorPoint classic_sfctemp[] = {
  {  0.0f, {  7,  90, 255}},
  { 30.0f, { 50, 118, 255}},
  { 70.0f, { 89, 144, 255}},
  { 73.0f, {140, 178, 255}},
  { 76.0f, {191, 212, 255}},
  { 79.0f, {229, 238, 255}},
  { 82.0f, {247, 249, 255}},
  { 85.0f, {255, 255, 204}},
  { 88.0f, {255, 255, 153}},
  { 91.0f, {255, 255,   0}},
  { 95.0f, {255, 204,   0}},
  {100.0f, {255, 153,   0}},
  {120.0f, {255,   0,   0}},
};
static constexpr ColorMap classic_sfctemp_map = {
  classic_sfctemp, std::size(classic_sfctemp)
};

// Blue to white to red (convergence, cm/s file encoding)
static constexpr ColorPoint classic_convergence[] = {
  {   0.0f, {  7,  90, 255}},
  { 100.0f, { 50, 118, 255}},
  { 140.0f, { 89, 144, 255}},
  { 160.0f, {140, 178, 255}},
  { 180.0f, {191, 212, 255}},
  { 190.0f, {229, 238, 255}},
  { 200.0f, {247, 249, 255}},
  { 210.0f, {255, 255, 204}},
  { 220.0f, {255, 255, 153}},
  { 240.0f, {255, 255,   0}},
  { 260.0f, {255, 204,   0}},
  { 300.0f, {255, 153,   0}},
  {1000.0f, {255, 102,   0}},
};
static constexpr ColorMap classic_convergence_map = {
  classic_convergence, std::size(classic_convergence)
};

// ---- Thermalmap.info color schemes ----

// Vertical speed (m/s)
// h = m_s * 1000 + 5000
static constexpr ColorPoint verticalspeed_colors[] = {
  {-4.75f, { 74,  85, 213}},
  {-3.75f, { 89, 137, 255}},
  {-2.75f, { 24, 160, 255}},
  {-1.75f, { 55, 196, 250}},
  {-1.25f, { 13, 217, 248}},
  {-0.75f, {  0, 239, 243}},
  { 0.0f,  {255, 255, 255}},
  { 0.5f,  {255, 215,   0}},
  { 1.0f,  {255, 165,   0}},
  { 2.0f,  {255,  73,  50}},
  { 2.5f,  {255,  65, 110}},
  { 3.5f,  {250,  51, 204}},
  { 4.5f,  {163,   0, 211}},
};
static constexpr ColorMap verticalspeed_map = {
  verticalspeed_colors, std::size(verticalspeed_colors)
};

static constexpr ColorPoint verticalspeed_colors_alpha[] = {
  {-4.75f, { 74,  85, 213, 255}},
  {-3.75f, { 89, 137, 255, 255}},
  {-2.75f, { 24, 160, 255, 255}},
  {-1.75f, { 55, 196, 250, 200}},
  {-1.25f, { 13, 217, 248, 128}},
  {-0.75f, {  0, 239, 243,  80}},
  { 0.0f,  {255, 255, 255,   0}},
  { 0.5f,  {255, 215,   0, 100}},
  { 1.0f,  {255, 165,   0, 180}},
  { 2.0f,  {255,  73,  50, 225}},
  { 2.5f,  {255,  65, 110, 255}},
  { 3.5f,  {250,  51, 204, 255}},
  { 4.5f,  {163,   0, 211, 255}},
};
static constexpr ColorMap verticalspeed_alpha_map = {
  verticalspeed_colors_alpha,
  std::size(verticalspeed_colors_alpha)
};

// Thermal strength (m/s)
// h = m_s * 100
static constexpr ColorPoint thermalstrength_colors[] = {
  {0.74f, {255, 255, 255}},
  {0.75f, {  0, 128, 255}},
  {1.0f,  {  0, 160, 255}},
  {1.5f,  { 64, 224, 255}},
  {1.75f, { 64, 255, 255}},
  {2.0f,  { 64, 255, 192}},
  {2.5f,  {128, 255,  64}},
  {2.75f, {192, 255,  64}},
  {3.0f,  {255, 255,  64}},
  {3.25f, {255, 224,  64}},
  {4.0f,  {255,  32,  64}},
  {4.25f, {255,  96, 192}},
  {4.75f, {185,  39, 190}},
};
static constexpr ColorMap thermalstrength_map = {
  thermalstrength_colors, std::size(thermalstrength_colors)
};

// Thermal height (m)
// h = meters * 1
static constexpr ColorPoint thermalheight_colors[] = {
  { 199.0f, {255, 255, 255}},
  { 200.0f, { 65,  71, 173}},
  { 400.0f, { 71, 119, 239}},
  { 600.0f, { 56, 165, 251}},
  { 800.0f, { 27, 208, 213}},
  {1000.0f, { 38, 237, 166}},
  {1200.0f, {100, 253, 106}},
  {1400.0f, {164, 252,  60}},
  {1600.0f, {211, 232,  53}},
  {1800.0f, {245, 198,  58}},
  {2000.0f, {254, 153,  44}},
  {2200.0f, {243,  99,  21}},
  {2800.0f, {122,   4,   3}},
};
static constexpr ColorMap thermalheight_map = {
  thermalheight_colors, std::size(thermalheight_colors)
};

// Temperature (Celsius)
// h = celsius * 100 + 50
static constexpr ColorPoint temperature_colors[] = {
  {-50.0f, { 23,  16, 207}},
  { -9.0f, { 35,  63, 249}},
  { -6.0f, { 48, 120, 250}},
  { -3.0f, { 66, 178, 251}},
  {  0.0f, { 84, 235, 253}},
  {  3.0f, {106, 255, 222}},
  {  6.0f, {125, 242, 160}},
  { 12.0f, {210, 241,  74}},
  { 15.0f, {252, 233,  60}},
  { 18.0f, {248, 180,  47}},
  { 21.0f, {246, 144,  38}},
  { 30.0f, {244,  36,  23}},
  { 33.0f, {243,   0,  18}},
};
static constexpr ColorMap temperature_map = {
  temperature_colors, std::size(temperature_colors)
};

// Temperature normalized (Celsius)
// h = celsius * 100 + 10000
static constexpr ColorPoint temperature_norm_colors[] = {
  {-49.5f, { 23,  16, 207}},
  { -8.5f, { 35,  63, 249}},
  { -5.5f, { 48, 120, 250}},
  { -2.5f, { 66, 178, 251}},
  {  0.5f, { 84, 235, 253}},
  {  3.5f, {106, 255, 222}},
  {  6.5f, {125, 242, 160}},
  { 12.5f, {210, 241,  74}},
  { 15.5f, {252, 233,  60}},
  { 18.5f, {248, 180,  47}},
  { 21.5f, {246, 144,  38}},
  { 30.5f, {244,  36,  23}},
  { 33.5f, {243,   0,  18}},
};
static constexpr ColorMap temperature_norm_map = {
  temperature_norm_colors, std::size(temperature_norm_colors)
};

// Rain (mm)
// h = mm * 10
static constexpr ColorPoint rain_colors[] = {
  { 1.9f, {255, 255, 255}},
  { 2.0f, {103, 255, 254}},
  { 5.0f, { 88, 204, 252}},
  {10.0f, { 83, 204,  47}},
  {15.0f, {127, 255,  62}},
  {20.0f, {193, 255, 108}},
  {30.0f, {254, 254,  65}},
  {50.0f, {238, 131,  35}},
  {70.0f, {199,   0,  13}},
  {80.0f, {155,   0,   8}},
  {90.0f, {245,  54, 250}},
  {120.0f, {146,   4, 150}},
  {200.0f, { 57,  15, 249}},
};
static constexpr ColorMap rain_map = {
  rain_colors, std::size(rain_colors)
};

static constexpr ColorPoint rain_colors_alpha[] = {
  { 1.9f, {103, 255, 254,   0}},
  { 2.0f, {103, 255, 254,  64}},
  { 5.0f, { 88, 204, 252, 110}},
  {10.0f, { 83, 204,  47, 160}},
  {15.0f, {127, 255,  62, 200}},
  {20.0f, {193, 255, 108}},
  {30.0f, {254, 254,  65}},
  {50.0f, {238, 131,  35}},
  {70.0f, {199,   0,  13}},
  {80.0f, {155,   0,   8}},
  {90.0f, {245,  54, 250}},
  {120.0f, {146,   4, 150}},
  {200.0f, { 57,  15, 249}},
};
static constexpr ColorMap rain_alpha_map = {
  rain_colors_alpha, std::size(rain_colors_alpha)
};

// PFD (potential flight distance) - LS4, DuoDiscus (km)
// h = km * 1
static constexpr ColorPoint pfd_ls4_colors[] = {
  { 23.0f, {255, 255, 255}},
  { 24.0f, {255, 255, 255}},
  { 25.0f, { 33,  13,  41}},
  { 50.0f, { 48,  18,  59}},
  {100.0f, { 70,  98, 216}},
  {200.0f, { 53, 171, 248}},
  {300.0f, { 27, 229, 181}},
  {400.0f, {116, 254,  93}},
  {500.0f, {201, 239,  52}},
  {600.0f, {251, 185,  56}},
  {700.0f, {245, 105,  24}},
  {800.0f, {201,  41,   3}},
  {900.0f, {122,   4,   3}},
};
static constexpr ColorMap pfd_ls4_map = {
  pfd_ls4_colors, std::size(pfd_ls4_colors)
};

// BL Average Windspeed, Wind Shear (knots)
// h = knots * 100 + 100
static constexpr ColorPoint bl_avg_windspeed_colors[] = {
  { -1.0f, { 40, 140, 255}},
  {  1.0f, {  0, 220, 225}},
  {  3.0f, {  0, 234, 156}},
  {  5.0f, {240, 245,   3}},
  {  7.0f, {255, 219,   0}},
  {  9.0f, {255, 152,   0}},
  { 11.0f, {247, 120,   0}},
  { 13.0f, {224,  97,  40}},
  { 17.0f, {220,  81,  50}},
  { 23.0f, {205,  58,  70}},
  { 29.0f, {180,  26,  90}},
  { 35.0f, {150,  40, 120}},
  { 41.0f, {122,  44, 122}},
};
static constexpr ColorMap bl_avg_windspeed_map = {
  bl_avg_windspeed_colors, std::size(bl_avg_windspeed_colors)
};

// XC Speed - LS4, Duo Discus (km/h)
// h = km_h * 1
static constexpr ColorPoint xcspeed_ls4_colors[] = {
  { 58.0f, {255, 255, 255}},
  { 59.0f, {255, 255, 255}},
  { 60.0f, { 48,  18,  59}},
  { 70.0f, { 69,  91, 205}},
  { 80.0f, { 62, 156, 254}},
  { 90.0f, { 24, 215, 203}},
  {100.0f, { 72, 248, 130}},
  {110.0f, {164, 252,  60}},
  {120.0f, {226, 220,  56}},
  {130.0f, {254, 163,  49}},
  {140.0f, {239,  89,  17}},
  {150.0f, {194,  36,   3}},
  {160.0f, {122,   4,   3}},
};
static constexpr ColorMap xcspeed_ls4_map = {
  xcspeed_ls4_colors, std::size(xcspeed_ls4_colors)
};

// XC Speed - K8 (km/h)
// h = km_h * 1
static constexpr ColorPoint xcspeed_k8_colors[] = {
  {25.0f, {255, 255, 255}},
  {26.0f, {255, 255, 255}},
  {27.0f, {255, 255, 255}},
  {28.0f, {255, 255, 255}},
  {29.0f, {255, 255, 255}},
  {30.0f, { 24, 215, 203}},
  {40.0f, { 72, 248, 130}},
  {50.0f, {164, 252,  60}},
  {60.0f, {226, 220,  56}},
  {70.0f, {254, 163,  49}},
  {80.0f, {239,  89,  17}},
  {90.0f, {194,  36,   3}},
  {100.0f, {122,   4,   3}},
};
static constexpr ColorMap xcspeed_k8_map = {
  xcspeed_k8_colors, std::size(xcspeed_k8_colors)
};

// Surface heat flux (W/m^2)
// h = W_m2 * 10 + 10000
static constexpr ColorPoint surface_heat_flux_colors[] = {
  {-1000.0f, { 11,   5, 130}},
  { -450.0f, { 34,  43, 160}},
  { -350.0f, { 78, 119, 215}},
  { -300.0f, { 98, 157, 236}},
  { -250.0f, {122, 191, 252}},
  { -150.0f, {193, 236, 254}},
  {  -50.0f, {233, 252, 255}},
  {    0.0f, {244, 253, 233}},
  {  100.0f, {252, 242, 144}},
  {  150.0f, {251, 220, 106}},
  {  250.0f, {248, 183,  53}},
  {  350.0f, {244, 138,  39}},
  {  450.0f, {227,   0,  16}},
};
static constexpr ColorMap surface_heat_flux_map = {
  surface_heat_flux_colors, std::size(surface_heat_flux_colors)
};

// Sea level pressure (hPa)
// h = hPa * 20 + 1000
static constexpr ColorPoint sealevel_pressure_colors[] = {
  { 970.0f, { 11,   5, 130}},
  { 978.0f, { 77, 118, 215}},
  { 982.0f, {107, 174, 246}},
  { 986.0f, {155, 212, 253}},
  { 994.0f, {231, 251, 255}},
  { 998.0f, {246, 251, 215}},
  {1002.0f, {252, 245, 149}},
  {1006.0f, {250, 212,  92}},
  {1010.0f, {249, 185,  54}},
  {1018.0f, {237,  82,  30}},
  {1022.0f, {227,   0,  30}},
  {1026.0f, {227,   0, 102}},
  {1034.0f, {227,   0, 255}},
};
static constexpr ColorMap sealevel_pressure_map = {
  sealevel_pressure_colors, std::size(sealevel_pressure_colors)
};

// Cloud fraction low level (percent 0..100)
// h = percent * 10
static constexpr ColorPoint cloudfraction_low_colors[] = {
  { -0.2f, {255, 255, 255}},
  {  9.0f, {255, 255, 255}},
  {  9.1f, {234, 246, 232}},
  { 18.2f, {213, 238, 209}},
  { 27.3f, {191, 229, 186}},
  { 36.4f, {166, 218, 163}},
  { 45.5f, {134, 202, 140}},
  { 54.5f, {102, 187, 118}},
  { 63.6f, { 69, 171,  96}},
  { 72.7f, { 49, 147,  78}},
  { 81.8f, { 33, 121,  61}},
  { 90.9f, { 16,  94,  44}},
  {100.0f, {  0,  68,  27}},
};
static constexpr ColorMap cloudfraction_low_map = {
  cloudfraction_low_colors, std::size(cloudfraction_low_colors)
};

static constexpr ColorPoint
    cloudfraction_low_colors_alpha[] = {
  { -0.2f, {255, 255, 255,   0}},
  {  9.0f, {255, 255, 255,  40}},
  {  9.1f, {234, 246, 232,  75}},
  { 18.2f, {213, 238, 209, 150}},
  { 27.3f, {191, 229, 186, 200}},
  { 36.4f, {166, 218, 163, 220}},
  { 45.5f, {134, 202, 140}},
  { 54.5f, {102, 187, 118}},
  { 63.6f, { 69, 171,  96}},
  { 72.7f, { 49, 147,  78}},
  { 81.8f, { 33, 121,  61}},
  { 90.9f, { 16,  94,  44}},
  {100.0f, {  0,  68,  27}},
};
static constexpr ColorMap cloudfraction_low_alpha_map = {
  cloudfraction_low_colors_alpha,
  std::size(cloudfraction_low_colors_alpha)
};

// Cloud fraction mid level (percent 0..100)
// h = percent * 10
static constexpr ColorPoint cloudfraction_mid_colors[] = {
  {  0.0f, {255, 255, 255}},
  {  9.0f, {255, 255, 255}},
  {  9.1f, {233, 242, 248}},
  { 18.2f, {211, 230, 242}},
  { 27.3f, {189, 217, 235}},
  { 36.4f, {163, 202, 227}},
  { 45.5f, {132, 184, 218}},
  { 54.5f, {101, 165, 208}},
  { 63.6f, { 70, 147, 199}},
  { 72.7f, { 51, 123, 178}},
  { 81.8f, { 37,  98, 155}},
  { 90.9f, { 22,  73, 131}},
  {100.0f, {  8,  48, 107}},
};
static constexpr ColorMap cloudfraction_mid_map = {
  cloudfraction_mid_colors, std::size(cloudfraction_mid_colors)
};

static constexpr ColorPoint
    cloudfraction_mid_colors_alpha[] = {
  {  0.0f, {255, 255, 255,   0}},
  {  9.0f, {255, 255, 255, 100}},
  { 20.0f, {233, 242, 248, 150}},
  { 42.0f, {211, 230, 242, 200}},
  { 48.0f, {189, 217, 235, 255}},
  { 55.0f, {163, 202, 227}},
  { 61.5f, {132, 184, 218}},
  { 68.5f, {101, 165, 208}},
  { 73.6f, { 70, 147, 199}},
  { 77.7f, { 51, 123, 178}},
  { 81.8f, { 37,  98, 155}},
  { 90.9f, { 22,  73, 131}},
  {100.0f, {  8,  48, 107}},
};
static constexpr ColorMap cloudfraction_mid_alpha_map = {
  cloudfraction_mid_colors_alpha,
  std::size(cloudfraction_mid_colors_alpha)
};

// Cloud fraction high level (percent 0..100)
// h = percent * 10 - 3
static constexpr ColorPoint
    cloudfraction_high_colors[] = {
  {  0.3f, {255, 255, 255}},
  {  9.3f, {255, 255, 255}},
  {  9.4f, {254, 230, 222}},
  { 18.5f, {253, 205, 188}},
  { 27.6f, {253, 180, 155}},
  { 36.7f, {250, 153, 125}},
  { 45.8f, {245, 123,  99}},
  { 54.8f, {240,  93,  74}},
  { 63.9f, {235,  63,  49}},
  { 73.0f, {208,  44,  36}},
  { 82.1f, {173,  29,  29}},
  { 91.2f, {138,  15,  21}},
  {100.3f, {103,   0,  13}},
};
static constexpr ColorMap cloudfraction_high_map = {
  cloudfraction_high_colors,
  std::size(cloudfraction_high_colors)
};

static constexpr ColorPoint
    cloudfraction_high_colors_alpha[] = {
  {  0.3f, {255, 255, 255,   0}},
  {  9.3f, {254, 230, 222,  64}},
  {  9.4f, {254, 230, 222,  64}},
  { 18.5f, {253, 205, 188, 125}},
  { 27.6f, {253, 180, 155, 175}},
  { 36.7f, {250, 153, 125, 200}},
  { 45.8f, {245, 123,  99, 225}},
  { 54.8f, {240,  93,  74}},
  { 63.9f, {235,  63,  49}},
  { 73.0f, {208,  44,  36}},
  { 82.1f, {173,  29,  29}},
  { 91.2f, {138,  15,  21}},
  {100.3f, {103,   0,  13}},
};
static constexpr ColorMap
    cloudfraction_high_alpha_map = {
  cloudfraction_high_colors_alpha,
  std::size(cloudfraction_high_colors_alpha)
};

// Cloud fraction accumulated (percent 0..100)
// h = percent * 10
static constexpr ColorPoint
    cloudfraction_accumulated_colors[] = {
  {  0.0f, {255, 255, 255}},
  {  8.3f, {241, 241, 241}},
  { 16.7f, {228, 228, 228}},
  { 25.0f, {214, 214, 214}},
  { 33.3f, {200, 200, 200}},
  { 41.7f, {187, 187, 187}},
  { 50.0f, {173, 173, 173}},
  { 58.3f, {159, 159, 159}},
  { 66.7f, {146, 146, 146}},
  { 75.0f, {132, 132, 132}},
  { 83.3f, {118, 118, 118}},
  { 91.7f, {105, 105, 105}},
  {100.0f, { 91,  91,  91}},
};
static constexpr ColorMap
    cloudfraction_accumulated_map = {
  cloudfraction_accumulated_colors,
  std::size(cloudfraction_accumulated_colors)
};

static constexpr ColorPoint
    cloudfraction_accumulated_colors_alpha[] = {
  {  0.0f, {255, 255, 255,   0}},
  {  8.3f, {255, 255, 255,  64}},
  { 16.7f, {255, 255, 255, 100}},
  { 25.0f, {255, 255, 255, 125}},
  { 33.3f, {255, 255, 255, 150}},
  { 41.7f, {255, 255, 255, 175}},
  { 50.0f, {173, 173, 173, 200}},
  { 58.3f, {159, 159, 159, 255}},
  { 65.7f, {146, 146, 146}},
  { 72.0f, {132, 132, 132}},
  { 80.0f, {118, 118, 118}},
  { 88.3f, {105, 105, 105}},
  {100.0f, { 91,  91,  91}},
};
static constexpr ColorMap
    cloudfraction_accumulated_alpha_map = {
  cloudfraction_accumulated_colors_alpha,
  std::size(cloudfraction_accumulated_colors_alpha)
};

/* RASP styles
   List of styles, first Blipmap classic, then thermalmap.info
   List of tuples:
     * string fieldname,
     * ColorMap (RGB colors)
     * ColorMap (RGBA colors, empty if no alpha)
     * float scale, float offset (value transform)
     * int height_scale = value range level
     * bool do_water enables "water masking" */

const RaspStyle rasp_styles[] = {
  MakeRaspStyle("wstar",
                classic_vertical_speed_map, 100, 200,
                false, UnitGroup::VERTICAL_SPEED),
  // dimensionless buoyancy/shear ratio: no unit
  MakeRaspStyle("wstar_bsratio",
                classic_vertical_speed_map, 100, 200),
  // knots file encoding, conversion to m/s undefined: no unit
  {"blwindspd",
   classic_windspeed_map, {}, 1, 0,
   3, false},
  {"hbl",
   classic_height_map, {}, 1, 0,
   4, false, UnitGroup::ALTITUDE},
  {"dwcrit",
   classic_height_map, {}, 1, 0,
   4, false, UnitGroup::ALTITUDE},
  {"blcloudpct",
   classic_cloud_map, {}, 1, 0,
   0, true},
  {"sfctemp",
   classic_sfctemp_map, {}, 1, 0,
   0, false, UnitGroup::TEMPERATURE,
   FAHRENHEIT_TO_KELVIN_SCALE, FAHRENHEIT_TO_KELVIN_OFFSET},
  {"hwcrit",
   classic_height_map, {}, 1, 0,
   4, false, UnitGroup::ALTITUDE},
  {"wblmaxmin",
   classic_convergence_map, {}, 1, 0,
   1, false, UnitGroup::VERTICAL_SPEED, CMS_TO_MS},
  {"blcwbase",
   classic_height_map, {}, 1, 0,
   4, false, UnitGroup::ALTITUDE},

  // Thermalmap.info styles
  MakeRaspStyle("ThermalStrength",
                thermalstrength_map, 100, 0,
                false, UnitGroup::VERTICAL_SPEED),
  MakeRaspStyle("ThermalHeight",
                thermalheight_map, 1, 0,
                false, UnitGroup::ALTITUDE),
  MakeRaspStyle("Convergence",
                verticalspeed_map, verticalspeed_alpha_map,
                1000, 5000, false, UnitGroup::VERTICAL_SPEED),
  MakeRaspStyle("Wave_1000m",
                verticalspeed_map, verticalspeed_alpha_map,
                1000, 5000, false, UnitGroup::VERTICAL_SPEED),
  MakeRaspStyle("Wave_2000m",
                verticalspeed_map, verticalspeed_alpha_map,
                1000, 5000, false, UnitGroup::VERTICAL_SPEED),
  MakeRaspStyle("Wave_3000m",
                verticalspeed_map, verticalspeed_alpha_map,
                1000, 5000, false, UnitGroup::VERTICAL_SPEED),
  MakeRaspStyle("Wave_4000m",
                verticalspeed_map, verticalspeed_alpha_map,
                1000, 5000, false, UnitGroup::VERTICAL_SPEED),
  MakeRaspStyle("Wave_5000m",
                verticalspeed_map, verticalspeed_alpha_map,
                1000, 5000, false, UnitGroup::VERTICAL_SPEED),
  MakeRaspStyle("Wave_6000m",
                verticalspeed_map, verticalspeed_alpha_map,
                1000, 5000, false, UnitGroup::VERTICAL_SPEED),
  MakeRaspStyle("Wave_7000m",
                verticalspeed_map, verticalspeed_alpha_map,
                1000, 5000, false, UnitGroup::VERTICAL_SPEED),
  MakeRaspStyle("Temperature2m",
                temperature_map, 100, 50,
                false, UnitGroup::TEMPERATURE, 1, CELSIUS_TO_KELVIN),
  MakeRaspStyle("Rain",
                rain_map, rain_alpha_map, 10, 0),
  MakeRaspStyle("PFD_Day_DuoDiscus",
                pfd_ls4_map, 1, 0,
                false, UnitGroup::DISTANCE, KM_TO_M),
  MakeRaspStyle("PFD_Day_LS4",
                pfd_ls4_map, 1, 0,
                false, UnitGroup::DISTANCE, KM_TO_M),
  MakeRaspStyle("PFD_Day_K8",
                pfd_ls4_map, 1, 0,
                false, UnitGroup::DISTANCE, KM_TO_M),
  MakeRaspStyle("Windspeed_10m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_500m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_1000m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_1500m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_2000m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_2500m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_3000m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_3500m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_4000m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_4500m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_5000m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_5500m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_6000m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_6500m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("Windspeed_7000m",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  // wind shear in knots-domain, but not a wind speed: no unit
  MakeRaspStyle("VerticalWindShear",
                bl_avg_windspeed_map, 100, 100),
  MakeRaspStyle("BL_AverageWindSpeed",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("XCSpeed_LS4",
                xcspeed_ls4_map, 1, 0,
                false, UnitGroup::TASK_SPEED, KMH_TO_MS),
  MakeRaspStyle("XCSpeed_DuoDiscus",
                xcspeed_ls4_map, 1, 0,
                false, UnitGroup::TASK_SPEED, KMH_TO_MS),
  MakeRaspStyle("XCSpeed_K8",
                xcspeed_k8_map, 1, 0,
                false, UnitGroup::TASK_SPEED, KMH_TO_MS),
  MakeRaspStyle("SurfaceHeatFlux",
                surface_heat_flux_map, 10, 10000),
  MakeRaspStyle("SeaLevelPressure",
                sealevel_pressure_map, 20, 1000,
                false, UnitGroup::PRESSURE),
  MakeRaspStyle("Cloudfraction_Low",
                cloudfraction_low_map,
                cloudfraction_low_alpha_map,
                10, 0),
  MakeRaspStyle("Cloudfraction_Mid",
                cloudfraction_mid_map,
                cloudfraction_mid_alpha_map,
                10, 0),
  MakeRaspStyle("Cloudfraction_High",
                cloudfraction_high_map,
                cloudfraction_high_alpha_map,
                10, -3),
  MakeRaspStyle("Cloudfraction_Accumulated",
                cloudfraction_accumulated_map,
                cloudfraction_accumulated_alpha_map,
                10, 0),

  // Sentinel: default style for unknown field names
  {nullptr,
   classic_vertical_speed_map, {}, 1, 0,
   2, false},
};

const RaspStyle rasp_colormaps_general[] = {
  // Thermalmap.info color scheme codes
  MakeRaspStyle("vth",
                thermalstrength_map, 100, 0,
                false, UnitGroup::VERTICAL_SPEED),
  MakeRaspStyle("hth",
                thermalheight_map, 1, 0,
                false, UnitGroup::ALTITUDE),
  MakeRaspStyle("vve",
                verticalspeed_map, verticalspeed_alpha_map,
                1000, 5000, false, UnitGroup::VERTICAL_SPEED),
  MakeRaspStyle("tce",
                temperature_norm_map, 100, 10000,
                false, UnitGroup::TEMPERATURE, 1, CELSIUS_TO_KELVIN),
  MakeRaspStyle("prr",
                rain_map, rain_alpha_map, 10, 0),
  MakeRaspStyle("pfd",
                pfd_ls4_map, 1, 0,
                false, UnitGroup::DISTANCE, KM_TO_M),
  MakeRaspStyle("vho",
                bl_avg_windspeed_map, 100, 100,
                false, UnitGroup::WIND_SPEED, KNOTS_TO_MS),
  MakeRaspStyle("vxc",
                xcspeed_ls4_map, 1, 0,
                false, UnitGroup::TASK_SPEED, KMH_TO_MS),
  MakeRaspStyle("vx8",
                xcspeed_k8_map, 1, 0,
                false, UnitGroup::TASK_SPEED, KMH_TO_MS),
  MakeRaspStyle("hfl",
                surface_heat_flux_map, 10, 10000),
  MakeRaspStyle("pre",
                sealevel_pressure_map, 20, 1000,
                false, UnitGroup::PRESSURE),
  MakeRaspStyle("fcl",
                cloudfraction_low_map,
                cloudfraction_low_alpha_map,
                10, 0),
  MakeRaspStyle("fcm",
                cloudfraction_mid_map,
                cloudfraction_mid_alpha_map,
                10, 0),
  MakeRaspStyle("fch",
                cloudfraction_high_map,
                cloudfraction_high_alpha_map,
                10, -3),
  MakeRaspStyle("fct",
                cloudfraction_accumulated_map,
                cloudfraction_accumulated_alpha_map,
                10, 0),

  // Sentinel
  {nullptr,
   classic_vertical_speed_map, {}, 1, 0,
   2, false},
};
