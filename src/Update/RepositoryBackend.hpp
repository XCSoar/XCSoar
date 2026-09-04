// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Backend.hpp"
#include "Repository/Service.hpp"

#include <optional>

class UpdateService;

class RepositoryUpdateBackend final
  : public UpdateBackend, private Repository::Listener {
  Repository::Service &repository;
  UpdateService &service;
  std::optional<UpdateCheckResult> result;
  bool registered = false;

  void PublishSnapshot(UpdateResultOrigin origin) noexcept;
  void OnRepositoryRefreshed(bool success) noexcept override;

public:
  RepositoryUpdateBackend(Repository::Service &repository,
                          UpdateService &service);
  ~RepositoryUpdateBackend() noexcept override;

  UpdateBackendId GetId() const noexcept override {
    return UpdateBackendId::REPOSITORY;
  }
  bool SupportsCheck(UpdateCheckTrigger) const noexcept override {
    return true;
  }
  void OnAttached() noexcept override;

  void StartCheck(UpdateCheckTrigger trigger) override;
  std::optional<UpdateCheckResult> ConsumeCheckResult() override;
  void BeginShutdown() noexcept override;
};
