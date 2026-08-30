// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <optional>
#include <type_traits>

/**
 * Identifiers for settings that may be overridden per page.
 * The registry in PageSetting.cpp is the catalog (Map Display →
 * Terrain pilot group first).
 */
enum class PageSettingId : uint8_t {
  TERRAIN_ENABLE = 0,
  TOPOGRAPHY_ENABLE,
  TERRAIN_COLORS,
  TERRAIN_SLOPE_SHADING,
  TERRAIN_CONTRAST,
  TERRAIN_BRIGHTNESS,
  TERRAIN_CONTOURS,

  COUNT
};

/**
 * Sparse per-page setting overrides.  Only entries present in #items
 * appear in the Pages editor; value #INHERIT means "use the global
 * setting" while keeping the field on this page.
 *
 * #MAX_ITEMS caps overrides per page; it is independent of catalog
 * #PageSettingId::COUNT.
 */
struct PageSettingOverrides {
  static constexpr unsigned MAX_ITEMS = 32;

  /** Sentinel: follow the global / profile value. */
  static constexpr int INHERIT = -1;

  struct Item {
    PageSettingId id;
    int value;
  };

  Item items[MAX_ITEMS];
  unsigned n_items;

  constexpr void Clear() noexcept {
    n_items = 0;
  }

  [[nodiscard]]
  constexpr bool IsEmpty() const noexcept {
    return n_items == 0;
  }

  [[nodiscard]]
  bool Contains(PageSettingId id) const noexcept;

  [[nodiscard]]
  int *FindValue(PageSettingId id) noexcept;

  [[nodiscard]]
  const int *FindValue(PageSettingId id) const noexcept;

  /**
   * Add @p id if missing.  New entries default to #INHERIT.
   * @return true when a new entry was added
   */
  bool Add(PageSettingId id, int value = INHERIT) noexcept;

  bool Remove(PageSettingId id) noexcept;

  void SetValue(PageSettingId id, int value) noexcept;

  [[nodiscard]]
  constexpr bool operator==(const PageSettingOverrides &other) const noexcept {
    if (n_items != other.n_items)
      return false;
    for (unsigned i = 0; i < n_items; ++i)
      if (items[i].id != other.items[i].id ||
          items[i].value != other.items[i].value)
        return false;
    return true;
  }

  [[nodiscard]]
  constexpr bool operator!=(const PageSettingOverrides &other) const noexcept {
    return !(*this == other);
  }
};

static_assert(std::is_trivial_v<PageSettingOverrides>);
static_assert(PageSettingOverrides::MAX_ITEMS >=
              unsigned(PageSettingId::COUNT),
              "MAX_ITEMS must allow one of each catalog setting");

struct PageSettingDescriptor;

/**
 * Catalog of page-applicable settings (labels, choices, apply).
 */
namespace PageSettingRegistry {

[[nodiscard]]
unsigned
Count() noexcept;

[[nodiscard]]
const PageSettingDescriptor &
Get(PageSettingId id) noexcept;

[[nodiscard]]
const PageSettingDescriptor &
Get(unsigned index) noexcept;

[[nodiscard]]
bool
IsValidValue(const PageSettingDescriptor &desc, int value) noexcept;

} // namespace PageSettingRegistry

/**
 * Apply a setting value.
 *
 * @param page_index nullopt writes the global MapSettings (+ profile)
 *   and notifies the map; a page index writes into that page's
 *   override list only (live apply happens on page switch).
 */
void
PageSettingApply(PageSettingId id, int value,
                 std::optional<unsigned> page_index = std::nullopt) noexcept;

/**
 * Reload live MapSettings from the global profile for all catalog
 * settings (no map notify).  Pair with #PageSettingApplyPageOverrides
 * then #PageSettingNotifyLive.
 */
void
PageSettingApplyGlobalBaseline() noexcept;

/**
 * Apply sparse overrides for @p page_index onto live MapSettings
 * (no map notify).  #INHERIT entries are skipped.
 */
void
PageSettingApplyPageOverrides(unsigned page_index) noexcept;

/** Push live MapSettings to the map (one FullRedraw). */
void
PageSettingNotifyLive() noexcept;
