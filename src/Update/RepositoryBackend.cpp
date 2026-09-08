// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RepositoryBackend.hpp"
#include "RepositoryOffer.hpp"
#include "Service.hpp"
#include "Version.hpp"

#include <stdexcept>
#include <utility>

#ifndef UPDATE_REPOSITORY_TARGET
#define UPDATE_REPOSITORY_TARGET "unknown"
#endif

#ifndef UPDATE_ALLOWED_HOSTS
#define UPDATE_ALLOWED_HOSTS "xcsoar.org,download.xcsoar.org,apps.apple.com"
#endif

RepositoryUpdateBackend::RepositoryUpdateBackend(
  Repository::Service &_repository, UpdateService &_service)
  :repository(_repository), service(_service)
{
  repository.AddListener(*this);
  registered = true;
}

RepositoryUpdateBackend::~RepositoryUpdateBackend() noexcept
{
  BeginShutdown();
}

void
RepositoryUpdateBackend::PublishSnapshot(UpdateResultOrigin origin) noexcept
{
  result = UpdateRepository::FindOffer(repository.GetRepository(),
                                       UPDATE_REPOSITORY_TARGET,
                                       UPDATE_ALLOWED_HOSTS,
                                       XCSoar_Version);
  if (!result) {
    result.emplace();
    result->state = UpdateState::FAILED;
  }
  result->origin = origin;
  service.NotifyBackendCompletion();
}

void
RepositoryUpdateBackend::OnAttached() noexcept
{
  if (repository.HasFreshSnapshot())
    PublishSnapshot(UpdateResultOrigin::CACHE);
}

void
RepositoryUpdateBackend::StartCheck(UpdateCheckTrigger trigger)
{
  const auto refresh = repository.Refresh(trigger == UpdateCheckTrigger::MANUAL);
  switch (refresh) {
  case Repository::RefreshResult::STARTED:
  case Repository::RefreshResult::IN_PROGRESS:
    return;

  case Repository::RefreshResult::FRESH:
    PublishSnapshot(UpdateResultOrigin::CACHE);
    return;

  case Repository::RefreshResult::UNAVAILABLE:
    throw std::runtime_error("Repository refresh is unavailable");
  }

  throw std::runtime_error("Invalid repository refresh result");
}

void
RepositoryUpdateBackend::OnRepositoryRefreshed(bool success) noexcept
{
  if (success)
    PublishSnapshot(UpdateResultOrigin::CHECK);
  else {
    result.emplace();
    result->state = UpdateState::FAILED;
    result->origin = UpdateResultOrigin::CHECK;
    service.NotifyBackendCompletion();
  }
}

std::optional<UpdateCheckResult>
RepositoryUpdateBackend::ConsumeCheckResult()
{
  return std::exchange(result, std::nullopt);
}

void
RepositoryUpdateBackend::BeginShutdown() noexcept
{
  if (registered) {
    repository.RemoveListener(*this);
    registered = false;
  }
  result.reset();
}
