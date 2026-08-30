// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageSetting.hpp"
#include "PageSettingDescriptor.hpp"
#include "util/Macros.hpp"
#include "PageSettings.hpp"
#include "ActionInterface.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "MapSettings.hpp"
#include "Profile/Current.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Terrain/TerrainDisplayChoices.hpp"
#include "Terrain/TerrainSettings.hpp"
#include "UISettings.hpp"

#include <cassert>

bool
PageSettingOverrides::Contains(PageSettingId id) const noexcept
{
  for (unsigned i = 0; i < n_items; ++i)
    if (items[i].id == id)
      return true;
  return false;
}

int *
PageSettingOverrides::FindValue(PageSettingId id) noexcept
{
  for (unsigned i = 0; i < n_items; ++i)
    if (items[i].id == id)
      return &items[i].value;
  return nullptr;
}

const int *
PageSettingOverrides::FindValue(PageSettingId id) const noexcept
{
  for (unsigned i = 0; i < n_items; ++i)
    if (items[i].id == id)
      return &items[i].value;
  return nullptr;
}

bool
PageSettingOverrides::Add(PageSettingId id, int value) noexcept
{
  if (Contains(id))
    return false;
  if (n_items >= MAX_ITEMS)
    return false;

  items[n_items++] = {id, value};
  return true;
}

bool
PageSettingOverrides::Remove(PageSettingId id) noexcept
{
  for (unsigned i = 0; i < n_items; ++i) {
    if (items[i].id != id)
      continue;

    for (unsigned j = i + 1; j < n_items; ++j)
      items[j - 1] = items[j];
    --n_items;
    return true;
  }
  return false;
}

void
PageSettingOverrides::SetValue(PageSettingId id, int value) noexcept
{
  if (int *v = FindValue(id); v != nullptr) {
    *v = value;
    return;
  }

  Add(id, value);
}

namespace {

/* --- Terrain enable ------------------------------------------------ */

[[nodiscard]]
int
TerrainEnableGetLive() noexcept
{
  return CommonInterface::GetMapSettings().terrain.enable ? 1 : 0;
}

void
TerrainEnableSetLive(int value) noexcept
{
  CommonInterface::SetMapSettings().terrain.enable = value != 0;
}

void
TerrainEnableSaveGlobalProfile(int value) noexcept
{
  Profile::Set(ProfileKeys::DrawTerrain, value != 0);
}

[[nodiscard]]
int
TerrainEnableLoadGlobalProfile() noexcept
{
  bool enable = true;
  Profile::Get(ProfileKeys::DrawTerrain, enable);
  return enable ? 1 : 0;
}

/* --- Topography ---------------------------------------------------- */

[[nodiscard]]
int
TopographyEnableGetLive() noexcept
{
  return CommonInterface::GetMapSettings().topography_enabled ? 1 : 0;
}

void
TopographyEnableSetLive(int value) noexcept
{
  CommonInterface::SetMapSettings().topography_enabled = value != 0;
}

void
TopographyEnableSaveGlobalProfile(int value) noexcept
{
  Profile::Set(ProfileKeys::DrawTopography, value != 0);
}

[[nodiscard]]
int
TopographyEnableLoadGlobalProfile() noexcept
{
  bool enable = true;
  Profile::Get(ProfileKeys::DrawTopography, enable);
  return enable ? 1 : 0;
}

/* --- Terrain colors ------------------------------------------------ */

[[nodiscard]]
int
TerrainColorsGetLive() noexcept
{
  return CommonInterface::GetMapSettings().terrain.ramp;
}

void
TerrainColorsSetLive(int value) noexcept
{
  if (value < 0 ||
      unsigned(value) >= TerrainRendererSettings::NUM_RAMPS)
    return;

  CommonInterface::SetMapSettings().terrain.ramp = unsigned(value);
}

void
TerrainColorsSaveGlobalProfile(int value) noexcept
{
  if (value < 0 ||
      unsigned(value) >= TerrainRendererSettings::NUM_RAMPS)
    return;

  Profile::Set(ProfileKeys::TerrainRamp, unsigned(value));
}

[[nodiscard]]
int
TerrainColorsLoadGlobalProfile() noexcept
{
  unsigned ramp = 0;
  if (!Profile::Get(ProfileKeys::TerrainRamp, ramp) ||
      ramp >= TerrainRendererSettings::NUM_RAMPS)
    ramp = 0;
  return int(ramp);
}

/* --- Slope shading ------------------------------------------------- */

[[nodiscard]]
int
TerrainSlopeGetLive() noexcept
{
  return int(CommonInterface::GetMapSettings().terrain.slope_shading);
}

void
TerrainSlopeSetLive(int value) noexcept
{
  if (value < 0 || value >= int(SlopeShading::COUNT))
    return;

  CommonInterface::SetMapSettings().terrain.slope_shading =
    SlopeShading(value);
}

void
TerrainSlopeSaveGlobalProfile(int value) noexcept
{
  if (value < 0 || value >= int(SlopeShading::COUNT))
    return;

  Profile::SetEnum(ProfileKeys::SlopeShadingType, SlopeShading(value));
}

[[nodiscard]]
int
TerrainSlopeLoadGlobalProfile() noexcept
{
  uint8_t temp = uint8_t(SlopeShading::FIXED);
  if (!Profile::Get(ProfileKeys::SlopeShadingType, temp) ||
      temp >= uint8_t(SlopeShading::COUNT))
    temp = uint8_t(SlopeShading::FIXED);
  return int(temp);
}

/* --- Contrast / brightness (percent in catalog, byte in MapSettings) */

[[nodiscard]]
int
TerrainContrastGetLive() noexcept
{
  return TerrainByteToPercent(
    CommonInterface::GetMapSettings().terrain.contrast);
}

void
TerrainContrastSetLive(int value) noexcept
{
  if (value < 0 || value > 100)
    return;

  CommonInterface::SetMapSettings().terrain.contrast =
    TerrainPercentToByte(short(value));
}

void
TerrainContrastSaveGlobalProfile(int value) noexcept
{
  if (value < 0 || value > 100)
    return;

  Profile::Set(ProfileKeys::TerrainContrast,
               TerrainPercentToByte(short(value)));
}

[[nodiscard]]
int
TerrainContrastLoadGlobalProfile() noexcept
{
  short contrast = 65;
  Profile::Get(ProfileKeys::TerrainContrast, contrast);
  return TerrainByteToPercent(contrast);
}

[[nodiscard]]
int
TerrainBrightnessGetLive() noexcept
{
  return TerrainByteToPercent(
    CommonInterface::GetMapSettings().terrain.brightness);
}

void
TerrainBrightnessSetLive(int value) noexcept
{
  if (value < 0 || value > 100)
    return;

  CommonInterface::SetMapSettings().terrain.brightness =
    TerrainPercentToByte(short(value));
}

void
TerrainBrightnessSaveGlobalProfile(int value) noexcept
{
  if (value < 0 || value > 100)
    return;

  Profile::Set(ProfileKeys::TerrainBrightness,
               TerrainPercentToByte(short(value)));
}

[[nodiscard]]
int
TerrainBrightnessLoadGlobalProfile() noexcept
{
  short brightness = 192;
  Profile::Get(ProfileKeys::TerrainBrightness, brightness);
  return TerrainByteToPercent(brightness);
}

/* --- Contours ------------------------------------------------------ */

[[nodiscard]]
int
TerrainContoursGetLive() noexcept
{
  return int(CommonInterface::GetMapSettings().terrain.contours);
}

void
TerrainContoursSetLive(int value) noexcept
{
  if (value < 0 || value >= int(Contours::COUNT))
    return;

  CommonInterface::SetMapSettings().terrain.contours = Contours(value);
}

void
TerrainContoursSaveGlobalProfile(int value) noexcept
{
  if (value < 0 || value >= int(Contours::COUNT))
    return;

  Profile::SetEnum(ProfileKeys::TerrainContours, Contours(value));
}

[[nodiscard]]
int
TerrainContoursLoadGlobalProfile() noexcept
{
  uint8_t temp = uint8_t(Contours::OFF);
  if (!Profile::Get(ProfileKeys::TerrainContours, temp) ||
      temp >= uint8_t(Contours::COUNT))
    temp = uint8_t(Contours::OFF);
  return int(temp);
}

/**
 * Map Display → Terrain catalog (all seven panel settings).
 * Choice tables shared with TerrainDisplayConfigPanel.
 */
static constexpr PageSettingDescriptor registry[] = {
  {
    PageSettingId::TERRAIN_ENABLE,
    PageSettingType::BOOL,
    N_("Terrain Display"),
    N_("Show terrain on this page. Global uses Map Display → Terrain."),
    "OverrideTerrainEnable",
    terrain_enable_choices,
    0, 0, 0,
    TerrainEnableGetLive,
    TerrainEnableSetLive,
    TerrainEnableSaveGlobalProfile,
    TerrainEnableLoadGlobalProfile,
  },
  {
    PageSettingId::TOPOGRAPHY_ENABLE,
    PageSettingType::BOOL,
    N_("Topography display"),
    N_("Show topography on this page. Global uses Map Display → Terrain."),
    "OverrideTopographyEnable",
    terrain_enable_choices,
    0, 0, 0,
    TopographyEnableGetLive,
    TopographyEnableSetLive,
    TopographyEnableSaveGlobalProfile,
    TopographyEnableLoadGlobalProfile,
  },
  {
    PageSettingId::TERRAIN_COLORS,
    PageSettingType::ENUM,
    N_("Terrain colors"),
    N_("Color ramp for terrain on this page. "
       "Global uses Map Display → Terrain."),
    "OverrideTerrainColors",
    terrain_ramp_choices,
    0, 0, 0,
    TerrainColorsGetLive,
    TerrainColorsSetLive,
    TerrainColorsSaveGlobalProfile,
    TerrainColorsLoadGlobalProfile,
  },
  {
    PageSettingId::TERRAIN_SLOPE_SHADING,
    PageSettingType::ENUM,
    N_("Slope shading"),
    N_("Terrain slope shading on this page. "
       "Global uses Map Display → Terrain."),
    "OverrideTerrainSlopeShading",
    terrain_slope_shading_choices,
    0, 0, 0,
    TerrainSlopeGetLive,
    TerrainSlopeSetLive,
    TerrainSlopeSaveGlobalProfile,
    TerrainSlopeLoadGlobalProfile,
  },
  {
    PageSettingId::TERRAIN_CONTRAST,
    PageSettingType::INTEGER,
    N_("Terrain contrast"),
    N_("Terrain contrast on this page (percent). "
       "Global uses Map Display → Terrain."),
    "OverrideTerrainContrast",
    nullptr,
    0, 100, 5,
    TerrainContrastGetLive,
    TerrainContrastSetLive,
    TerrainContrastSaveGlobalProfile,
    TerrainContrastLoadGlobalProfile,
  },
  {
    PageSettingId::TERRAIN_BRIGHTNESS,
    PageSettingType::INTEGER,
    N_("Terrain brightness"),
    N_("Terrain brightness on this page (percent). "
       "Global uses Map Display → Terrain."),
    "OverrideTerrainBrightness",
    nullptr,
    0, 100, 5,
    TerrainBrightnessGetLive,
    TerrainBrightnessSetLive,
    TerrainBrightnessSaveGlobalProfile,
    TerrainBrightnessLoadGlobalProfile,
  },
  {
    PageSettingId::TERRAIN_CONTOURS,
    PageSettingType::ENUM,
    N_("Contours"),
    N_("Terrain contour lines on this page. "
       "Global uses Map Display → Terrain."),
    "OverrideTerrainContours",
    terrain_contours_choices,
    0, 0, 0,
    TerrainContoursGetLive,
    TerrainContoursSetLive,
    TerrainContoursSaveGlobalProfile,
    TerrainContoursLoadGlobalProfile,
  },
};

static_assert(ARRAY_SIZE(registry) == unsigned(PageSettingId::COUNT),
              "Registry size must match PageSettingId::COUNT");

} // namespace

namespace PageSettingRegistry {

unsigned
Count() noexcept
{
  return unsigned(PageSettingId::COUNT);
}

const PageSettingDescriptor &
Get(PageSettingId id) noexcept
{
  assert(unsigned(id) < unsigned(PageSettingId::COUNT));
  return registry[unsigned(id)];
}

const PageSettingDescriptor &
Get(unsigned index) noexcept
{
  assert(index < unsigned(PageSettingId::COUNT));
  return registry[index];
}

bool
IsValidValue(const PageSettingDescriptor &desc, int value) noexcept
{
  if (value == PageSettingOverrides::INHERIT)
    return true;

  if (desc.type == PageSettingType::INTEGER) {
    if (value < desc.int_min || value > desc.int_max)
      return false;
    if (desc.int_step <= 0)
      return true;
    return (value - desc.int_min) % desc.int_step == 0;
  }

  assert(desc.choices != nullptr);
  for (const StaticEnumChoice *c = desc.choices;
       c->display_string != nullptr; ++c)
    if (int(c->id) == value)
      return true;
  return false;
}

} // namespace PageSettingRegistry

void
PageSettingNotifyLive() noexcept
{
  ActionInterface::SendMapSettings(true);
}

void
PageSettingApply(PageSettingId id, int value,
                 std::optional<unsigned> page_index) noexcept
{
  const auto &desc = PageSettingRegistry::Get(id);
  if (!PageSettingRegistry::IsValidValue(desc, value)) {
    LogFmt("perPage: Apply reject id={} value={} (invalid)",
           unsigned(id), value);
    return;
  }

  if (!page_index.has_value()) {
    if (value == PageSettingOverrides::INHERIT)
      value = desc.LoadGlobalProfile();
    LogFmt("perPage: Apply global '{}' value={}", desc.label, value);
    desc.SetLive(value);
    desc.SaveGlobalProfile(value);
    PageSettingNotifyLive();
    return;
  }

  auto &pages = CommonInterface::SetUISettings().pages;
  if (*page_index >= PageSettings::MAX_PAGES) {
    LogFmt("perPage: Apply page reject index={} id={}",
           *page_index, unsigned(id));
    return;
  }

  LogFmt("perPage: Apply page={} '{}' value={}",
         *page_index, desc.label, value);
  pages.overrides[*page_index].SetValue(id, value);
}

void
PageSettingApplyGlobalBaseline() noexcept
{
  LogFmt("perPage: ApplyGlobalBaseline ({} settings)",
         PageSettingRegistry::Count());
  for (unsigned i = 0; i < PageSettingRegistry::Count(); ++i) {
    const auto &desc = PageSettingRegistry::Get(i);
    const int value = desc.LoadGlobalProfile();
    const int live = desc.GetLive();
    LogFmt("perPage:   baseline '{}' = {} (live was {})",
           desc.label, value, live);
    desc.SetLive(value);
  }
}

void
PageSettingApplyPageOverrides(unsigned page_index) noexcept
{
  if (page_index >= PageSettings::MAX_PAGES) {
    LogFmt("perPage: ApplyPageOverrides skip bad index={}", page_index);
    return;
  }

  const auto &overrides =
    CommonInterface::GetUISettings().pages.overrides[page_index];

  LogFmt("perPage: ApplyPageOverrides page={} n_items={}",
         page_index, overrides.n_items);

  for (unsigned i = 0; i < overrides.n_items; ++i) {
    const auto &item = overrides.items[i];
    const auto &desc = PageSettingRegistry::Get(item.id);

    if (item.value == PageSettingOverrides::INHERIT) {
      LogFmt("perPage:   override '{}' inherit (skip)", desc.label);
      continue;
    }

    if (!PageSettingRegistry::IsValidValue(desc, item.value)) {
      LogFmt("perPage:   override '{}' value={} invalid (skip)",
             desc.label, item.value);
      continue;
    }

    LogFmt("perPage:   override '{}' = {}", desc.label, item.value);
    desc.SetLive(item.value);
  }
}
