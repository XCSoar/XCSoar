// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSettings.hpp"

#include <functional>

class Button;
class RowFormWidget;
class WndProperty;

/**
 * Draft Layer/Time (Level/Altitude) for the current map page on Info →
 * Weather panels. Apply commits when dirty; Add page copies the draft.
 */
namespace WeatherOverlayDraft {

void
OpenPagesConfig() noexcept;

struct State {
  PageLayout draft = PageLayout::Default();
  PageLayout baseline = PageLayout::Default();

  void Load(PageLayout::Overlay overlay,
            const char *skysight_layer_id=nullptr) noexcept;

  [[nodiscard]] [[gnu::pure]]
  bool IsDirty() const noexcept;

  void SyncButtons(Button *apply_button, Button *add_button) const noexcept;

  /** Apply when dirty; reload draft/baseline from the live page. */
  [[nodiscard]]
  bool ApplyIfDirty() noexcept;

  /** Append a page from #draft; refresh button enablement. */
  void AddPage(Button *apply_button, Button *add_button) noexcept;

  void ApplyClicked(Button *apply_button, Button *add_button) noexcept {
    if (!ApplyIfDirty())
      return;
    SyncButtons(apply_button, add_button);
  }

  void Reload(PageLayout::Overlay overlay,
              Button *apply_button, Button *add_button,
              const char *skysight_layer_id=nullptr) noexcept {
    Load(overlay, skysight_layer_id);
    SyncButtons(apply_button, add_button);
  }

  void OpenPagesAndReload(PageLayout::Overlay overlay,
                          Button *apply_button, Button *add_button,
                          const char *skysight_layer_id=nullptr) noexcept {
    OpenPagesConfig();
    Reload(overlay, apply_button, add_button, skysight_layer_id);
  }
};

/**
 * Shared Apply / Add / Pages setup host for overlay providers.
 */
void
AddApplyAddButtons(RowFormWidget &form,
                   Button *&apply_button, Button *&add_button,
                   std::function<void()> apply_clicked,
                   std::function<void()> add_clicked) noexcept;

void
AddPagesSetupButton(RowFormWidget &form,
                    std::function<void()> after_open) noexcept;

[[nodiscard]]
bool
ApplySkySightLayerToPage(const char *layer_id) noexcept;

/**
 * Single-choice enum label used for draft Time / Level / Altitude rows.
 */
void
SetAxisLabel(WndProperty &control, const char *label,
             bool enabled) noexcept;

} // namespace WeatherOverlayDraft
