// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMList.hpp"
#include "LogFile.hpp"

#ifdef HAVE_HTTP

#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Button.hpp"
#include "Widget/ListWidget.hpp"
#include "ui/control/List.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Look/DialogLook.hpp"
#include "util/Compiler.h"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "NetComponents.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "NOTAM/NOTAM.hpp"
#include "NOTAM/Filter.hpp"
#include "NOTAM/NOTAMGlue.hpp"
#include "NOTAM/Converter.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Airspace.hpp"
#include "BackendComponents.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Formatter/UserUnits.hpp"
#include "Interface.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "Protection.hpp"
#include "util/StringFormat.hpp"
#include "util/TruncateString.hpp"
#include "Operation/Operation.hpp"
#include "util/UTF8.hpp"
#include "util/StringAPI.hxx"
#include "system/FileUtil.hpp"

#include <array>
#include <algorithm>
#include <cassert>
#include <ctime>
#include <iterator>
#include <string>
#include <string_view>

// Use full struct name to avoid collision with AirspaceClass::NOTAM enum
using NOTAMStruct = struct NOTAM;

static void
AppendFilterReason(std::string &text, const char *reason)
{
  if (!text.empty())
    text += ", ";

  text += reason;
}

[[nodiscard]]
static std::string
FormatFilterReasons(const NOTAMStruct &notam,
                    const NOTAMSettings &settings,
                    std::chrono::system_clock::time_point now)
{
  std::string reasons;
  const auto filter_reasons = NOTAMFilter::Evaluate(notam, settings, now);

  if (NOTAMFilter::HasFilterReason(filter_reasons,
                                   NOTAMFilter::FilterReason::IFR))
    AppendFilterReason(reasons, _("IFR-only"));

  if (NOTAMFilter::HasFilterReason(filter_reasons,
                                   NOTAMFilter::FilterReason::TIME))
    AppendFilterReason(reasons, _("not effective"));

  if (NOTAMFilter::HasFilterReason(filter_reasons,
                                   NOTAMFilter::FilterReason::RADIUS))
    AppendFilterReason(reasons, _("radius"));

  if (NOTAMFilter::HasFilterReason(filter_reasons,
                                   NOTAMFilter::FilterReason::QCODE))
    AppendFilterReason(reasons, _("Q-Code"));

  return reasons;
}

// Return UTF-8 text safe for UI, or a placeholder if invalid.
[[nodiscard]]
static std::string
SafeString(const std::string &input)
{
  if (input.empty()) {
    return {};
  }
  if (!ValidateUTF8(input)) {
    return std::string(_("[Invalid text]"));
  }
  return input;
}

[[nodiscard]]
static bool
HasHiddenQCodeToken(std::string_view hidden_list,
                    std::string_view qcode) noexcept
{
  if (qcode.empty())
    return false;

  size_t start = hidden_list.find_first_not_of(" \t,");
  while (start != std::string_view::npos) {
    const size_t end = hidden_list.find_first_of(" \t,", start);
    const size_t token_len = end == std::string_view::npos
      ? hidden_list.size() - start
      : end - start;

    if (token_len == qcode.size() &&
        StringIsEqualIgnoreCase(hidden_list.data() + start,
                                qcode.data(), qcode.size()))
      return true;

    if (end == std::string_view::npos)
      break;

    start = hidden_list.find_first_not_of(" \t,", end + 1);
  }

  return false;
}

[[nodiscard]]
static unsigned
CountNOTAMsWithQCodePrefix(std::string_view qcode) noexcept
{
  if (qcode.empty() || net_components == nullptr ||
      net_components->notam == nullptr)
    return 0;

  unsigned count = 0;
  try {
    const auto snapshot = net_components->notam->GetSnapshot();
    for (const auto &notam : snapshot.notams)
      if (NOTAMFilter::IsQCodeHidden(notam.feature_type, qcode))
        ++count;
  } catch (...) {
    LogError(std::current_exception(), "Failed to count NOTAM Q-code");
  }

  return count;
}

static bool
AddHiddenQCode(NOTAMSettings &settings, std::string_view qcode) noexcept
{
  if (qcode.empty() || HasHiddenQCodeToken(settings.hidden_qcodes, qcode))
    return true;

  const size_t length = settings.hidden_qcodes.length();
  const size_t separator = length > 0 ? 1 : 0;
  if (length + separator + qcode.size() + 1 >
      settings.hidden_qcodes.capacity())
    return false;

  if (separator > 0)
    settings.hidden_qcodes += ' ';

  settings.hidden_qcodes += qcode;
  return true;
}

static bool
RemoveHiddenQCode(NOTAMSettings &settings, std::string_view qcode) noexcept
{
  if (qcode.empty())
    return false;

  StaticString<256> remaining;
  bool removed = false;
  std::string_view hidden_list = settings.hidden_qcodes;
  size_t start = hidden_list.find_first_not_of(" \t,");
  while (start != std::string_view::npos) {
    const size_t end = hidden_list.find_first_of(" \t,", start);
    const auto token = hidden_list.substr(start, end == std::string_view::npos
                                          ? hidden_list.size() - start
                                          : end - start);
    const bool matches = token.size() <= qcode.size() &&
      StringIsEqualIgnoreCase(token.data(), qcode.data(), token.size());

    if (matches) {
      removed = true;
    } else {
      if (!remaining.empty())
        remaining += ' ';
      remaining += token;
    }

    if (end == std::string_view::npos)
      break;

    start = hidden_list.find_first_not_of(" \t,", end + 1);
  }

  if (removed)
    settings.hidden_qcodes = remaining;

  return removed;
}

static void
ApplyNOTAMFilterSettings(const NOTAMSettings &settings) noexcept
{
  Profile::Set(ProfileKeys::NOTAMHiddenQCodes, settings.hidden_qcodes);

  if (net_components != nullptr && net_components->notam != nullptr)
    net_components->notam->SetSettings(settings);

  if (net_components != nullptr && net_components->notam != nullptr &&
      data_components != nullptr && data_components->airspaces != nullptr) {
    try {
      const ScopeSuspendAllThreads suspend;
      net_components->notam->UpdateAirspaces(*data_components->airspaces);
      if (data_components->terrain != nullptr)
        SetAirspaceGroundLevels(*data_components->airspaces,
                                *data_components->terrain);
    } catch (...) {
      LogError(std::current_exception(),
               "Failed to apply NOTAM Q-code filter");
    }
  }
}


static StaticString<32>
FormatRelativeNotamTime(std::chrono::system_clock::duration duration)
{
  StaticString<32> result;
  const auto minutes =
    std::chrono::duration_cast<std::chrono::minutes>(duration);
  if (minutes.count() < 60) {
    result.Format(_("%dm"), static_cast<int>(minutes.count()));
    return result;
  }

  const auto rounded_hours = (minutes.count() + 59) / 60;
  if (rounded_hours < 48) {
    result.Format(_("%dh"), static_cast<int>(rounded_hours));
    return result;
  }

  result.Format(_("%dd"), static_cast<int>(rounded_hours / 24));
  return result;
}

static std::chrono::system_clock::time_point
GetCurrentNOTAMTimeUTC() noexcept
{
  const auto &basic = CommonInterface::Basic();
  return basic.time_available && basic.date_time_utc.IsDatePlausible()
    ? basic.date_time_utc.ToTimePoint()
    : std::chrono::system_clock::now();
}

[[nodiscard]]
static std::string
EllipsizeText(Canvas &canvas, const std::string &text,
              const unsigned max_width)
{
  if (max_width == 0)
    return {};

  if (canvas.CalcTextWidth(text) <= max_width)
    return text;

  constexpr std::string_view ellipsis = "...";
  const unsigned ellipsis_width = canvas.CalcTextWidth(ellipsis);
  if (ellipsis_width >= max_width)
    return std::string{ellipsis};

  std::string result;
  result.reserve(text.size());

  for (std::string_view remaining = text; !remaining.empty();) {
    const std::size_t sequence = SequenceLengthUTF8(remaining.front());
    if (sequence == 0 || sequence > remaining.size())
      break;

    std::string candidate = result;
    candidate.append(remaining.substr(0, sequence));
    candidate += ellipsis;

    if (canvas.CalcTextWidth(candidate) > max_width)
      break;

    result.append(remaining.substr(0, sequence));
    remaining.remove_prefix(sequence);
  }

  result += ellipsis;
  return result;
}

[[nodiscard]]
static unsigned
GetTextWidth(const PixelRect &rc, const TwoTextRowsRenderer &row_renderer)
{
  const int width = rc.GetWidth() - row_renderer.GetX();
  return width > 0 ? unsigned(width) : 0u;
}

class NOTAMListWidget final : public ListWidget {
  static constexpr unsigned HEADER_COUNT = 4;
  std::vector<NOTAMStruct> items;
  TwoTextRowsRenderer row_renderer;
  Button *details_button = nullptr;
  Button *filter_qcode_button = nullptr;
  Button *toggle_filter_button = nullptr;
  bool show_all = false;

public:
  NOTAMListWidget() = default;

  void UpdateList();

  void SetDetailsButton(Button *_details_button) noexcept {
    details_button = _details_button;
    if (details_button != nullptr)
      details_button->SetEnabled(false);
  }

  void SetFilterQCodeButton(Button *_filter_qcode_button) noexcept {
    filter_qcode_button = _filter_qcode_button;
    if (filter_qcode_button != nullptr)
      filter_qcode_button->SetEnabled(false);
  }

  void SetToggleFilterButton(Button *_toggle_filter_button) noexcept {
    toggle_filter_button = _toggle_filter_button;
    UpdateToggleFilterButton();
  }

  void ShowDetails() noexcept {
    OnActivateItem(GetList().GetCursorIndex());
  }

  void FilterSelectedQCode() noexcept;

  void ToggleFilter() noexcept {
    show_all = !show_all;
    UpdateToggleFilterButton();
    try {
      UpdateList();
    } catch (...) {
      LogError(std::current_exception(), "Failed to toggle NOTAM filter");
      ResetListAfterUpdateFailure();
    }
  }

  void UpdateButtons() noexcept {
    const unsigned index = GetList().GetCursorIndex();

    if (details_button != nullptr)
      details_button->SetEnabled(CanActivateItem(index));

    if (filter_qcode_button != nullptr) {
      const auto *notam = GetSelectableNOTAM(index);
      const auto &settings =
        CommonInterface::GetComputerSettings().airspace.notam;
      const bool has_qcode = notam != nullptr && !notam->feature_type.empty();
      filter_qcode_button->SetEnabled(has_qcode);
      if (has_qcode)
        filter_qcode_button->SetCaption(
          NOTAMFilter::IsQCodeHidden(notam->feature_type,
                                     settings.hidden_qcodes)
          ? _("Show Q-code")
          : _("Hide Q-code"));
    }
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    ListWidget::Show(rc);
    try {
      UpdateList();
    } catch (...) {
      LogError(std::current_exception(), "Failed to update NOTAM list");
      ResetListAfterUpdateFailure();
    }
  }

  /* virtual methods from ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from ListCursorHandler */
  void OnCursorMoved([[maybe_unused]] unsigned index) noexcept override {
    UpdateButtons();
  }

  bool CanActivateItem(unsigned index) const noexcept override {
    return index >= HEADER_COUNT && index < items.size();
  }

  void OnActivateItem(unsigned index) noexcept override;

private:
  void ResetListAfterUpdateFailure() noexcept {
    items.clear();
    GetList().SetLength(1);
    GetList().Invalidate();
    UpdateButtons();
  }

  void UpdateToggleFilterButton() noexcept {
    if (toggle_filter_button != nullptr)
      toggle_filter_button->SetCaption(show_all
                                       ? _("Hide Filtered")
                                       : _("Show All"));
  }

  const NOTAMStruct *GetSelectableNOTAM(unsigned index) const noexcept {
    return CanActivateItem(index) ? &items[index] : nullptr;
  }
};

void
NOTAMListWidget::Prepare(ContainerWindow &parent,
                         const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font,
                                          look.small_font));
}

void
NOTAMListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                             unsigned i) noexcept
{
  try {
    if (items.empty()) {
      assert(i == 0);
      row_renderer.DrawFirstRow(canvas, rc, _("No NOTAMs available"));
      row_renderer.DrawSecondRow(canvas, rc, "");
      return;
    }

    if (i >= items.size())
      return;

    assert(i < items.size());

    const auto &notam = items[i];

    // Check if this is a metadata header (first HEADER_COUNT items)
    const bool is_header = i < HEADER_COUNT;

    // Build user-friendly first row: "A1234/24 • EDDF • Active" or
    // "Future: starts in 2h"
    std::string first_row_text =
      SafeString(notam.number.empty() ? notam.id : notam.number);

    if (is_header) {
      // For headers, just show "Label: Value" format
      first_row_text += ": ";
      first_row_text += SafeString(notam.text);
    } else {
      // For actual NOTAMs, add location if available
      if (!notam.location.empty()) {
        first_row_text += " • ";
        first_row_text += SafeString(notam.location);
      }

      // Add status indicator
      const auto now = GetCurrentNOTAMTimeUTC();
      const bool is_perm = notam.end_time_permanent;
      if (now < notam.start_time) {
        const auto starts_in = FormatRelativeNotamTime(notam.start_time - now);
        StaticString<64> status;
        status.Format(_("Starts in %s"), starts_in.c_str());
        first_row_text += " • ";
        first_row_text += status.c_str();
      } else if (!is_perm && now > notam.end_time) {
        first_row_text += " • ";
        first_row_text += _("Expired");
      } else {
        first_row_text += " • ";
        first_row_text += _("Active");
      }

      // Add filtered indicator
      const auto &settings =
        CommonInterface::GetComputerSettings().airspace.notam;
      const bool is_filtered =
        !NOTAMFilter::ShouldDisplay(notam, settings, now, false);

      if (is_filtered) {
        first_row_text += " • ";
        first_row_text += _("Filtered");
      const auto reasons = FormatFilterReasons(notam, settings, now);
        if (!reasons.empty()) {
          first_row_text += ": ";
          first_row_text += reasons;
        }
      }
    }

    canvas.Select(row_renderer.GetFirstFont());
    first_row_text =
      EllipsizeText(canvas, first_row_text, GetTextWidth(rc, row_renderer));

    StaticString<512> first_row;
    CopyTruncateStringUTF8({first_row.buffer(), first_row.capacity()},
                 first_row_text.c_str(),
                 first_row.capacity() - 1);
    row_renderer.DrawFirstRow(canvas, rc, first_row.c_str());

    // For headers, skip the second row (text is already shown in first row)
    if (is_header) {
      row_renderer.DrawSecondRow(canvas, rc, "");
      return;
    }

    // Build user-friendly second row: truncated text with Q-code if available
    std::string second_row_text;

    // Add Q-code/feature type if available
    if (!notam.feature_type.empty()) {
      second_row_text = SafeString(notam.feature_type);
      second_row_text += ": ";
    }

    // Add text, truncated to fit
    if (!notam.text.empty()) {
      std::string text = SafeString(notam.text);
      // Remove newlines and extra spaces for better display
      for (auto &ch : text) {
        if (ch == '\n' || ch == '\r') ch = ' ';
      }
      second_row_text += text;
    }

    if (!second_row_text.empty()) {
      canvas.Select(row_renderer.GetSecondFont());
      second_row_text =
        EllipsizeText(canvas, second_row_text, GetTextWidth(rc, row_renderer));

      StaticString<512> second_row;
      CopyTruncateStringUTF8({second_row.buffer(), second_row.capacity()},
                             second_row_text.c_str(),
                             second_row.capacity() - 1);
      row_renderer.DrawSecondRow(canvas, rc, second_row.c_str());
    } else {
      row_renderer.DrawSecondRow(canvas, rc, "");
    }
  } catch (...) {
#ifndef NDEBUG
    LogError(std::current_exception(), "Failed to paint NOTAM list item");
#endif
    row_renderer.DrawFirstRow(canvas, rc, _("NOTAM render error"));
    row_renderer.DrawSecondRow(canvas, rc, "");
  }
}

void
NOTAMListWidget::OnActivateItem(unsigned i) noexcept
{
  if (!CanActivateItem(i))
    return;

  try {
    auto airspace = NOTAMConverter::BuildNOTAMAirspace(items[i], true);
    if (!airspace)
      return;

    dlgAirspaceDetails(std::move(airspace),
                       backend_components != nullptr
                       ? backend_components->GetAirspaceWarnings()
                       : nullptr);
  } catch (...) {
    LogError(std::current_exception(), "Failed to show NOTAM details");
  }
}

void
NOTAMListWidget::FilterSelectedQCode() noexcept
{
  const auto *notam = GetSelectableNOTAM(GetList().GetCursorIndex());
  if (notam == nullptr || notam->feature_type.empty())
    return;

  const auto qcode = std::string_view{notam->feature_type};
  auto &settings = CommonInterface::SetComputerSettings().airspace.notam;

  if (NOTAMFilter::IsQCodeHidden(qcode, settings.hidden_qcodes)) {
    StaticString<512> message;
    message.Format(_("Show Q-code %s?\n"
                     "This will remove matching Q-code filters, which may "
                     "also show other NOTAMs."),
                   notam->feature_type.c_str());

    if (ShowMessageBox(message.c_str(), _("NOTAM"),
                       MB_YESNO | MB_ICONQUESTION) != IDYES)
      return;

    if (!RemoveHiddenQCode(settings, qcode))
      return;

    ApplyNOTAMFilterSettings(settings);
    try {
      UpdateList();
    } catch (...) {
      LogError(std::current_exception(), "Failed to refresh NOTAM list");
      ResetListAfterUpdateFailure();
    }
    return;
  }

  const unsigned affected_count = CountNOTAMsWithQCodePrefix(qcode);
  StaticString<512> message;
  message.Format(_("Hide Q-code %s?\n"
                   "This will hide %u currently loaded NOTAM(s) with a "
                   "matching Q-code prefix from the map and mark them as "
                   "filtered in the NOTAM list."),
                 notam->feature_type.c_str(), affected_count);

  if (ShowMessageBox(message.c_str(), _("NOTAM"),
                     MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  if (!AddHiddenQCode(settings, qcode)) {
    ShowMessageBox(_("The hidden Q-code list is full. Remove unused filters "
                     "in NOTAM settings first."),
                   _("NOTAM"), MB_OK | MB_ICONEXCLAMATION);
    return;
  }

  ApplyNOTAMFilterSettings(settings);
  try {
    UpdateList();
  } catch (...) {
    LogError(std::current_exception(), "Failed to refresh NOTAM list");
    ResetListAfterUpdateFailure();
  }
}

void
NOTAMListWidget::UpdateList()
{
  items.clear();

  if (net_components && net_components->notam) {
    const auto snapshot = net_components->notam->GetSnapshot();
    const auto &notams = snapshot.notams;
    const auto now = GetCurrentNOTAMTimeUTC();
    const auto &settings =
      CommonInterface::GetComputerSettings().airspace.notam;
    const unsigned visible_count =
      static_cast<unsigned>(std::count_if(notams.begin(), notams.end(),
        [&](const auto &notam) {
          return NOTAMFilter::ShouldDisplay(notam, settings, now, false);
        }));

    // Add header items with statistics
    NOTAMStruct header1, header2, header3, header4;
    
    // Last Update time
    std::time_t last_update = snapshot.last_update_time;
    header1.number = _("Last Update (local)");
    if (last_update > 0) {
      struct tm tm_buf;
#ifdef _WIN32
      const auto *tm =
        (localtime_s(&tm_buf, &last_update) == 0) ? &tm_buf : nullptr;
#else
      const auto *tm = localtime_r(&last_update, &tm_buf);
#endif
      if (tm != nullptr) {
        char time_buffer[64];
        StringFormat(time_buffer, sizeof(time_buffer),
                     "%04d-%02d-%02d %02d:%02d",
                     tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                     tm->tm_hour, tm->tm_min);
        header1.text = time_buffer;
      } else {
        header1.text = _("Unknown");
      }
    } else {
      header1.text = _("Never");
    }
    
    // Distance from last update
    const auto &basic = CommonInterface::Basic();
    GeoPoint last_loc = snapshot.last_update_location;
    header2.number = _("Distance");
    if (basic.location_available && basic.location.IsValid() &&
        last_loc.IsValid()) {
      double dist_m = basic.location.Distance(last_loc);
      header2.text = FormatUserDistanceSmart(dist_m).c_str();
    } else {
      header2.text = _("Unknown");
    }
    
    // NOTAM counts
    char count_buffer[64];
    header3.number = _("NOTAMs");
    StringFormat(count_buffer, sizeof(count_buffer), _("%u total"),
                 static_cast<unsigned>(notams.size()));
    header3.text = count_buffer;
    
    header4.number = _("After Filtering");
    StringFormat(count_buffer, sizeof(count_buffer), _("%u visible"),
                 visible_count);
    header4.text = count_buffer;
    
    const std::array<NOTAMStruct, HEADER_COUNT> headers = {
      header1, header2, header3, header4,
    };
    items.insert(items.end(), headers.begin(), headers.end());
    
    if (!show_all) {
      std::copy_if(notams.begin(), notams.end(), std::back_inserter(items),
                   [&](const auto &notam) {
                     return NOTAMFilter::ShouldDisplay(notam, settings, now,
                                                       false);
                   });
    } else {
      items.insert(items.end(), notams.begin(), notams.end());
    }
  } else {
    LogFmt("NOTAM: UpdateList - net_components or notam is null");
  }

#ifndef NDEBUG
  LogFmt("NOTAM: UpdateList - rows={} (headers={}, notams={})",
         static_cast<unsigned>(items.size()),
         HEADER_COUNT,
         static_cast<unsigned>(items.size() >= HEADER_COUNT
                               ? items.size() - HEADER_COUNT
                               : 0));
#endif
  GetList().SetLength(std::max(static_cast<size_t>(1), items.size()));
  GetList().Invalidate();
  UpdateButtons();
}

void
ShowNOTAMListDialog(UI::SingleWindow &parent)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  auto list_widget = std::make_unique<NOTAMListWidget>();
  NOTAMListWidget *const list = list_widget.get();
  WidgetDialog dialog(WidgetDialog::Auto{}, parent, look, _("NOTAMs"),
                      list_widget.release());
  list->SetDetailsButton(dialog.AddButton(_("Details"),
                                          [list](){ list->ShowDetails(); }));
  list->SetFilterQCodeButton(
    dialog.AddButton(_("Hide Q-code"),
                     [list](){ list->FilterSelectedQCode(); }));
  list->SetToggleFilterButton(
    dialog.AddButton(_("Show All"),
                     [list](){ list->ToggleFilter(); }));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}

#else // !HAVE_HTTP

void
ShowNOTAMListDialog(UI::SingleWindow &)
{
  // NOTAM support not compiled in - do nothing
  LogFormat("NOTAM: ShowNOTAMListDialog called, but NOTAM support is not "
            "available");
}

#endif
