// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "co/InvokeTask.hxx"
#include "net/AsyncTask.hpp"
#include "net/client/WeGlide/Account.hpp"
#include "net/client/WeGlide/Tiles.hpp"
#include "system/Path.hpp"
#include "ui/event/Notify.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include <chrono>
#include <string>
#include <vector>

class CurlGlobal;

/**
 * One Copilot request at a time: refresh, tile token, catalog,
 * latest, or a single PNG. The UI thread decides which, in Poll().
 */
class WeGlideDownloadGlue final {
public:
  enum class Kind : uint8_t {
    REFRESH,
    TOKEN,
    CATALOG,
    LATEST,
    TILE,
  };

private:
  CurlGlobal &curl;
  Net::AsyncTask task;
  UI::Notify complete_notify{[this]{ OnCompleteNotify(); }};

  Kind kind = Kind::TOKEN;
  std::string url;
  std::string form;
  std::string authorization;
  AllocatedPath path{nullptr};

  WeGlide::AccountToken account;
  WeGlide::RefreshOutcome refresh_outcome =
    WeGlide::RefreshOutcome::UNCHANGED;
  unsigned status = 0;
  std::string body;
  std::exception_ptr error;
  bool wrote_file = false;

  Co::InvokeTask Run();
  void OnCompletion(std::exception_ptr error) noexcept;
  void OnCompleteNotify() noexcept;

  UI::PeriodicTimer timer{[this]{ Poll(); }};

public:
  explicit WeGlideDownloadGlue(CurlGlobal &_curl) noexcept;

  ~WeGlideDownloadGlue() noexcept { BeginShutdown(); }

  WeGlideDownloadGlue(const WeGlideDownloadGlue &) = delete;
  WeGlideDownloadGlue &operator=(const WeGlideDownloadGlue &) = delete;

  void BeginShutdown() noexcept;

  void Cancel() noexcept;

  void Poll() noexcept;

private:
  [[nodiscard]] bool IsRunning() const noexcept { return task.IsRunning(); }

  void StartRefresh(WeGlide::AccountToken _account) noexcept;
  void StartGet(Kind _kind, std::string _url, std::string _authorization,
                AllocatedPath _path = nullptr) noexcept;
  void Schedule(std::chrono::seconds delay) noexcept;
};

WeGlideDownloadGlue *GetWeGlideDownloadGlue() noexcept;

namespace WeGlide {

void ActivatePageOverlay(std::string_view layer_id, bool first_enter,
                         int64_t frame_time) noexcept;
void DeactivatePageOverlay() noexcept;
void SuspendForPan() noexcept;
void ResumeAfterPan() noexcept;
void OnProjectionModified() noexcept;
void SelectLayer(std::string_view layer_id) noexcept;

/**
 * Zero follows the live frame. A positive value is a Unix frame,
 * snapped onto the selected layer's grid.
 */
void SetFrameTime(int64_t frame_time) noexcept;

[[nodiscard]] std::string_view SelectedLayer() noexcept;
[[nodiscard]] const LayerTick *SelectedTick() noexcept;
[[nodiscard]] const std::vector<LayerInfo> &Catalog() noexcept;

} // namespace WeGlide
