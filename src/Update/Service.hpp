// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "State.hpp"
#include "Types.hpp"
#include "ui/event/Notify.hpp"

#include <functional>
#include <atomic>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

class UpdateBackend;
enum class UpdateCheckTrigger;

class UpdateService {
public:
  using SaveStateCallback = void (*)(const UpdateStateData &) noexcept;

private:
  struct BackendState {
    UpdateBackend *backend;
    UpdateState state = UpdateState::IDLE;
    std::optional<UpdateInfo> info;
    bool checking_automatically = false;
  };

  std::vector<std::unique_ptr<UpdateBackend>> owned_backends;
  std::vector<BackendState> backends;
  UpdateStateData persistent_state;
  SaveStateCallback save_state_callback = nullptr;
  UpdateState state = UpdateState::IDLE;
  std::optional<UpdateInfo> info;
  UI::Notify completion_notify{[this]{ OnBackendCompletion(); }};
  std::function<void()> result_listener;
  std::atomic<bool> shutting_down{false};
  bool startup_complete = false;
  bool completion_deferred = false;
  bool notify_automatically = false;

  BackendState *FindBackend(UpdateBackend &backend) noexcept;
  void RecomputeEffectiveState() noexcept;
  void NotifyResultListener() noexcept;
  bool StartChecks(UpdateCheckTrigger trigger, bool automatic) noexcept;

public:
  explicit UpdateService(UpdateStateData state = {},
                         SaveStateCallback save_state = nullptr);
  ~UpdateService() noexcept;

  void Attach(UpdateBackend &backend);
  void Detach(UpdateBackend &backend) noexcept;
  void AddBackend(std::unique_ptr<UpdateBackend> backend);

  bool StartCheck(bool automatic = false) noexcept;
  bool StartAutomaticCheck(int64_t now) noexcept;
  void OnStartupFinished() noexcept;
  void NotifyBackendCompletion() noexcept;
  void OnBackendCompletion() noexcept;
  void BeginShutdown() noexcept;
  std::function<void()> SetResultListener(std::function<void()> listener);

  [[gnu::pure]] bool IsDismissed(const UpdateInfo &offer) const noexcept {
    return persistent_state.IsDismissed(offer.backend_id,
                                        offer.offer_id.c_str());
  }
  void Dismiss(const UpdateInfo &offer) noexcept;

  [[gnu::pure]] UpdateState GetState() const noexcept { return state; }
  [[gnu::pure]] bool HasBackend() const noexcept { return !backends.empty(); }
  [[gnu::pure]] const UpdateInfo *GetInfo() const noexcept {
    return info ? &*info : nullptr;
  }
  [[gnu::pure]] bool ShouldNotifyAutomatically() const noexcept {
    return notify_automatically;
  }
  void MarkAutomaticNotificationPresented() noexcept {
    notify_automatically = false;
  }
};
