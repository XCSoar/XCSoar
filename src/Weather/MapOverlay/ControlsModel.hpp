// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FieldPickerResult.hpp"
#include "util/Compiler.h"
#include "util/StaticString.hxx"

#include <cstdint>
#include <functional>
#include <utility>

struct MoreData;

namespace WeatherMapOverlay {

/**
 * Secondary row label tap when level auto is not resumed first.
 */
enum class SecondaryLabelAction : uint8_t {
  NONE,
  OPEN_PICKER,
};

/**
 * Result of #ControlsModel::OpenSecondaryPicker.
 *
 * #OPEN_SETUP must be handled by the caller after the model method returns,
 * without touching the cursor-bar widget again — Weather Setup can replace
 * the bottom widget and destroy #ControlsWidget.
 */
enum class SecondaryPickerResult : uint8_t {
  NONE,
  OPEN_SETUP,
};

/**
 * Map a FieldControls picker result onto #SecondaryPickerResult and run
 * @p on_changed when the selection changed.
 */
template<typename F>
SecondaryPickerResult
HandleSecondaryFieldPicker(FieldPickerResult result,
                           F &&on_changed) noexcept
{
  switch (result) {
  case FieldPickerResult::OPEN_SETUP:
    return SecondaryPickerResult::OPEN_SETUP;
  case FieldPickerResult::CHANGED:
    on_changed();
    return SecondaryPickerResult::NONE;
  case FieldPickerResult::NONE:
    return SecondaryPickerResult::NONE;
  }

  gcc_unreachable();
}

enum class PrimaryLabelAction : uint8_t {
  NONE,
  RESUME_AUTO,
  OPEN_PICKER,
  OPEN_SETUP,
};

/**
 * How #ControlsWidget should refresh after a model-driven change.
 */
enum class ControlsUpdate : uint8_t {
  NONE,
  LABELS,
  OVERLAY,
};

/**
 * Overlay-specific behaviour for the shared weather cursor bar.
 *
 * Primary axis is forecast time; secondary is level (EDL), layer
 * (RASP / XCTherm), or another overlay-specific axis.
 */
class ControlsModel {
  std::function<void(ControlsUpdate)> notify;

public:
  virtual ~ControlsModel() = default;

  void SetNotify(std::function<void(ControlsUpdate)> callback) noexcept {
    notify = std::move(callback);
  }

  virtual void OnShow() noexcept {}
  virtual void OnHide() noexcept {}

  virtual void FormatPrimaryLabel(StaticString<64> &text) const noexcept = 0;
  virtual void FormatSecondaryLabel(StaticString<64> &text) const noexcept = 0;

  [[nodiscard]]
  virtual bool HasPrimaryData() const noexcept = 0;

  [[nodiscard]]
  virtual bool HasSecondaryData() const noexcept = 0;

  /**
   * Step the primary / secondary axis.
   *
   * @return true when the map overlay should refresh
   */
  [[nodiscard]]
  virtual bool StepPrimary(int delta) noexcept = 0;

  [[nodiscard]]
  virtual bool StepSecondary(int delta) noexcept = 0;

  [[nodiscard]]
  virtual bool GetPrimaryAutoAdvance() const noexcept = 0;
  virtual void SetPrimaryAutoAdvance(bool auto_advance) noexcept = 0;
  virtual void ApplyPrimaryAutoAdvance() noexcept = 0;

  [[nodiscard]]
  virtual bool SupportsSecondaryAutoAdvance() const noexcept {
    return false;
  }

  [[nodiscard]]
  virtual bool GetSecondaryAutoAdvance() const noexcept {
    return false;
  }

  virtual void SetSecondaryAutoAdvance(bool auto_advance) noexcept {
    (void)auto_advance;
  }

  virtual void ApplySecondaryAutoAdvance() noexcept {}

  [[nodiscard]]
  virtual PrimaryLabelAction GetPrimaryLabelAction() const noexcept {
    return PrimaryLabelAction::RESUME_AUTO;
  }

  virtual void OpenPrimaryPicker() noexcept {}

  [[nodiscard]]
  virtual SecondaryLabelAction GetSecondaryLabelAction() const noexcept {
    return SecondaryLabelAction::NONE;
  }

  virtual void ResumePrimaryAuto() noexcept {}
  virtual void ResumeSecondaryAuto() noexcept {}

  [[nodiscard]]
  virtual SecondaryPickerResult OpenSecondaryPicker() noexcept {
    return SecondaryPickerResult::NONE;
  }

  /** Map-side refresh only; labels are updated by #ControlsWidget. */
  virtual void RefreshOverlay() noexcept = 0;

  virtual void OnGPSUpdate(const MoreData &basic) noexcept {
    (void)basic;
  }

  /** Shared entry point for menu/input and picker auto-on actions. */
  virtual void EnablePrimaryAutoFromInput() noexcept {
    EnablePrimaryAutoAndRefresh();
  }

protected:
  void NotifyOverlay() noexcept {
    Notify(ControlsUpdate::OVERLAY);
  }

  void EnablePrimaryAutoAndRefresh() noexcept {
    SetPrimaryAutoAdvance(true);
    ApplyPrimaryAutoAdvance();
    NotifyOverlay();
  }

  void Notify(ControlsUpdate update) noexcept {
    if (notify)
      notify(update);
  }
};

} // namespace WeatherMapOverlay
