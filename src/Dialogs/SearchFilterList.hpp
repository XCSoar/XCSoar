// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SearchNesting.hpp"
#include "Widget/TwoWidgets.hpp"

#include <memory>

/**
 * Filter row stacked above a result list (see search-dialog-ux.mdc).
 */
[[nodiscard]]
inline std::unique_ptr<TwoWidgets>
MakeFilterAndList(std::unique_ptr<Widget> filter,
                  std::unique_ptr<Widget> list) noexcept
{
  return std::make_unique<TwoWidgets>(std::move(filter),
                                      std::move(list), false);
}

/**
 * Filter + action buttons on the left, result list on the right.
 */
[[nodiscard]]
inline std::unique_ptr<TwoWidgets>
MakeFilterButtonsAndList(std::unique_ptr<Widget> filter,
                         std::unique_ptr<Widget> buttons,
                         std::unique_ptr<Widget> list) noexcept
{
  auto left = std::make_unique<TwoWidgets>(std::move(filter),
                                           std::move(buttons), true);
  return std::make_unique<TwoWidgets>(std::move(left),
                                      std::move(list), false);
}

/**
 * After a nested details dialog, close the parent browse search only
 * when the child reported a committed state change.
 */
[[gnu::const]]
constexpr bool
ParentSearchClosesAfterDetails(bool state_change_committed) noexcept
{
  return state_change_committed;
}
