// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/MapOverlay/FieldPickerResult.hpp"
#include "util/StaticString.hxx"

struct PageLayout;

namespace XCTherm {

using LayerPickerResult = WeatherMapOverlay::FieldPickerResult;

/**
 * Apply both XCTherm cursor axes from a page to the live session.
 */
void
ApplyCursorFromPageLayout(const PageLayout &page) noexcept;

/**
 * Persist the live XCTherm cursor to a configured page.
 */
void
PersistCursorToPage(unsigned page_index) noexcept;

void
PersistCursorToCurrentPage() noexcept;

/**
 * Edit @p page.xctherm_time via the shared time picker. Does not
 * persist or update the live overlay (dialogs use this on a draft).
 *
 * @return false when the user cancels or no layers are configured
 */
[[nodiscard]]
bool
EditTimeOnLayout(PageLayout &page) noexcept;

/**
 * Cursor-bar time picker: #EditTimeOnLayout on the current map page,
 * then persist and refresh live.
 */
void
OpenTimePicker() noexcept;

/**
 * Edit @p page.xctherm_layer via the altitude picker. Does not
 * persist or update the live overlay (dialogs use this on a draft).
 */
[[nodiscard]]
LayerPickerResult
EditLayerOnLayout(PageLayout &page, bool offer_setup=false) noexcept;

/**
 * Cursor-bar altitude picker: #EditLayerOnLayout on the current map
 * page, then persist and refresh live.
 */
[[nodiscard]]
LayerPickerResult
OpenLayerPicker(bool offer_setup=false) noexcept;

void
FormatTimeLabelForPage(StaticString<64> &text,
                       const PageLayout &page) noexcept;

void
FormatLayerLabelForPage(StaticString<64> &text,
                        const PageLayout &page) noexcept;

} // namespace XCTherm
