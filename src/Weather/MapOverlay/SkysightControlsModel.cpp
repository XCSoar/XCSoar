// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightControlsModel.hpp"

#include "ActionInterface.hpp"
#include "DataGlobals.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Weather/Skysight/ForecastUtils.hpp"
#include "Weather/Skysight/Skysight.hpp"
#include "time/BrokenDateTime.hpp"

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

  const BrokenDateTime date_time{
    std::chrono::system_clock::from_time_t(forecast_time) +
    CommonInterface::GetComputerSettings().utc_offset.ToDuration()};

  if (SkySight::IsFullDayForecastLayer(layer))
    label.Format("%04u-%02u-%02u",
                 date_time.year, date_time.month, date_time.day);
  else
    label.Format("%04u-%02u-%02u %02u:%02u",
                 date_time.year, date_time.month, date_time.day,
                 date_time.hour, date_time.minute);

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

  if (layer->SupportsLiveTiles()) {
    if (layer->updating)
      text = _("Updating...");
    else if (layer->last_update != 0)
      text.Format(_("Updated %s"),
                  FormatForecastTimeLabel(*layer, layer->last_update).c_str());
    else
      text = _("Live tiles");
    return;
  }

  if (layer->pending_downloads > 0 || layer->datafiles_pending) {
    text = _("Updating...");
    return;
  }

  const auto steps = BuildForecastTimes(*layer).size();
  if (steps == 0) {
    text = _("No forecast steps");
    return;
  }

  text.Format(steps == 1
                ? _("1 step available")
                : _("%u steps available"),
              unsigned(steps));
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
  return GetLayer() != nullptr;
}

bool
SkysightControlsModel::StepPrimary(int delta) noexcept
{
  const auto *layer = GetLayer();
  if (layer == nullptr || layer->SupportsLiveTiles())
    return false;

  const auto times = BuildForecastTimes(*layer);
  if (times.empty())
    return false;

  int index = FindForecastIndex(*layer, times);
  if (index < 0)
    index = 0;

  index += delta;
  if (index < 0 || index >= int(times.size()))
    return false;

  return skysight->SelectForecastTime(layer->id, times[unsigned(index)]);
}

bool
SkysightControlsModel::StepSecondary([[maybe_unused]] int delta) noexcept
{
  return false;
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
  const auto *layer = GetLayer();
  if (layer == nullptr || layer->SupportsLiveTiles())
    return;

  if (auto_advance)
    (void)skysight->SelectAutomaticForecastTime(layer->id);
  else if (layer->forecast_time != 0)
    (void)skysight->SelectForecastTime(layer->id, layer->forecast_time);
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

void
SkysightControlsModel::OpenPrimaryPicker() noexcept
{
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
