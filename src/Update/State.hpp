// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Types.hpp"

#include <boost/json/fwd.hpp>

#include <cstdint>
#include <array>
#include <optional>

struct UpdateStateData {
  std::optional<int64_t> last_successful_check;
  std::array<StaticString<64>,
             static_cast<unsigned>(UpdateBackendId::COUNT)> dismissed_offers{};

  [[gnu::pure]] bool IsAutomaticCheckDue(int64_t now) const noexcept;
  [[gnu::pure]] bool IsDismissed(UpdateBackendId backend,
                                const char *offer) const noexcept;
  void Dismiss(UpdateBackendId backend, const char *offer) noexcept;
};

boost::json::value MakeUpdateStateJSON(const UpdateStateData &state);

[[nodiscard]]
std::optional<UpdateStateData>
ParseUpdateStateJSON(const boost::json::value &value) noexcept;
