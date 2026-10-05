// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "State.hpp"

#include <boost/json.hpp>

#include <string_view>

static constexpr int64_t CHECK_INTERVAL = 24 * 60 * 60;

bool
UpdateStateData::IsAutomaticCheckDue(int64_t now) const noexcept
{
  if (!last_successful_check)
    return true;

  const int64_t stored = *last_successful_check;
  if (stored > now)
    return true;

  return now - stored >= CHECK_INTERVAL;
}

bool
UpdateStateData::IsDismissed(UpdateBackendId backend,
                             const char *offer) const noexcept
{
  if (*UpdateBackendIdToString(backend) == '\0' || offer == nullptr ||
      !IsValidUpdateOfferId(offer))
    return false;

  return dismissed_offers[static_cast<unsigned>(backend)] == offer;
}

void
UpdateStateData::Dismiss(UpdateBackendId backend, const char *offer) noexcept
{
  if (*UpdateBackendIdToString(backend) == '\0' || offer == nullptr ||
      !IsValidUpdateOfferId(offer))
    return;

  dismissed_offers[static_cast<unsigned>(backend)] = offer;
}

boost::json::value
MakeUpdateStateJSON(const UpdateStateData &state)
{
  boost::json::object object;
  object["schema"] = 2;
  if (state.last_successful_check)
    object["last_successful_check"] = *state.last_successful_check;
  boost::json::object dismissed;
  for (unsigned i = 0; i < state.dismissed_offers.size(); ++i)
    if (!state.dismissed_offers[i].empty())
      dismissed.emplace(UPDATE_BACKEND_NAMES[i],
                        std::string(state.dismissed_offers[i].c_str()));
  if (!dismissed.empty())
    object["dismissed_offers"] = std::move(dismissed);
  return object;
}

static bool
IsValidDismissedOffer(std::string_view value) noexcept
{
  if (value.empty() || value.size() >= StaticString<96>::capacity())
    return false;

  const auto separator = value.find('/');
  if (separator == value.npos || separator == 0 ||
      separator + 1 >= value.size() ||
      value.find('/', separator + 1) != value.npos)
    return false;

  bool known_backend = false;
  const auto backend = value.substr(0, separator);
  for (const char *const name : UPDATE_BACKEND_NAMES)
    if (backend == std::string_view{name}) {
      known_backend = true;
      break;
    }

  if (!known_backend)
    return false;

  return IsValidUpdateOfferId(value.substr(separator + 1));
}

std::optional<UpdateStateData>
ParseUpdateStateJSON(const boost::json::value &value) noexcept
{
  if (!value.is_object())
    return std::nullopt;

  const auto &object = value.as_object();
  const auto *const schema = object.if_contains("schema");
  if (schema == nullptr || !schema->is_int64() ||
      (schema->as_int64() != 1 && schema->as_int64() != 2))
    return std::nullopt;

  UpdateStateData result;
  if (const auto *timestamp = object.if_contains("last_successful_check")) {
    if (!timestamp->is_int64() || timestamp->as_int64() < 0)
      return std::nullopt;
    result.last_successful_check = timestamp->as_int64();
  }

  // Accept schema 1 state files written by pre-release builds of this feature.
  if (schema->as_int64() == 1) {
    const auto *dismissed = object.if_contains("dismissed_offer");
    if (dismissed == nullptr)
      return result;
    if (!dismissed->is_string())
      return std::nullopt;

    const auto &string = dismissed->as_string();
    const std::string_view dismissed_offer{string.data(), string.size()};
    if (!IsValidDismissedOffer(dismissed_offer))
      return std::nullopt;

    const auto separator = dismissed_offer.find('/');
    const auto backend = dismissed_offer.substr(0, separator);
    const auto offer = dismissed_offer.substr(separator + 1);
    for (unsigned i = 0; i < UPDATE_BACKEND_NAMES.size(); ++i)
      if (backend == UPDATE_BACKEND_NAMES[i]) {
        result.dismissed_offers[i] = offer;
        break;
      }
  } else if (const auto *dismissed = object.if_contains("dismissed_offers")) {
    if (!dismissed->is_object())
      return std::nullopt;
    for (const auto &item : dismissed->as_object()) {
      if (!item.value().is_string())
        return std::nullopt;
      const auto &string = item.value().as_string();
      const std::string_view offer{string.data(), string.size()};
      if (!IsValidUpdateOfferId(offer))
        return std::nullopt;
      bool known = false;
      const std::string_view backend{item.key().data(), item.key().size()};
      for (unsigned i = 0; i < UPDATE_BACKEND_NAMES.size(); ++i)
        if (backend == std::string_view{UPDATE_BACKEND_NAMES[i]}) {
          result.dismissed_offers[i] = offer;
          known = true;
          break;
        }
      if (!known)
        return std::nullopt;
    }
  }

  return result;
}
