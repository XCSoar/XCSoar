// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/CharUtil.hxx"
#include "util/StaticString.hxx"

#include <array>
#include <optional>
#include <string_view>

enum class UpdateState {
  IDLE,
  CHECKING,
  UP_TO_DATE,
  AVAILABLE,
  FAILED,
  COUNT,
};

enum class UpdateScope {
  APPLICATION,
  SYSTEM,
  UNKNOWN,
  COUNT,
};

enum class RestartRequirement {
  NONE,
  APPLICATION,
  SYSTEM,
  UNKNOWN,
  COUNT,
};

enum class UpdateBackendId {
  REPOSITORY,
  COUNT,
};

static constexpr std::array UPDATE_BACKEND_NAMES{
  "repository-v1",
};

static_assert(UPDATE_BACKEND_NAMES.size() ==
              static_cast<unsigned>(UpdateBackendId::COUNT));

[[gnu::pure]]
static constexpr bool
IsValidUpdateOfferId(std::string_view offer) noexcept
{
  if (offer.empty() || offer.size() >= StaticString<64>::capacity())
    return false;

  for (const char ch : offer)
    if (!(IsAlphaNumericASCII(ch) || ch == '.' || ch == '-' || ch == '_'))
      return false;

  return true;
}

[[gnu::const]]
static constexpr const char *
UpdateBackendIdToString(UpdateBackendId id) noexcept
{
  const auto index = static_cast<unsigned>(id);
  return index < UPDATE_BACKEND_NAMES.size()
    ? UPDATE_BACKEND_NAMES[index]
    : "";
}

enum class UpdateResultOrigin {
  CHECK,
  CACHE,
  COUNT,
};

/** A bounded update offer supplied by a platform backend. */
struct UpdateInfo {
  UpdateBackendId backend_id = UpdateBackendId::COUNT;
  StaticString<64> offer_id;
  UpdateScope scope = UpdateScope::UNKNOWN;
  StaticString<32> version;
  StaticString<96> title;
  StaticString<384> summary;
  StaticString<128> source;
  StaticString<256> handoff_url;
  RestartRequirement restart = RestartRequirement::UNKNOWN;
};

struct UpdateCheckResult {
  UpdateState state = UpdateState::FAILED;
  UpdateResultOrigin origin = UpdateResultOrigin::CHECK;
  std::optional<UpdateInfo> info;
};
