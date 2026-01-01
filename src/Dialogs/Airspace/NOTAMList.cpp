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
#include "Interface.hpp"
#include "util/StringFormat.hpp"
#include "Operation/Operation.hpp"
#include "util/UTF8.hpp"
#include "util/ConvertString.hpp"
#include "system/FileUtil.hpp"

#include <array>
#include <iterator>

// Use full struct name to avoid collision with AirspaceClass::NOTAM enum
using NOTAMStruct = struct NOTAM;

#include <cassert>
#include <algorithm>

// Helper function to safely convert UTF-8 std::string to tstring
static tstring SafeString(const std::string &input) {
  if (input.empty()) {
    return tstring();
  }
  if (!ValidateUTF8(input)) {
    return _("[Invalid text]");
  }
  
#ifdef _UNICODE
  // On Windows (wide character build), convert UTF-8 to wide string
  UTF8ToWideConverter conv(input.c_str());
  if (conv.IsValid()) {
    return tstring(conv.c_str());
  } else {
    return _("[Conversion failed]");
  }
#else
  // On Unix/iOS/Android (narrow character build), tstring is already UTF-8
  return tstring(input);
#endif
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
    UpdateList();
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
  if (items.empty()) {
    assert(i == 0);
    row_renderer.DrawFirstRow(canvas, rc, _("No NOTAMs available"));
    return;
  }

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
    first_row += _T(": ");
    first_row += SafeString(notam.text).c_str();
  } else {
    // For actual NOTAMs, add location if available
    if (!notam.location.empty()) {
      first_row += _T(" • ");
      first_row += SafeString(notam.location).c_str();
    }
    
    // Add status indicator
    auto now = std::chrono::system_clock::now();
    const bool is_perm = notam.end_time >= NOTAMTime::PermanentEndTime();
    if (now < notam.start_time) {
      auto starts_in =
        std::chrono::duration_cast<std::chrono::hours>(notam.start_time - now);
      if (starts_in.count() < 24) {
        first_row.AppendFormat(_T(" • %s %dh"), _("Starts in"),
                               (int)starts_in.count());
      } else {
        first_row.AppendFormat(_T(" • %s %dd"), _("Starts in"),
                               (int)(starts_in.count() / 24));
      }
    } else if (!is_perm && now > notam.end_time) {
      first_row += _T(" • ");
      first_row += _("Expired");
    } else {
      first_row += _T(" • ");
      first_row += _("Active");
    }
    
    // Add filtered indicator
    const auto &settings =
      CommonInterface::GetComputerSettings().airspace.notam;
    const bool is_filtered =
      !NOTAMFilter::ShouldDisplay(notam, settings, false);
    
    if (is_filtered) {
      first_row += _T(" • ");
      first_row += _("Filtered");
    }
  }
  
  row_renderer.DrawFirstRow(canvas, rc, first_row.c_str());
  
  // For headers, skip the second row (text is already shown in first row)
  if (is_header) {
    row_renderer.DrawSecondRow(canvas, rc, _T(""));
    return;
  }
  
  // Build user-friendly second row: truncated text with Q-code if available
  StaticString<512> second_row;
  
  // Add Q-code/feature type if available
  if (!notam.feature_type.empty()) {
    second_row = SafeString(notam.feature_type).c_str();
    second_row += _T(": ");
  }
  
  // Add text, truncated to fit
  if (!notam.text.empty()) {
    tstring text = SafeString(notam.text);
    // Remove newlines and extra spaces for better display
    for (auto &ch : text) {
      if (ch == '\n' || ch == '\r') ch = ' ';
    }
    // Truncate if too long
    const size_t max_len = 120;
    if (text.length() > max_len) {
      text = text.substr(0, max_len - 3);
      text += _T("...");
    }
    second_row += text.c_str();
  }
  
  if (!second_row.empty()) {
    row_renderer.DrawSecondRow(canvas, rc, second_row.c_str());
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
    if (last_update > 0) {
      struct tm tm_buf;
#ifdef _WIN32
      const auto *tm =
        (localtime_s(&tm_buf, &last_update) == 0) ? &tm_buf : nullptr;
#else
      const auto *tm = localtime_r(&last_update, &tm_buf);
#endif
      if (tm != nullptr) {
        TCHAR time_buffer[64];
        _stprintf(time_buffer, _T("%04d-%02d-%02d %02d:%02d"),
                  tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                  tm->tm_hour, tm->tm_min);
        WideToUTF8Converter number_conv(_("Last Update"));
        WideToUTF8Converter text_conv(time_buffer);
        if (number_conv.IsValid()) header1.number = number_conv.c_str();
        if (text_conv.IsValid()) header1.text = text_conv.c_str();
      } else {
        WideToUTF8Converter number_conv(_("Last Update"));
        WideToUTF8Converter text_conv(_("Unknown"));
        if (number_conv.IsValid()) header1.number = number_conv.c_str();
        if (text_conv.IsValid()) header1.text = text_conv.c_str();
      }
    } else {
      WideToUTF8Converter number_conv(_("Last Update"));
      WideToUTF8Converter text_conv(_("Never"));
      if (number_conv.IsValid()) header1.number = number_conv.c_str();
      if (text_conv.IsValid()) header1.text = text_conv.c_str();
    }
    
    // Distance from last update
    const auto &basic = CommonInterface::Basic();
    GeoPoint last_loc = net_components->notam->GetLastUpdateLocation();
    if (basic.location_available && basic.location.IsValid() &&
        last_loc.IsValid()) {
      double dist_m = basic.location.Distance(last_loc);
      TCHAR dist_buffer[64];
      StringFormat(
          dist_buffer,
          std::size(dist_buffer),
          _T("%.1f km"),
          dist_m / 1000.0);
      WideToUTF8Converter number_conv(_("Distance"));
      WideToUTF8Converter text_conv(dist_buffer);
      if (number_conv.IsValid()) header2.number = number_conv.c_str();
      if (text_conv.IsValid()) header2.text = text_conv.c_str();
    } else {
      WideToUTF8Converter number_conv(_("Distance"));
      WideToUTF8Converter text_conv(_("Unknown"));
      if (number_conv.IsValid()) header2.number = number_conv.c_str();
      if (text_conv.IsValid()) header2.text = text_conv.c_str();
    }
    
    // NOTAM counts
    NOTAMGlue::FilterStats stats = net_components->notam->GetFilterStats();
    TCHAR count_buffer[64];
    StringFormat(
        count_buffer,
        std::size(count_buffer),
        _T("%u total"),
        stats.total);
    WideToUTF8Converter number_conv(_("NOTAMs"));
    WideToUTF8Converter text_conv(count_buffer);
    if (number_conv.IsValid()) header3.number = number_conv.c_str();
    if (text_conv.IsValid()) header3.text = text_conv.c_str();
    
    StringFormat(
        count_buffer,
        std::size(count_buffer),
        _T("%u visible"),
        stats.final_count);
    WideToUTF8Converter number_conv2(_("After Filtering"));
    WideToUTF8Converter text_conv2(count_buffer);
    if (number_conv2.IsValid()) header4.number = number_conv2.c_str();
    if (text_conv2.IsValid()) header4.text = text_conv2.c_str();
    
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
