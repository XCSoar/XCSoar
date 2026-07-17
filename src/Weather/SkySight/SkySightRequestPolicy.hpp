// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <algorithm>
#include <charconv>
#include <ctime>
#include <limits>
#include <map>
#include <string>
#include <string_view>

namespace SkySight {

inline constexpr time_t THROTTLE_FALLBACK_SECONDS = 30;

[[nodiscard]] inline time_t
ParseRetryAfterSeconds(std::string_view value, time_t now) noexcept
{
  unsigned long long seconds = 0;
  const auto result = std::from_chars(value.data(), value.data() + value.size(),
                                      seconds);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
    return 0;

  const auto maximum = std::numeric_limits<time_t>::max() - now;
  return seconds > static_cast<unsigned long long>(maximum)
    ? std::numeric_limits<time_t>::max()
    : now + static_cast<time_t>(seconds);
}

enum class RequestFailureAction {
  Terminal,
  Retry,
  Reauthenticate,
};

enum class AuthenticationFailureAction {
  Retry,
  Throttle,
  Rejected,
};

struct AuthenticationFailureDecision {
  AuthenticationFailureAction action;
  time_t ready_at;
  unsigned attempts;
};

/**
 * Prevents a queued authenticated request from turning a failed login into an
 * endless authentication loop.  Rejected credentials stay rejected until the
 * caller explicitly resets the policy.
 */
class AuthenticationFailurePolicy {
  static constexpr time_t BASE_RETRY_SECONDS = 30;
  static constexpr time_t MAX_RETRY_SECONDS = 10 * 60;
  time_t ready_at = 0;
  unsigned attempts = 0;
  bool rejected = false;

  static constexpr time_t
  GetBackoff(unsigned value) noexcept
  {
    time_t delay = BASE_RETRY_SECONDS;
    for (unsigned i = 1; i < value && delay < MAX_RETRY_SECONDS; ++i)
      delay = std::min(delay * 2, MAX_RETRY_SECONDS);

    return delay;
  }

public:
  [[nodiscard]] bool
  CanAttempt(time_t now) const noexcept
  {
    return !rejected && now >= ready_at;
  }

  AuthenticationFailureDecision
  OnHttpFailure(unsigned status, time_t now, time_t retry_after=0) noexcept
  {
    ++attempts;

    if (status == 429) {
      rejected = false;
      ready_at = retry_after > now
        ? retry_after
        : now + THROTTLE_FALLBACK_SECONDS;
      return {AuthenticationFailureAction::Throttle, ready_at, attempts};
    }

    if (status == 408 || status >= 500) {
      rejected = false;
      ready_at = now + GetBackoff(attempts);
      return {AuthenticationFailureAction::Retry, ready_at, attempts};
    }

    rejected = true;
    ready_at = 0;
    return {AuthenticationFailureAction::Rejected, 0, attempts};
  }

  AuthenticationFailureDecision
  OnTransportFailure(time_t now) noexcept
  {
    ++attempts;
    rejected = false;
    ready_at = now + GetBackoff(attempts);
    return {AuthenticationFailureAction::Retry, ready_at, attempts};
  }

  void Reset() noexcept
  {
    ready_at = 0;
    attempts = 0;
    rejected = false;
  }
};

/**
 * Uses a conservative fixed cooldown when the server omits Retry-After.
 */
class ThrottleFallbackPolicy {
public:
  [[nodiscard]] time_t
  OnThrottle(time_t now, time_t retry_at=0) noexcept
  {
    if (retry_at > now)
      return retry_at;

    return now + THROTTLE_FALLBACK_SECONDS;
  }
};

/**
 * Keeps live XYZ requests below the provider's observed rolling rate limit.
 * Forecast downloads use immutable API-provided links and are not paced here.
 */
class LiveTileRequestPacer {
  static constexpr time_t MIN_INTERVAL_SECONDS = 8;
  static constexpr time_t BURST_REARM_IDLE_SECONDS = 30;
  static constexpr unsigned BURST_SIZE = 4;

  time_t next_request_at = 0;
  time_t idle_since = 0;
  unsigned burst_remaining = BURST_SIZE;

public:
  [[nodiscard]] bool CanStart(time_t now) const noexcept {
    return burst_remaining > 0 || now >= next_request_at;
  }

  void OnStarted(time_t now) noexcept {
    if (burst_remaining > 0)
      --burst_remaining;

    idle_since = 0;
    next_request_at = now + MIN_INTERVAL_SECONDS;
  }

  void OnQueueState(time_t now, bool busy) noexcept {
    if (busy) {
      idle_since = 0;
      return;
    }

    if (idle_since == 0) {
      idle_since = now;
      return;
    }

    if (now >= idle_since + BURST_REARM_IDLE_SECONDS)
      burst_remaining = BURST_SIZE;
  }

  void OnThrottle() noexcept {
    burst_remaining = 0;
    idle_since = 0;
  }
};

/**
 * Prevents authentication, catalog, metadata and live-tile requests from
 * forming a startup burst.  Forecast file downloads are intentionally exempt.
 */
class InteractiveRequestPacer {
  static constexpr time_t MIN_INTERVAL_SECONDS = 1;

  time_t next_request_at = 0;

public:
  [[nodiscard]] bool CanStart(time_t now) const noexcept {
    return now >= next_request_at;
  }

  void OnStarted(time_t now) noexcept {
    next_request_at = now + MIN_INTERVAL_SECONDS;
  }
};

struct RequestFailureDecision {
  RequestFailureAction action;
  time_t ready_at;
  unsigned attempts;
};

/**
 * Remembers failures by immutable request key so render-driven scheduling
 * cannot turn a terminal response into an unbounded retry loop.
 */
class RequestFailurePolicy {
  struct State {
    time_t ready_at = 0;
    unsigned attempts = 0;
    bool terminal = false;
  };

  static constexpr time_t BASE_RETRY_SECONDS = 10;
  static constexpr time_t MAX_RETRY_SECONDS = 10 * 60;
  std::map<std::string, State, std::less<>> states;

  static constexpr time_t
  GetBackoff(unsigned attempts) noexcept
  {
    time_t delay = BASE_RETRY_SECONDS;
    for (unsigned i = 1; i < attempts && delay < MAX_RETRY_SECONDS; ++i)
      delay = std::min(delay * 2, MAX_RETRY_SECONDS);

    return delay;
  }

  RequestFailureDecision
  SetTerminal(std::string_view key) noexcept
  {
    auto &state = states[std::string{key}];
    ++state.attempts;
    state.ready_at = 0;
    state.terminal = true;
    return {RequestFailureAction::Terminal, 0, state.attempts};
  }

  RequestFailureDecision
  SetRetry(std::string_view key, time_t now, time_t ready_at=0) noexcept
  {
    auto &state = states[std::string{key}];
    ++state.attempts;
    state.terminal = false;
    state.ready_at = ready_at > now
      ? ready_at
      : now + GetBackoff(state.attempts);
    return {RequestFailureAction::Retry, state.ready_at, state.attempts};
  }

public:
  [[nodiscard]] bool
  CanQueue(std::string_view key, time_t now) const noexcept
  {
    const auto i = states.find(key);
    return i == states.end() ||
      (!i->second.terminal && now >= i->second.ready_at);
  }

  [[nodiscard]] unsigned
  GetAttempts(std::string_view key) const noexcept
  {
    const auto i = states.find(key);
    return i == states.end() ? 0 : i->second.attempts;
  }

  RequestFailureDecision
  OnHttpFailure(std::string_view key, unsigned status, time_t now,
                time_t retry_at=0) noexcept
  {
    if (status == 401) {
      auto &state = states[std::string{key}];
      ++state.attempts;
      if (state.attempts == 1) {
        state.ready_at = now;
        state.terminal = false;
        return {RequestFailureAction::Reauthenticate, now, state.attempts};
      }

      state.ready_at = 0;
      state.terminal = true;
      return {RequestFailureAction::Terminal, 0, state.attempts};
    }

    if (status == 429)
      return SetRetry(key, now, retry_at > now
                      ? retry_at
                      : now + THROTTLE_FALLBACK_SECONDS);

    if (status == 408 || status >= 500)
      return SetRetry(key, now);

    /* Immutable tile requests cannot recover from other 4xx responses. */
    return SetTerminal(key);
  }

  RequestFailureDecision
  OnTransportFailure(std::string_view key, time_t now) noexcept
  {
    return SetRetry(key, now);
  }

  void OnSuccess(std::string_view key) noexcept {
    if (const auto i = states.find(key); i != states.end())
      states.erase(i);
  }

  void Erase(std::string_view key) noexcept {
    if (const auto i = states.find(key); i != states.end())
      states.erase(i);
  }

  void Clear() noexcept {
    states.clear();
  }
};

} // namespace SkySight
