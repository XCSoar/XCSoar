// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/SkySight/SkySightLimits.hpp"
#include "Weather/SkySight/SkySightRequestPolicy.hpp"
#include "TestUtil.hpp"

static void
TestRequestFailurePolicy()
{
  constexpr time_t NOW = 1000;
  SkySight::RequestFailurePolicy policy;

  ok1(SkySight::ParseRetryAfterSeconds("123", NOW) == NOW + 123);
  ok1(policy.CanQueue("missing-tile", NOW));
  auto decision = policy.OnHttpFailure("missing-tile", 404, NOW);
  ok1(decision.action == SkySight::RequestFailureAction::Terminal);
  ok1(!policy.CanQueue("missing-tile", NOW + 24 * 60 * 60));

  decision = policy.OnHttpFailure("bad-tile", 400, NOW);
  ok1(decision.action == SkySight::RequestFailureAction::Terminal);
  policy.Erase("bad-tile");
  ok1(policy.CanQueue("bad-tile", NOW));
  decision = policy.OnHttpFailure("forbidden-tile", 403, NOW);
  ok1(decision.action == SkySight::RequestFailureAction::Terminal);

  decision = policy.OnHttpFailure("unauthorized", 401, NOW);
  ok1(decision.action == SkySight::RequestFailureAction::Reauthenticate);
  ok1(policy.CanQueue("unauthorized", NOW));
  decision = policy.OnHttpFailure("unauthorized", 401, NOW);
  ok1(decision.action == SkySight::RequestFailureAction::Terminal);
  ok1(!policy.CanQueue("unauthorized", NOW + 24 * 60 * 60));

  decision = policy.OnHttpFailure("throttled", 429, NOW, NOW + 123);
  ok1(decision.action == SkySight::RequestFailureAction::Retry);
  ok1(decision.ready_at == NOW + 123);
  ok1(!policy.CanQueue("throttled", NOW + 122));
  ok1(policy.CanQueue("throttled", NOW + 123));
  decision = policy.OnHttpFailure("throttled-fallback", 429, NOW);
  ok1(decision.ready_at == NOW + 30);

  decision = policy.OnHttpFailure("server-error", 503, NOW);
  ok1(decision.ready_at == NOW + 10);
  decision = policy.OnHttpFailure("server-error", 503, NOW + 10);
  ok1(decision.ready_at == NOW + 30);
  for (unsigned i = 0; i < 10; ++i)
    decision = policy.OnTransportFailure("server-error", decision.ready_at);
  ok1(decision.ready_at <= NOW + 30 + 10 * 60 * 10);

  policy.Clear();
  ok1(policy.CanQueue("missing-tile", NOW));
}

static void
TestAuthenticationFailurePolicy()
{
  constexpr time_t NOW = 1000;
  SkySight::AuthenticationFailurePolicy authentication;

  ok1(authentication.CanAttempt(NOW));
  auto decision = authentication.OnHttpFailure(401, NOW);
  ok1(decision.action == SkySight::AuthenticationFailureAction::Rejected);
  ok1(!authentication.CanAttempt(NOW + 24 * 60 * 60));
  authentication.Reset();
  ok1(authentication.CanAttempt(NOW));

  decision = authentication.OnHttpFailure(429, NOW, NOW + 123);
  ok1(decision.action == SkySight::AuthenticationFailureAction::Throttle);
  ok1(decision.ready_at == NOW + 123);
  ok1(!authentication.CanAttempt(NOW + 122));
  ok1(authentication.CanAttempt(NOW + 123));

  authentication.Reset();
  decision = authentication.OnHttpFailure(429, NOW);
  ok1(decision.ready_at == NOW + 30);
  ok1(!authentication.CanAttempt(NOW + 29));

  authentication.Reset();
  decision = authentication.OnHttpFailure(503, NOW);
  ok1(decision.action == SkySight::AuthenticationFailureAction::Retry);
  ok1(decision.ready_at == NOW + 30);
  decision = authentication.OnTransportFailure(NOW + 30);
  ok1(decision.ready_at == NOW + 90);
}

static void
TestRequestPacing()
{
  constexpr time_t NOW = 1000;
  SkySight::ThrottleFallbackPolicy throttle;
  ok1(throttle.OnThrottle(NOW) == NOW + 30);
  ok1(throttle.OnThrottle(NOW + 30) == NOW + 60);
  ok1(throttle.OnThrottle(NOW + 60) == NOW + 90);
  ok1(throttle.OnThrottle(NOW + 90) == NOW + 120);
  ok1(throttle.OnThrottle(NOW + 120, NOW + 323) == NOW + 323);
  ok1(throttle.OnThrottle(NOW + 323) == NOW + 353);

  SkySight::LiveTileRequestPacer pacer;
  ok1(pacer.CanStart(NOW));
  pacer.OnStarted(NOW);
  ok1(pacer.CanStart(NOW + 1));
  pacer.OnStarted(NOW + 1);
  ok1(pacer.CanStart(NOW + 2));
  pacer.OnStarted(NOW + 2);
  ok1(pacer.CanStart(NOW + 3));
  pacer.OnStarted(NOW + 3);
  ok1(!pacer.CanStart(NOW + 10));
  ok1(pacer.CanStart(NOW + 11));

  SkySight::LiveTileRequestPacer rearmed_pacer;
  for (unsigned i = 0; i < 4; ++i)
    rearmed_pacer.OnStarted(NOW + i);
  rearmed_pacer.OnQueueState(NOW + 4, false);
  rearmed_pacer.OnQueueState(NOW + 34, false);
  rearmed_pacer.OnStarted(NOW + 34);
  ok1(rearmed_pacer.CanStart(NOW + 34));

  SkySight::LiveTileRequestPacer throttled_pacer;
  throttled_pacer.OnStarted(NOW);
  throttled_pacer.OnThrottle();
  ok1(!throttled_pacer.CanStart(NOW + 1));
  ok1(throttled_pacer.CanStart(NOW + 8));

  SkySight::InteractiveRequestPacer interactive_pacer;
  ok1(interactive_pacer.CanStart(NOW));
  interactive_pacer.OnStarted(NOW);
  ok1(!interactive_pacer.CanStart(NOW));
  ok1(interactive_pacer.CanStart(NOW + 1));
}

static void
TestResourcePolicies()
{
  ok1(SkySight::IsSafeId("EAST_US"));
  ok1(SkySight::IsSafeId("wstar_bsratio"));
  ok1(!SkySight::IsSafeId(""));
  ok1(!SkySight::IsSafeId(".."));
  ok1(!SkySight::IsSafeId("EUROPE/../x"));
  ok1(!SkySight::IsSafeId("rain.jpg"));
  ok1(SkySight::IsNetCdfGridSizeAllowed(1024, 1024));
  ok1(!SkySight::IsNetCdfGridSizeAllowed(1, 1024));
  ok1(!SkySight::IsNetCdfGridSizeAllowed(
    SkySight::MAX_NETCDF_GRID_AXIS + 1, 2));
  ok1(!SkySight::IsNetCdfGridSizeAllowed(4096, 4096));
}

int
main()
{
  plan_tests(61);
  TestRequestFailurePolicy();
  TestAuthenticationFailurePolicy();
  TestRequestPacing();
  TestResourcePolicies();
  return exit_status();
}
