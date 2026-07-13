// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightControlsModel.hpp"

#include "ActionInterface.hpp"
#include "DataGlobals.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Form/DataField/Enum.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Weather/Skysight/ForecastUtils.hpp"
#include "Weather/Skysight/Skysight.hpp"
#include <algorithm>
#include <chrono>
#include <limits>
#include <vector>

namespace WeatherMapOverlay {

static constexpr unsigned TIME_PICKER_AUTO =
  std::numeric_limits<unsigned>::max();

[[nodiscard]] static StaticString<32>
FormatForecastTimeLabel(const SkySight::Layer &layer,
                        time_t forecast_time) noexcept
{
  StaticString<32> label;
  if (forecast_time <= 0)
    return label;

  const auto date_time = FormatLocalDateTimeYYYYMMDDHHMM(
    TimeStamp(std::chrono::duration<double>(forecast_time)),
    CommonInterface::GetComputerSettings().utc_offset);

  if (SkySight::IsFullDayForecastLayer(layer))
    label = date_time.c_str();
  else
    label = date_time.c_str();

  if (SkySight::IsFullDayForecastLayer(layer))
    label.Truncate(10);

  return label;
}

[[nodiscard]] static std::vector<time_t>
BuildForecastTimes(const SkySight::Layer &layer)
{
  std::vector<time_t> times;
  times.reserve(layer.forecast_datafiles.size());

  for (const auto &datafile : layer.forecast_datafiles)
    if (datafile.time > 0)
      times.push_back(datafile.time);

  std::sort(times.begin(), times.end());
  SkySight::CollapseFullDayForecastTimes(layer.id, times);
  return times;
}

[[nodiscard]] static int
FindForecastIndex(const SkySight::Layer &layer,
                  const std::vector<time_t> &times) noexcept
{
  for (unsigned i = 0; i < times.size(); ++i)
    if (times[i] == layer.forecast_time ||
        (SkySight::IsFullDayForecastLayer(layer) &&
         SkySight::IsSameForecastDay(times[i], layer.forecast_time)))
      return i;

  return -1;
}

SkysightControlsModel::SkysightControlsModel(std::shared_ptr<Skysight> _skysight,
                                             std::string_view _layer_id) noexcept
  :skysight(std::move(_skysight)), layer_id(_layer_id) {}

void
SkysightControlsModel::OnShow() noexcept
{
  countdown_timer.Schedule(std::chrono::seconds{1});
}

void
SkysightControlsModel::OnHide() noexcept
{
  countdown_timer.Cancel();
  countdown_visible = false;
}

const SkySight::Layer *
SkysightControlsModel::GetLayer() const noexcept
{
  return skysight != nullptr
    ? skysight->GetSelectedLayer(layer_id.c_str())
    : nullptr;
}

void
SkysightControlsModel::FormatPrimaryLabel(StaticString<64> &text) const noexcept
{
  if (skysight != nullptr && skysight->IsThrottled()) {
    text.Format(_("Download limit: retry in %u s"),
                unsigned(skysight->GetThrottleRemainingSeconds()));
    return;
  }

  if (skysight != nullptr) {
    const auto retry = skysight->GetDatafilesRetryRemainingSeconds();
    if (retry > 0) {
      text.Format(_("Retry download in %u s"), unsigned(retry));
      return;
    }
  }

  const auto *layer = GetLayer();
  if (layer == nullptr) {
    text = _("SkySight");
    return;
  }

  if (layer->SupportsLiveTiles()) {
    text = _("Live");
    return;
  }

  if (layer->forecast_time == 0) {
    text = _("Auto");
    return;
  }

  const auto label = FormatForecastTimeLabel(*layer, layer->forecast_time);
  if (layer->UsesAutomaticForecastTime())
    text.Format(_("AUTO: %s"), label.c_str());
  else
    text = label;
}

void
SkysightControlsModel::FormatSecondaryLabel(StaticString<64> &text) const noexcept
{
  const auto *layer = GetLayer();
  if (layer == nullptr) {
    text = _("No page layer");
    return;
  }

  text = layer->name.c_str();
}

bool
SkysightControlsModel::HasPrimaryData() const noexcept
{
  const auto *layer = GetLayer();
  return layer != nullptr &&
    (layer->SupportsLiveTiles() ||
     layer->forecast_time != 0 ||
     !layer->forecast_datafiles.empty());
}

bool
SkysightControlsModel::HasSecondaryData() const noexcept
{
  return skysight != nullptr && skysight->NumSelectedLayers() > 0;
}

bool
SkysightControlsModel::StepPrimary(int delta) noexcept
{
  try {
    const auto *layer = GetLayer();
    if (layer == nullptr || layer->SupportsLiveTiles())
      return false;

    const auto times = BuildForecastTimes(*layer);
    if (times.empty())
      return false;

    int index = FindForecastIndex(*layer, times);
    if (index < 0) {
      const auto reference = layer->forecast_time > 0
        ? layer->forecast_time
        : SkySight::ChooseClosestForecastTime(times,
                                              [](time_t time) { return time; });
      const auto nearest = std::lower_bound(times.begin(), times.end(), reference);
      if (nearest == times.end())
        index = int(times.size()) - 1;
      else if (nearest == times.begin())
        index = 0;
      else {
        const auto before = nearest - 1;
        index = reference - *before <= *nearest - reference
          ? int(before - times.begin())
          : int(nearest - times.begin());
      }
    }

    index += delta;
    if (index < 0 || index >= int(times.size()))
      return false;

    return skysight->SelectForecastTime(layer->id, times[unsigned(index)]);
  } catch (...) {
    return false;
  }
}

bool
SkysightControlsModel::StepSecondary(int delta) noexcept
{
  if (skysight == nullptr || delta == 0 || skysight->NumSelectedLayers() == 0)
    return false;

  int current = 0;
  for (std::size_t i = 0; i < skysight->NumSelectedLayers(); ++i) {
    const auto *layer = skysight->GetSelectedLayer(i);
    if (layer != nullptr && layer->id == layer_id.c_str()) {
      current = int(i);
      break;
    }
  }

  const int count = int(skysight->NumSelectedLayers());
  const int next = ((current + delta) % count + count) % count;
  return SelectLayer(unsigned(next));
}

void
SkysightControlsModel::UpdateCountdownLabel() noexcept
{
  const bool waiting = skysight != nullptr &&
    (skysight->IsThrottled() ||
     skysight->GetDatafilesRetryRemainingSeconds() > 0);

  if (waiting || countdown_visible)
    Notify(ControlsUpdate::LABELS);

  countdown_visible = waiting;
}

bool
SkysightControlsModel::SelectLayer(unsigned index) noexcept
{
  try {
    if (skysight == nullptr || index >= skysight->NumSelectedLayers())
      return false;

    const auto *layer = skysight->GetSelectedLayer(index);
    if (layer == nullptr || !skysight->SelectPageLayer(layer->id))
      return false;

    layer_id = layer->id.c_str();
    return true;
  } catch (...) {
    return false;
  }
}

bool
SkysightControlsModel::GetPrimaryAutoAdvance() const noexcept
{
  const auto *layer = GetLayer();
  return layer != nullptr && layer->UsesAutomaticForecastTime();
}

void
SkysightControlsModel::SetPrimaryAutoAdvance(bool auto_advance) noexcept
{
  try {
    const auto *layer = GetLayer();
    if (layer == nullptr || layer->SupportsLiveTiles())
      return;

    if (auto_advance)
      (void)skysight->SelectAutomaticForecastTime(layer->id);
    else if (layer->forecast_time != 0)
      (void)skysight->SelectForecastTime(layer->id, layer->forecast_time);
  } catch (...) {
  }
}

void
SkysightControlsModel::ApplyPrimaryAutoAdvance() noexcept
{
}

PrimaryLabelAction
SkysightControlsModel::GetPrimaryLabelAction() const noexcept
{
  const auto *layer = GetLayer();
  return layer != nullptr &&
    !layer->SupportsLiveTiles() &&
    !layer->forecast_datafiles.empty()
    ? PrimaryLabelAction::OPEN_PICKER
    : PrimaryLabelAction::NONE;
}

SecondaryLabelAction
SkysightControlsModel::GetSecondaryLabelAction() const noexcept
{
  return skysight != nullptr && skysight->NumSelectedLayers() > 0
    ? SecondaryLabelAction::OPEN_PICKER
    : SecondaryLabelAction::NONE;
}

void
SkysightControlsModel::OpenPrimaryPicker() noexcept
{
  try {
    const auto *layer = GetLayer();
    if (layer == nullptr || layer->SupportsLiveTiles())
      return;

    const auto times = BuildForecastTimes(*layer);
    if (times.empty())
      return;

    DataFieldEnum picker;
    picker.ClearChoices();
    picker.addEnumText(_("Auto"), TIME_PICKER_AUTO);

    for (unsigned i = 0; i < times.size(); ++i)
      picker.addEnumText(FormatForecastTimeLabel(*layer, times[i]).c_str(), i);

    if (layer->UsesAutomaticForecastTime())
      picker.SetValue(TIME_PICKER_AUTO);
    else {
      const int current = FindForecastIndex(*layer, times);
      picker.SetValue(current >= 0 ? unsigned(current) : 0);
    }

    if (!ComboPicker(_("SkySight Time"), picker, nullptr))
      return;

    const unsigned selected = picker.GetValue();
    if (selected == TIME_PICKER_AUTO)
      (void)skysight->SelectAutomaticForecastTime(layer->id);
    else if (selected < times.size())
      (void)skysight->SelectForecastTime(layer->id, times[selected]);
  } catch (...) {
  }
}

void
SkysightControlsModel::OpenSecondaryPicker() noexcept
{
  if (skysight == nullptr || skysight->NumSelectedLayers() == 0)
    return;

  DataFieldEnum picker;
  unsigned current = 0;
  for (std::size_t i = 0; i < skysight->NumSelectedLayers(); ++i) {
    const auto *layer = skysight->GetSelectedLayer(i);
    if (layer == nullptr)
      continue;

    picker.addEnumText(layer->name.c_str(), int(i));
    if (layer->id == layer_id.c_str())
      current = i;
  }

  picker.SetValue(current);
  if (!ComboPicker(_("SkySight Layer"), picker, nullptr))
    return;

  const int selected = picker.GetValue();
  if (selected >= 0 && unsigned(selected) < skysight->NumSelectedLayers() &&
      SelectLayer(unsigned(selected)))
    Notify(ControlsUpdate::OVERLAY);
}

void
SkysightControlsModel::ResumePrimaryAuto() noexcept
{
  if (GetPrimaryAutoAdvance())
    return;

  SetPrimaryAutoAdvance(true);
  Notify(ControlsUpdate::OVERLAY);
}

void
SkysightControlsModel::RefreshOverlay() noexcept
{
  ActionInterface::SendUIState(true);
}

} // namespace WeatherMapOverlay
