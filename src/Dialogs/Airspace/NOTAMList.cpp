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
#include "NOTAM/NOTAMGlue.hpp"
#include "Interface.hpp"
#include "util/StringFormat.hpp"
#include "Operation/Operation.hpp"
#include "util/UTF8.hpp"
#include "util/ConvertString.hpp"
#include "system/FileUtil.hpp"

// Use full struct name to avoid collision with AirspaceClass::NOTAM enum
using NOTAMStruct = struct NOTAM;

#include <cassert>
#include <algorithm>
#include <cctype>

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

// Helper function to check if a NOTAM is filtered by current settings
static bool IsNOTAMFiltered(const NOTAMStruct &notam, const NOTAMSettings &settings) {
  // Check IFR filter
  if (!settings.show_ifr && !notam.traffic.empty() && notam.traffic == "I") {
    return true;
  }
  
  // Check if currently effective
  if (settings.show_only_effective) {
    auto now = std::chrono::system_clock::now();
    if (now < notam.start_time || now > notam.end_time) {
      return true;
    }
  }
  
  // Check radius filter
  if (settings.max_radius_m > 0 && notam.geometry.radius_meters > settings.max_radius_m) {
    return true;
  }
  
  // Check Q-code filters
  if (!notam.feature_type.empty() && !settings.hidden_qcodes.empty()) {
    WideToUTF8Converter hidden_conv(settings.hidden_qcodes.c_str());
    if (hidden_conv.IsValid()) {
      std::string hidden_list = hidden_conv.c_str();
      const auto &qcode = notam.feature_type;
      
      // Check each hidden Q-code prefix
      size_t start = 0;
      while (start < hidden_list.size()) {
        size_t end = hidden_list.find(',', start);
        if (end == std::string::npos) end = hidden_list.size();
        
        std::string prefix = hidden_list.substr(start, end - start);
        // Trim whitespace
        while (!prefix.empty() && std::isspace(prefix.front())) prefix.erase(0, 1);
        while (!prefix.empty() && std::isspace(prefix.back())) prefix.pop_back();
        
        if (!prefix.empty() && qcode.find(prefix) == 0) {
          return true;
        }
        
        start = end + 1;
      }
    }
  }
  
  return false;
}

class NOTAMListWidget final : public ListWidget {
  std::vector<NOTAMStruct> items;
  TwoTextRowsRenderer row_renderer;

public:
  NOTAMListWidget() = default;

  void UpdateList();
  void LoadCachedNOTAMs();

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
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return false; // No activation needed for NOTAM list
  }
};

void
NOTAMListWidget::Prepare(ContainerWindow &parent,
                         const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc, 
             row_renderer.CalculateLayout(*look.list.font, look.small_font));
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
  
  // Check if this is a metadata header (first 4 items)
  const bool is_header = i < 4;
  
  // Build user-friendly first row: "A1234/24 • EDDF • Active" or "Future: starts in 2h"
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
    if (now < notam.start_time) {
      auto starts_in = std::chrono::duration_cast<std::chrono::hours>(notam.start_time - now);
      if (starts_in.count() < 24) {
        first_row.AppendFormat(_T(" • %s %dh"), _("Starts in"), (int)starts_in.count());
      } else {
        first_row.AppendFormat(_T(" • %s %dd"), _("Starts in"), (int)(starts_in.count() / 24));
      }
    } else if (now > notam.end_time) {
      first_row += _T(" • ");
      first_row += _("Expired");
    } else {
      first_row += _T(" • ");
      first_row += _("Active");
    }
    
    // Add filtered indicator
    const auto &settings = CommonInterface::GetComputerSettings().airspace.notam;
    const bool is_filtered = IsNOTAMFiltered(notam, settings);
    
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
      const auto *tm = (localtime_s(&tm_buf, &last_update) == 0) ? &tm_buf : nullptr;
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
    if (basic.location_available && basic.location.IsValid() && last_loc.IsValid()) {
      double dist_m = basic.location.Distance(last_loc);
      TCHAR dist_buffer[64];
      StringFormat(dist_buffer, sizeof(dist_buffer) / sizeof(dist_buffer[0]),
                   _T("%.1f km"), dist_m / 1000.0);
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
    StringFormat(count_buffer, sizeof(count_buffer) / sizeof(count_buffer[0]),
                 _T("%u total"), stats.total);
    WideToUTF8Converter number_conv(_("NOTAMs"));
    WideToUTF8Converter text_conv(count_buffer);
    if (number_conv.IsValid()) header3.number = number_conv.c_str();
    if (text_conv.IsValid()) header3.text = text_conv.c_str();
    
    StringFormat(count_buffer, sizeof(count_buffer) / sizeof(count_buffer[0]),
                 _T("%u visible"), stats.final_count);
    WideToUTF8Converter number_conv2(_("After Filtering"));
    WideToUTF8Converter text_conv2(count_buffer);
    if (number_conv2.IsValid()) header4.number = number_conv2.c_str();
    if (text_conv2.IsValid()) header4.text = text_conv2.c_str();
    
    items.push_back(header1);
    items.push_back(header2);
    items.push_back(header3);
    items.push_back(header4);
    
    // Get all NOTAMs (max_count=0 means no limit)
    auto notams = net_components->notam->GetNOTAMs(0);
    items.insert(items.end(), notams.begin(), notams.end());
    LogFormat("NOTAM: UpdateList - received %u NOTAMs", static_cast<unsigned>(notams.size()));
  } else {
    LogFormat("NOTAM: UpdateList - net_components or notam is null");
  }

  LogFormat("NOTAM: UpdateList - final items.size()=%u", static_cast<unsigned>(items.size()));
  GetList().SetLength(std::max((size_t)1, items.size()));
  GetList().Invalidate();
}

void
NOTAMListWidget::LoadCachedNOTAMs()
{
  if (!net_components || !net_components->notam) {
    LogFormat("NOTAM: Cannot load cache - net_components or NOTAM system not available");
    return;
  }

  // Check if NOTAM system is enabled first
  const auto &settings = CommonInterface::GetComputerSettings().airspace.notam;
  if (!settings.enabled) {
    LogFormat("NOTAM: System is disabled in settings");
    items.clear();
    items.emplace_back();
    items.back().id = "DISABLED";
    
    // Convert translated strings from TCHAR to UTF-8 std::string
    WideToUTF8Converter number_conv(_("NOTAM System Disabled"));
    WideToUTF8Converter text_conv(_("Enable NOTAM support in Settings > Airspace"));
    
    items.back().number = number_conv.IsValid() ? number_conv.c_str() : "NOTAM System Disabled";
    items.back().text = text_conv.IsValid() ? text_conv.c_str() : "Enable NOTAM support in Settings > Airspace";
    
    GetList().SetLength(std::max((size_t)1, items.size()));
    GetList().Invalidate();
    return;
  }

  // Load cached NOTAMs synchronously
  const unsigned cached_count = net_components->notam->LoadCachedNOTAMs();
  
  if (cached_count > 0) {
    LogFormat("NOTAM: Successfully loaded %u cached NOTAMs", cached_count);
  } else {
    LogFormat("NOTAM: No cached NOTAMs available");
  }
  
  // Update the display with the loaded NOTAMs (or show empty list)
  UpdateList();
}


void
ShowNOTAMListDialog(UI::SingleWindow &parent)
{
  LogFormat("NOTAM: ShowNOTAMListDialog entered");
  
  const DialogLook &look = UIGlobals::GetDialogLook();
  
  auto list_widget = std::make_unique<NOTAMListWidget>();
  LogFormat("NOTAM: Created NOTAMListWidget");

  WidgetDialog dialog(WidgetDialog::Auto{}, parent, look, _("NOTAMs"), list_widget.release());
  LogFormat("NOTAM: Created WidgetDialog");
  
  dialog.AddButton(_("Close"), mrCancel);
  LogFormat("NOTAM: About to show modal dialog");
  
  dialog.ShowModal();
  
  LogFormat("NOTAM: Dialog closed");
}

#else // !HAVE_HTTP

void
ShowNOTAMListDialog(UI::SingleWindow &)
{
  // NOTAM support not compiled in - do nothing
  LogFormat("NOTAM: ShowNOTAMListDialog called, but NOTAM support is not available");
}

#endif
