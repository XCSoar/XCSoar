// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSettings.hpp"

class Button;
class WndProperty;

/**
 * Draft Layer/Time (Level/Altitude) for the current map page on Info →
 * Weather panels. Apply commits when dirty; Add page copies the draft.
 */
namespace WeatherOverlayDraft {

struct State {
  PageLayout draft = PageLayout::Default();
  PageLayout baseline = PageLayout::Default();

  void Load(PageLayout::Overlay overlay) noexcept;

  [[nodiscard]] [[gnu::pure]]
  bool IsDirty() const noexcept;

  void SyncButtons(Button *apply_button, Button *add_button) const noexcept;

  /** Apply when dirty; reload draft/baseline from the live page. */
  [[nodiscard]]
  bool ApplyIfDirty() noexcept;

  /** Append a page from #draft; refresh button enablement. */
  void AddPage(Button *apply_button, Button *add_button) noexcept;
};

void
OpenPagesConfig() noexcept;

/**
 * Single-choice enum label used for draft Time / Level / Altitude rows.
 */
void
SetAxisLabel(WndProperty &control, const char *label,
             bool enabled) noexcept;

} // namespace WeatherOverlayDraft
