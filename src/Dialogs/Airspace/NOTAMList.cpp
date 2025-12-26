// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMList.hpp"
#include "LogFile.hpp"

#ifdef HAVE_HTTP

#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "ui/control/List.hpp"
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
#include "Formatter/UserUnits.hpp"
#include "Interface.hpp"
#include "util/StringFormat.hpp"
#include "util/TruncateString.hpp"
#include "Operation/Operation.hpp"
#include "util/UTF8.hpp"
#include "system/FileUtil.hpp"

#include <array>
#include <algorithm>
#include <cassert>
#include <iterator>
#include <string>

// Use full struct name to avoid collision with AirspaceClass::NOTAM enum
using NOTAMStruct = struct NOTAM;

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

class NOTAMListWidget final : public ListWidget {
  static constexpr unsigned HEADER_COUNT = 4;
  std::vector<NOTAMStruct> items;
  TwoTextRowsRenderer row_renderer;

public:
  NOTAMListWidget() = default;

  void UpdateList();

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    ListWidget::Show(rc);
    try {
      UpdateList();
    } catch (...) {
      LogError(std::current_exception(), "Failed to update NOTAM list");
      items.clear();
      GetList().SetLength(1);
      GetList().Invalidate();
    }
  }

  /* virtual methods from ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from ListCursorHandler */
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept
    override {
    return false; // No activation needed for NOTAM list
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
    StaticString<256> first_row;
    first_row = SafeString(notam.number).c_str();

    if (is_header) {
      // For headers, just show "Label: Value" format
      first_row += ": ";
      first_row += SafeString(notam.text).c_str();
    } else {
      // For actual NOTAMs, add location if available
      if (!notam.location.empty()) {
        first_row += " • ";
        first_row += SafeString(notam.location).c_str();
      }

      // Add status indicator
      const auto now = GetCurrentNOTAMTimeUTC();
      const bool is_perm = notam.end_time >= NOTAMTime::PermanentEndTime();
      if (now < notam.start_time) {
        const auto starts_in = FormatRelativeNotamTime(notam.start_time - now);
        first_row.AppendFormat(" • %s %s", _("Starts in"),
                               starts_in.c_str());
      } else if (!is_perm && now > notam.end_time) {
        first_row += " • ";
        first_row += _("Expired");
      } else {
        first_row += " • ";
        first_row += _("Active");
      }

      // Add filtered indicator
      const auto &settings =
        CommonInterface::GetComputerSettings().airspace.notam;
      const bool is_filtered =
        !NOTAMFilter::ShouldDisplay(notam, settings, now, false);

      if (is_filtered) {
        first_row += " • ";
        first_row += _("Filtered");
      }
    }

    row_renderer.DrawFirstRow(canvas, rc, first_row.c_str());

    // For headers, skip the second row (text is already shown in first row)
    if (is_header) {
      row_renderer.DrawSecondRow(canvas, rc, "");
      return;
    }

    // Build user-friendly second row: truncated text with Q-code if available
    StaticString<512> second_row;

    // Add Q-code/feature type if available
    if (!notam.feature_type.empty()) {
      second_row = SafeString(notam.feature_type).c_str();
      second_row += ": ";
    }

    // Add text, truncated to fit
    if (!notam.text.empty()) {
      std::string text = SafeString(notam.text);
      // Remove newlines and extra spaces for better display
      for (auto &ch : text) {
        if (ch == '\n' || ch == '\r') ch = ' ';
      }
      // Truncate if too long (byte-limited, UTF-8 safe)
      constexpr std::size_t max_bytes = 120;
      static_assert(max_bytes >= 3, "max_bytes must leave room for ellipsis");
      if (text.length() > max_bytes) {
        const std::size_t truncate_at =
          TruncateStringUTF8(text.c_str(), max_bytes - 3, max_bytes - 3);
        text.resize(truncate_at);
        text += "...";
      }
      second_row += text.c_str();
    }

    if (!second_row.empty())
      row_renderer.DrawSecondRow(canvas, rc, second_row.c_str());
  } catch (...) {
#ifndef NDEBUG
    LogError(std::current_exception(), "Failed to paint NOTAM list item");
#endif
    row_renderer.DrawFirstRow(canvas, rc, _("NOTAM render error"));
    row_renderer.DrawSecondRow(canvas, rc, "");
  }
}

void
NOTAMListWidget::UpdateList()
{
  items.clear();

  if (net_components && net_components->notam) {
    // Add header items with statistics
    NOTAMStruct header1, header2, header3, header4;
    
    // Last Update time
    std::time_t last_update = net_components->notam->GetLastUpdateTime();
    header1.number = _("Last Update");
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
    GeoPoint last_loc = net_components->notam->GetLastUpdateLocation();
    header2.number = _("Distance");
    if (basic.location_available && basic.location.IsValid() &&
        last_loc.IsValid()) {
      double dist_m = basic.location.Distance(last_loc);
      header2.text = FormatUserDistanceSmart(dist_m).c_str();
    } else {
      header2.text = _("Unknown");
    }
    
    // NOTAM counts
    NOTAMGlue::FilterStats stats = net_components->notam->GetFilterStats();
    char count_buffer[64];
    header3.number = _("NOTAMs");
    StringFormat(count_buffer, sizeof(count_buffer), _("%u total"),
                 stats.total);
    header3.text = count_buffer;
    
    header4.number = _("After Filtering");
    StringFormat(count_buffer, sizeof(count_buffer), _("%u visible"),
                 stats.final_count);
    header4.text = count_buffer;
    
    const std::array<NOTAMStruct, HEADER_COUNT> headers = {
      header1, header2, header3, header4,
    };
    items.insert(items.end(), headers.begin(), headers.end());
    
    // Get all NOTAMs (max_count=0 means no limit)
    auto notams = net_components->notam->GetNOTAMs(0);
    items.insert(items.end(), notams.begin(), notams.end());
    LogFormat("NOTAM: UpdateList - received %u NOTAMs",
              static_cast<unsigned>(notams.size()));
  } else {
    LogFormat("NOTAM: UpdateList - net_components or notam is null");
  }

  LogFormat("NOTAM: UpdateList - final items.size()=%u",
            static_cast<unsigned>(items.size()));
  GetList().SetLength(std::max(static_cast<size_t>(1), items.size()));
  GetList().Invalidate();
}

void
ShowNOTAMListDialog(UI::SingleWindow &parent)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  auto list_widget = std::make_unique<NOTAMListWidget>();
  WidgetDialog dialog(WidgetDialog::Auto{}, parent, look, _("NOTAMs"),
                      list_widget.release());
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
