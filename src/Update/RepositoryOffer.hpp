// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Types.hpp"

#include <optional>
#include <string_view>

struct FileRepository;

namespace UpdateRepository {

/** Find the newest valid stable offer for this build target. */
[[nodiscard]]
std::optional<UpdateCheckResult>
FindOffer(const FileRepository &repository, std::string_view target,
          std::string_view allowed_hosts,
          std::string_view current_version) noexcept;

} // namespace UpdateRepository
