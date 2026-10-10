// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RangeStatisticsDialog.hpp"
#include "RangePlot.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Error.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "FLARM/Range.hpp"
#include "FLARM/RangeEstimate.hpp"
#include "Computer/FlarmRangeComputer.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Look/DialogLook.hpp"
#include "Look/FlarmTrafficLook.hpp"
#include "Look/Look.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "Operation/Operation.hpp"
#include "Language/Language.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/NumberParser.hpp"
#include "util/StaticString.hxx"
#include "UIGlobals.hpp"

#include <optional>
#include <vector>

/**
 * A sector needs at least this many data points (RFCNT) before its
 * mean range counts as measured: "A reasonable number is 50 or
 * larger" (FTD-065, the CARP application note, 2.3.1).
 */
static constexpr unsigned MIN_POINTS = 50;

static std::vector<FlarmRangeSector>
ToSectors(const FlarmRange::Channel &channel) noexcept
{
  std::vector<FlarmRangeSector> sectors(channel.mean.size());
  for (unsigned i = 0; i < sectors.size(); ++i) {
    sectors[i].range = channel.mean[i];
    sectors[i].significant = i < channel.count.size() &&
      channel.count[i] && *channel.count[i] >= MIN_POINTS;
  }

  return sectors;
}

class FlarmRangeWidget final : public RowFormWidget, DataFieldListener {
  enum Controls {
    ANTENNA,
    POINTS,
    PERIOD,
    BELOW_MINIMUM,
    RANGE_SETTING,
  };

  FlarmDevice &device;
  const FlarmTrafficLook &look;

  FlarmRange range;

  /** the device's RANGE setting [m], if it answered */
  std::optional<unsigned> range_setting;

  FlarmRangePlot *plot;

public:
  FlarmRangeWidget(const DialogLook &_dialog_look,
                   const FlarmTrafficLook &_look,
                   FlarmDevice &_device) noexcept
    :RowFormWidget(_dialog_look), device(_device), look(_look) {}

  void Reset() noexcept;

private:
  void Load() noexcept;
  void Update() noexcept;

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  /* virtual methods from DataFieldListener */
  void OnModified([[maybe_unused]] DataField &df) noexcept override {
    Update();
  }
};

void
FlarmRangeWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                          [[maybe_unused]] const PixelRect &rc) noexcept
{
  static constexpr StaticEnumChoice antenna_list[] = {
    { 0, "A" },
    { 1, "B" },
    nullptr
  };

  AddEnum(_("Antenna"),
          _("The FLARM keeps the statistics for each of its two radio "
            "channels."),
          antenna_list, 0, this);
  AddReadOnly(_("Data points"));
  AddReadOnly(_("Period"));
  AddReadOnly(_("Below minimum range"),
              _("Measured sectors of this channel whose mean range is "
                "shorter than FLARM's minimum: 2 nm ahead, 1 nm to the "
                "sides and behind.  Check the antenna installation."));
  AddReadOnly(_("Range setting"),
              _("The FLARM reports no aircraft beyond this distance.  "
                "The plot shows it as a dashed circle when it is within "
                "the plotted range."));

  auto window = std::make_unique<FlarmRangePlot>(GetLook(), look);
  window->Create((ContainerWindow &)GetWindow(), {0, 0, 100, 100},
                 WindowStyle{});
  plot = window.get();
  AddRemaining(std::move(window));

  Load();
}

void
FlarmRangeWidget::Load() noexcept
{
  range = {};
  range_setting.reset();

  try {
    PopupOperationEnvironment env;
    if (!device.ReadRangeStatistics(range, env))
      ShowMessageBox(_("The FLARM did not send its range statistics."),
                     _("FLARM range"), MB_OK | MB_ICONERROR);
  } catch (OperationCancelled) {
  } catch (...) {
    ShowError(std::current_exception(), _("FLARM range"));
  }

  /* after ReadRangeStatistics(), which has stopped the port's
     receive thread that would otherwise take the answer; the setting
     only adds a circle, so failing to read it is not an error */
  try {
    NullOperationEnvironment env;
    if (unsigned value; device.GetRange(value, env))
      range_setting = value;
  } catch (...) {
  }

  Update();
}

void
FlarmRangeWidget::Update() noexcept
{
  StaticString<64> buffer;

  if (range.points) {
    buffer.Format("%u", *range.points);
    SetText(POINTS, buffer);
  } else
    ClearText(POINTS);

  if (range.first && range.last) {
    /* the dates only; the first and last packet are UTC */
    char first[16], last[16];
    FormatISO8601(first, BrokenDate{BrokenDateTime{*range.first}});
    FormatISO8601(last, BrokenDate{BrokenDateTime{*range.last}});
    buffer.Format("%s – %s", first, last);
    SetText(PERIOD, buffer);
  } else
    ClearText(PERIOD);

  auto sectors = ToSectors(range.channels[GetValueEnum(ANTENNA)]);
  unsigned measured = 0, below = 0;
  for (unsigned i = 0; i < sectors.size(); ++i) {
    if (sectors[i].significant)
      ++measured;
    if (IsBelowMinimum(sectors[i], i, sectors.size()))
      ++below;
  }

  if (measured > 0) {
    buffer.Format("%u / %u", below, measured);
    SetText(BELOW_MINIMUM, buffer);
  } else
    ClearText(BELOW_MINIMUM);

  /* 65535 means "unlimited" (FTD-014, RANGE) */
  if (!range_setting)
    ClearText(RANGE_SETTING);
  else if (*range_setting >= 65535)
    SetText(RANGE_SETTING, _("Unlimited"));
  else
    SetText(RANGE_SETTING, FormatUserDistanceSmart(*range_setting));

  plot->SetLimit(range_setting);
  plot->SetSectors(std::move(sectors));
}

void
FlarmRangeWidget::Reset() noexcept
{
  if (ShowMessageBox(_("Reset the range statistics? Do this after a "
                       "change to the antennas; the FLARM then collects "
                       "them anew."),
                     _("FLARM range"), MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  try {
    PopupOperationEnvironment env;
    if (!device.ResetRangeStatistics(env))
      ShowMessageBox(_("The FLARM did not confirm the reset."),
                     _("FLARM range"), MB_OK | MB_ICONERROR);
  } catch (OperationCancelled) {
  } catch (...) {
    ShowError(std::current_exception(), _("FLARM range"));
  }

  Load();
}

void
FlarmRangeStatisticsDialog(FlarmDevice &device)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<FlarmRangeWidget> dialog(WidgetDialog::Full{},
                                         UIGlobals::GetMainWindow(),
                                         look, _("FLARM range"));
  dialog.SetWidget(look, UIGlobals::GetLook().flarm_dialog, device);
  dialog.AddButton(_("Reset"), [&dialog](){
    dialog.GetWidget().Reset();
  });
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}

/**
 * The share of the reports in a sector that the plot shows as its
 * range.  The mean of all reports mostly says how far away the
 * traffic was; most reports lie closer than the range.
 */
static constexpr double ESTIMATE_PERCENTILE = 0.9;

static std::vector<FlarmRangeSector>
ToSectors(const FlarmRangeEstimate &estimate) noexcept
{
  std::vector<FlarmRangeSector> sectors(FlarmRangeEstimate::SECTORS);
  for (unsigned i = 0; i < sectors.size(); ++i) {
    const auto &sector = estimate.sectors[i];
    sectors[i].range = sector.Percentile(ESTIMATE_PERCENTILE);
    sectors[i].significant = sector.count >= MIN_POINTS;
    if (sector.count > 0)
      sectors[i].maximum = sector.maximum;
  }

  return sectors;
}

class FlarmRangeEstimateWidget final : public RowFormWidget {
  enum Controls {
    POINTS,
    PERIOD,
    BELOW_MINIMUM,
    RANGE_SETTING,
  };

  FlarmDevice &device;
  FlarmRangeComputer &computer;
  const FlarmTrafficLook &look;

  FlarmRangePlot *plot;

public:
  FlarmRangeEstimateWidget(const DialogLook &_dialog_look,
                           const FlarmTrafficLook &_look,
                           FlarmDevice &_device,
                           FlarmRangeComputer &_computer) noexcept
    :RowFormWidget(_dialog_look), device(_device), computer(_computer),
     look(_look) {}

  void Reset() noexcept;

private:
  void Update() noexcept;

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};

void
FlarmRangeEstimateWidget::Prepare(ContainerWindow &,
                                  const PixelRect &) noexcept
{
  AddReadOnly(_("Data points"),
              _("Position reports of other FLARM aircraft received in "
                "flight.  XCSoar estimates the range from them: the "
                "sector shows the distance 90 % of the reports stay "
                "within, the line across it the largest one."));
  AddReadOnly(_("Period"));
  AddReadOnly(_("Below minimum range"),
              _("Measured sectors whose estimated range is shorter than "
                "FLARM's minimum: 2 nm ahead, 1 nm to the sides and "
                "behind.  Check the antenna installation."));
  AddReadOnly(_("Range setting"),
              _("The FLARM reports no aircraft beyond this distance.  "
                "The plot shows it as a dashed circle when it is within "
                "the plotted range."));

  auto window = std::make_unique<FlarmRangePlot>(GetLook(), look);
  window->Create((ContainerWindow &)GetWindow(), {0, 0, 100, 100},
                 WindowStyle{});
  plot = window.get();
  AddRemaining(std::move(window));

  /* the setting caps what the FLARM reports, so an estimate that
     reaches it says more about the setting than about the antenna;
     read it the way the FLARM dialog reads its settings, which works
     while the port's receive thread runs */
  try {
    static const char *const names[] = { "RANGE", nullptr };
    PopupOperationEnvironment env;
    if (device.RequestAllSettings(names, env))
      if (const auto value = device.GetSetting("RANGE")) {
        char *end;
        const unsigned range = ParseUnsigned(value->c_str(), &end);
        if (end != value->c_str()) {
          plot->SetLimit(range);
          if (range >= 65535)
            SetText(RANGE_SETTING, _("Unlimited"));
          else
            SetText(RANGE_SETTING, FormatUserDistanceSmart(range));
        }
      }
  } catch (...) {
  }

  Update();
}

void
FlarmRangeEstimateWidget::Update() noexcept
{
  const auto estimate = computer.GetEstimate();
  StaticString<64> buffer;

  if (const auto count = estimate.GetCount(); count > 0) {
    buffer.Format("%u", unsigned(count));
    SetText(POINTS, buffer);
  } else
    ClearText(POINTS);

  if (estimate.first && estimate.last) {
    /* the dates only; the reports are UTC */
    char first[16], last[16];
    FormatISO8601(first, BrokenDate{BrokenDateTime{*estimate.first}});
    FormatISO8601(last, BrokenDate{BrokenDateTime{*estimate.last}});
    buffer.Format("%s – %s", first, last);
    SetText(PERIOD, buffer);
  } else
    ClearText(PERIOD);

  auto sectors = ToSectors(estimate);
  unsigned measured = 0, below = 0;
  for (unsigned i = 0; i < sectors.size(); ++i) {
    if (sectors[i].significant)
      ++measured;
    if (IsBelowMinimum(sectors[i], i, sectors.size()))
      ++below;
  }

  if (measured > 0) {
    buffer.Format("%u / %u", below, measured);
    SetText(BELOW_MINIMUM, buffer);
  } else
    ClearText(BELOW_MINIMUM);

  plot->SetSectors(std::move(sectors));
}

void
FlarmRangeEstimateWidget::Reset() noexcept
{
  if (ShowMessageBox(_("Reset the range estimate? Do this after a "
                       "change to the antenna; XCSoar then collects it "
                       "anew."),
                     _("FLARM range estimate"),
                     MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  computer.Reset();
  Update();
}

void
FlarmRangeEstimateDialog(FlarmDevice &device, FlarmRangeComputer &computer)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<FlarmRangeEstimateWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look, _("FLARM range estimate"));
  dialog.SetWidget(look, UIGlobals::GetLook().flarm_dialog, device, computer);
  dialog.AddButton(_("Reset"), [&dialog](){
    dialog.GetWidget().Reset();
  });
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
