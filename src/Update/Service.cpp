// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Service.hpp"
#include "Backend.hpp"
#include "Profile/Keys.hpp"
#include "Profile/ProfileMap.hpp"
#include "time/SystemClock.hxx"

#include <chrono>
#include <algorithm>

static bool
IsAutomaticCheckEnabled() noexcept
{
  bool enabled = true;
  Profile::Get(ProfileKeys::UpdateCheckEnabled, enabled);
  return enabled;
}

UpdateService::UpdateService(UpdateStateData state,
                             SaveStateCallback save_state)
  :persistent_state(std::move(state)), save_state_callback(save_state)
{
}

UpdateService::~UpdateService() noexcept
{
  BeginShutdown();
}

void
UpdateService::Attach(UpdateBackend &_backend)
{
  if (FindBackend(_backend) == nullptr)
    backends.push_back(BackendState{&_backend});
}

void
UpdateService::Detach(UpdateBackend &_backend) noexcept
{
  const auto i = std::find_if(backends.begin(), backends.end(),
                              [&](const auto &entry) {
                                return entry.backend == &_backend;
                              });
  if (i != backends.end())
    backends.erase(i);
  RecomputeEffectiveState();
}

UpdateService::BackendState *
UpdateService::FindBackend(UpdateBackend &backend) noexcept
{
  const auto i = std::find_if(backends.begin(), backends.end(),
                              [&](const auto &entry) {
                                return entry.backend == &backend;
                              });
  return i != backends.end() ? &*i : nullptr;
}

void
UpdateService::AddBackend(std::unique_ptr<UpdateBackend> backend)
{
  if (!backend)
    return;

  auto &reference = *backend;
  owned_backends.push_back(std::move(backend));
  try {
    Attach(reference);
  } catch (...) {
    owned_backends.pop_back();
    throw;
  }

  reference.OnAttached();
}

void
UpdateService::RecomputeEffectiveState() noexcept
{
  BackendState *best = nullptr;
  bool checking = false;
  bool any_up_to_date = false;
  bool any_error = false;

  for (auto &entry : backends) {
    checking |= entry.state == UpdateState::CHECKING;
    any_up_to_date |= entry.state == UpdateState::UP_TO_DATE;
    any_error |= entry.state == UpdateState::FAILED;
    if (entry.state == UpdateState::AVAILABLE && entry.info &&
        (best == nullptr || entry.backend->GetPriority() >
                              best->backend->GetPriority()))
      best = &entry;
  }

  if (best != nullptr) {
    state = UpdateState::AVAILABLE;
    info = best->info;
  } else {
    info.reset();
    state = checking ? UpdateState::CHECKING
      : any_up_to_date ? UpdateState::UP_TO_DATE
      : any_error ? UpdateState::FAILED
      : UpdateState::IDLE;
  }
}

void
UpdateService::NotifyResultListener() noexcept
{
  if (result_listener)
    try {
      result_listener();
    } catch (...) {
    }
}

bool
UpdateService::StartChecks(UpdateCheckTrigger trigger, bool automatic) noexcept
{
  if (shutting_down)
    return false;

  bool started = false;
  for (auto &entry : backends) {
    if (!entry.backend->SupportsCheck(trigger) ||
        entry.state == UpdateState::CHECKING)
      continue;

    try {
      entry.backend->StartCheck(trigger);
      entry.state = UpdateState::CHECKING;
      entry.checking_automatically = automatic;
      started = true;
    } catch (...) {
      if (!entry.info)
        entry.state = UpdateState::FAILED;
    }
  }

  if (started)
    notify_automatically = false;
  RecomputeEffectiveState();
  if (!started)
    NotifyResultListener();
  return started;
}

bool
UpdateService::StartCheck(bool automatic) noexcept
{
  return StartChecks(UpdateCheckTrigger::MANUAL, automatic);
}

bool
UpdateService::StartAutomaticCheck(int64_t now) noexcept
{
  return IsAutomaticCheckEnabled() &&
    persistent_state.IsAutomaticCheckDue(now) &&
    StartChecks(UpdateCheckTrigger::SCHEDULED, true);
}

void
UpdateService::NotifyBackendCompletion() noexcept
{
  if (!shutting_down)
    completion_notify.SendNotification();
}

void
UpdateService::OnStartupFinished() noexcept
{
  startup_complete = true;
  if (completion_deferred)
    NotifyResultListener();
  completion_deferred = false;
}

void
UpdateService::OnBackendCompletion() noexcept
{
  if (shutting_down)
    return;

  const bool defer_listener = !startup_complete;

  bool changed = false;
  for (auto &entry : backends) {
    std::optional<UpdateCheckResult> result;
    try {
      result = entry.backend->ConsumeCheckResult();
    } catch (...) {
      entry.checking_automatically = false;
      entry.state = UpdateState::FAILED;
      entry.info.reset();
      changed = true;
      continue;
    }

    if (!result)
      continue;

    changed = true;
    const bool automatic = entry.checking_automatically;
    entry.checking_automatically = false;
    if (result->origin == UpdateResultOrigin::CACHE) {
      notify_automatically |= IsAutomaticCheckEnabled();
      if (result->state == UpdateState::AVAILABLE)
        entry.info = result->info;
      if (entry.state != UpdateState::CHECKING) {
        entry.state = result->state;
        if (result->state != UpdateState::AVAILABLE)
          entry.info.reset();
      }
    } else {
      if (result->state == UpdateState::AVAILABLE)
        notify_automatically |= automatic;
      entry.state = result->state;
      if (entry.state == UpdateState::AVAILABLE)
        entry.info = result->info;
      else if (entry.state == UpdateState::UP_TO_DATE)
        entry.info.reset();
      else if (entry.state == UpdateState::FAILED && entry.info)
        entry.state = UpdateState::AVAILABLE;
    }

    if (result->origin == UpdateResultOrigin::CHECK &&
        entry.backend->SupportsCheck(UpdateCheckTrigger::SCHEDULED) &&
        (result->state == UpdateState::UP_TO_DATE ||
         result->state == UpdateState::AVAILABLE)) {
      const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        DurationSinceUnixEpoch(std::chrono::system_clock::now())).count();
      persistent_state.last_successful_check = now;
      if (save_state_callback != nullptr)
        save_state_callback(persistent_state);
    }
  }

  if (!changed)
    return;

  RecomputeEffectiveState();

  if (defer_listener)
    completion_deferred = true;
  else
    NotifyResultListener();
}

void
UpdateService::BeginShutdown() noexcept
{
  shutting_down = true;
  completion_notify.ClearNotification();
  completion_deferred = false;
  for (auto &backend : owned_backends)
    backend->BeginShutdown();
  backends.clear();
  info.reset();
  result_listener = nullptr;
}

std::function<void()>
UpdateService::SetResultListener(std::function<void()> listener)
{
  return std::exchange(result_listener, std::move(listener));
}

void
UpdateService::Dismiss(const UpdateInfo &offer) noexcept
{
  persistent_state.Dismiss(offer.backend_id, offer.offer_id.c_str());
  if (save_state_callback != nullptr)
    save_state_callback(persistent_state);
}
