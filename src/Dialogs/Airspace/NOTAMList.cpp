// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMList.hpp"

#ifdef HAVE_HTTP

#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/RowFormWidget.hpp"
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
#include "LogFile.hpp"
#include "util/UTF8.hpp"
#include "system/FileUtil.hpp"

// Use full struct name to avoid collision with AirspaceClass::NOTAM enum
using NOTAMStruct = struct NOTAM;

#include <cassert>
#include <algorithm>

// Helper function to safely get a displayable string
static const char* SafeString(const std::string &input) {
  if (input.empty() || ValidateUTF8(input)) {
    return input.c_str();
  }
  return "[Invalid text]";
}

class NOTAMListWidget final : public ListWidget {
  std::vector<NOTAMStruct> items;
  TwoTextRowsRenderer row_renderer;

public:
  NOTAMListWidget() = default;

  void UpdateList();
  void ReloadNOTAMs();
  void LoadCachedNOTAMs();
  void ClearNOTAMs();
  void AddNOTAMsToAirspaceDatabase();

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
  
  // Use safe strings to prevent UTF-8 assertion failures
  row_renderer.DrawFirstRow(canvas, rc, SafeString(notam.number));
  
  // Draw the second row with the detailed text if it exists
  if (!notam.text.empty()) {
    row_renderer.DrawSecondRow(canvas, rc, SafeString(notam.text));
  }
}

void
NOTAMListWidget::UpdateList()
{
  items.clear();

  if (net_components && net_components->notam) {
    // Get NOTAM count first
    const unsigned count = net_components->notam->GetNOTAMs(nullptr, 0);
    LogFormat("NOTAM: UpdateList - GetNOTAMs(nullptr, 0) returned count=%u", count);
    
    if (count > 0) {
      // Allocate buffer and get NOTAMs
      items.resize(count);
      const unsigned actual_count = net_components->notam->GetNOTAMs(
        items.data(), count);
      items.resize(actual_count);
      LogFormat("NOTAM: UpdateList - GetNOTAMs(buffer, %u) returned actual_count=%u", count, actual_count);
    }
  } else {
    LogFormat("NOTAM: UpdateList - net_components or notam is null");
  }

  LogFormat("NOTAM: UpdateList - final items.size()=%u", static_cast<unsigned>(items.size()));
  GetList().SetLength(std::max((size_t)1, items.size()));
  GetList().Invalidate();
}

void
NOTAMListWidget::ReloadNOTAMs()
{
  if (!net_components || !net_components->notam) {
    LogFormat("NOTAM: Cannot reload - net_components or NOTAM system not available");
    return;
  }

  // Check if NOTAM system is enabled first
  const auto &settings = CommonInterface::GetComputerSettings().airspace.notam;
  if (!settings.enabled) {
    LogFormat("NOTAM: System is disabled in settings");
    items.clear();
    items.emplace_back();
    items.back().id = "DISABLED";
    items.back().number = "NOTAM System Disabled";
    items.back().text = "Enable NOTAM support in Settings > Airspace";
    
    GetList().SetLength(std::max((size_t)1, items.size()));
    GetList().Invalidate();
    return;
  }

  // Get current location for NOTAM loading
  const auto &basic = CommonInterface::Basic();
  GeoPoint location;
  
  if (basic.location_available && basic.location.IsValid()) {
    // Check if location is meaningful (not 0,0 which is often a default/invalid GPS reading)
    double lat = basic.location.latitude.Degrees();
    double lon = basic.location.longitude.Degrees();
    
    if (std::abs(lat) > 0.001 || std::abs(lon) > 0.001) {
      location = basic.location;
      LogFormat("NOTAM: Using GPS location: %.4f, %.4f", lat, lon);
    } else {
      // GPS reports 0,0 which is likely invalid
      // TODO: Figure out why GPS is reporting 0,0
      location = GeoPoint{Angle::Degrees(8.5622), Angle::Degrees(50.0333)};
      LogFormat("NOTAM: GPS location is 0,0 (invalid), using default location: %.4f, %.4f", 
                location.latitude.Degrees(), location.longitude.Degrees());
    }
  } else {
    // Use a default location for testing (example: EDDF - Frankfurt Airport)
    location = GeoPoint{Angle::Degrees(8.5622), Angle::Degrees(50.0333)};
    LogFormat("NOTAM: No GPS available, using default location: %.4f, %.4f", 
              location.latitude.Degrees(), location.longitude.Degrees());
  }
  
  LogFormat("NOTAM: Starting reload with API: %s, radius: %u km", 
            settings.api_base_url, settings.radius_km);
  
  // Show loading message temporarily
  items.clear();
  items.emplace_back();
  items.back().id = "LOADING";
  items.back().number = "Loading NOTAMs...";
  items.back().text = "Please wait...";
  
  GetList().SetLength(std::max((size_t)1, items.size()));
  GetList().Invalidate();
  
  // Trigger NOTAM loading
  class SimpleOperationEnvironment : public OperationEnvironment {
  public:
    void SetErrorMessage(const TCHAR *) noexcept override {}
    void SetText(const TCHAR *) noexcept override {}
    void SetProgressRange(unsigned) noexcept override {}
    void SetProgressPosition(unsigned) noexcept override {}
    void SetCancelHandler(std::function<void()>) noexcept override {}
    bool IsCancelled() const noexcept override { return false; }
    void Sleep(std::chrono::steady_clock::duration) noexcept override {}
  };
  
  SimpleOperationEnvironment operation;
  net_components->notam->LoadNOTAMs(location, operation);
  
  LogFormat("NOTAM: Load operation completed, updating display");
  
  // Update the list with actual NOTAMs
  UpdateList();
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
    items.back().number = "NOTAM System Disabled";
    items.back().text = "Enable NOTAM support in Settings > Airspace";
    
    GetList().SetLength(std::max((size_t)1, items.size()));
    GetList().Invalidate();
    return;
  }

  // Get current location for loading cached NOTAMs
  const auto &basic = CommonInterface::Basic();
  GeoPoint location;
  
  if (basic.location_available && basic.location.IsValid()) {
    // Check if location is meaningful (not 0,0 which is often a default/invalid GPS reading)
    double lat = basic.location.latitude.Degrees();
    double lon = basic.location.longitude.Degrees();
    
    if (std::abs(lat) > 0.001 || std::abs(lon) > 0.001) {
      location = basic.location;
      LogFormat("NOTAM: Loading cache for GPS location: %.4f, %.4f", lat, lon);
    } else {
      // GPS reports 0,0 which is likely invalid (middle of ocean)
      location = GeoPoint{Angle::Degrees(8.5622), Angle::Degrees(50.0333)};
      LogFormat("NOTAM: GPS location is 0,0 (invalid), using default location: %.4f, %.4f", 
                location.latitude.Degrees(), location.longitude.Degrees());
    }
  } else {
    // Use the same default location as in ReloadNOTAMs
    location = GeoPoint{Angle::Degrees(8.5622), Angle::Degrees(50.0333)};
    LogFormat("NOTAM: Loading cache for default location: %.4f, %.4f", 
              location.latitude.Degrees(), location.longitude.Degrees());
  }
  
  // Load cached NOTAMs synchronously (location is only used for logging context)
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
NOTAMListWidget::ClearNOTAMs()
{
  if (!net_components || !net_components->notam) {
    LogFormat("NOTAM: Cannot clear - net_components or NOTAM system not available");
    return;
  }

  LogFormat("NOTAM: Clearing all NOTAMs from memory and cache");
  
  // Clear NOTAMs from memory
  net_components->notam->Clear();
  
  // Also delete the cache file
  const auto &basic = CommonInterface::Basic();
  GeoPoint location;
  
  if (basic.location_available && basic.location.IsValid()) {
    // Check if location is meaningful (not 0,0)
    double lat = basic.location.latitude.Degrees();
    double lon = basic.location.longitude.Degrees();
    
    if (std::abs(lat) > 0.001 || std::abs(lon) > 0.001) {
      location = basic.location;
    } else {
      // Use default location for cache file path
      location = GeoPoint{Angle::Degrees(8.5622), Angle::Degrees(50.0333)};
    }
  } else {
    // Use default location for cache file path
    location = GeoPoint{Angle::Degrees(8.5622), Angle::Degrees(50.0333)};
  }
  
  // Try to delete the cache file
  try {
    auto cache_path = net_components->notam->GetNOTAMCacheFilePath();
    if (File::Delete(cache_path)) {
      LogFormat("NOTAM: Cache file deleted: %s", cache_path.c_str());
    } else {
      LogFormat("NOTAM: Failed to delete cache file: %s", cache_path.c_str());
    }
  } catch (const std::exception &e) {
    LogFormat("NOTAM: Error deleting cache file: %s", e.what());
  }
  
  // Show cleared message
  items.clear();
  items.emplace_back();
  items.back().id = "CLEARED";
  items.back().number = "NOTAMs Cleared";
  items.back().text = "All NOTAMs have been cleared from memory and cache";
  
  GetList().SetLength(std::max((size_t)1, items.size()));
  GetList().Invalidate();
  
  LogFormat("NOTAM: Clear operation completed");
}

void
NOTAMListWidget::AddNOTAMsToAirspaceDatabase()
{
  if (!net_components || !net_components->notam) {
    LogFormat("NOTAM: Cannot add to airspace database - net_components or NOTAM system not available");
    return;
  }

  if (!data_components || !data_components->airspaces) {
    LogFormat("NOTAM: Cannot add to airspace database - airspace database not available");
    return;
  }

  LogFormat("NOTAM: Adding NOTAMs to airspace database");
  
  // Add the NOTAMs as airspaces
  // @TODO: This will lead to duplicates/accumulation if called multiple times without clearing first, how should we handle this?
  net_components->notam->UpdateAirspaces(*data_components->airspaces);
  
  LogFormat("NOTAM: NOTAMs successfully added to airspace database");
}

class NOTAMListButtons final : public RowFormWidget {
  NOTAMListWidget &list_widget;

public:
  NOTAMListButtons(const DialogLook &look, NOTAMListWidget &_list_widget) noexcept
    :RowFormWidget(look), list_widget(_list_widget) {}

  void Prepare([[maybe_unused]] ContainerWindow &parent, 
               [[maybe_unused]] const PixelRect &rc) noexcept override {
    AddButton(_("Load from API"), [this](){
      list_widget.ReloadNOTAMs();
    });

    AddButton(_("Load from cache file"), [this](){
      list_widget.LoadCachedNOTAMs();
    });

    AddButton(_("Clear all NOTAMs"), [this](){
      list_widget.ClearNOTAMs();
    });

    AddButton(_("Add to Map"), [this](){
      list_widget.AddNOTAMsToAirspaceDatabase();
    });
  }
};



void
ShowNOTAMListDialog([[maybe_unused]] UI::SingleWindow &parent)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  
  auto list_widget = std::make_unique<NOTAMListWidget>();
  auto buttons_widget = std::make_unique<NOTAMListButtons>(look, *list_widget);
  
  // Store pointers before moving (for potential future use)
  [[maybe_unused]] NOTAMListWidget *list_ptr = list_widget.get();
  
  auto widget = std::make_unique<TwoWidgets>(
    std::move(list_widget),
    std::move(buttons_widget)
  );

  WidgetDialog dialog(WidgetDialog::Auto{}, parent, look, _("NOTAMs"), widget.release());
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}

#else // !HAVE_HTTP

void
ShowNOTAMListDialog(UI::SingleWindow &)
{
  // NOTAM support not compiled in - do nothing
}

#endif