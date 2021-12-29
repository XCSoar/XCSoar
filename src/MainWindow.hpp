/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_MAIN_WINDOW_HXX
#define XCSOAR_MAIN_WINDOW_HXX

#include "ui/window/SingleWindow.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/event/Notify.hpp"
#include "BatteryTimer.hpp"
#include "Widget/ManagedWidget.hpp"
#include "UIUtil/GestureManager.hpp"

#include <cstdint>
#include <cassert>

#ifdef KOBO
#define HAVE_SHOW_MENU_BUTTON
#include "Menu/ShowMenuButton.hpp"
#endif

struct ComputerSettings;
struct MapSettings;
struct UIState;
struct Look;
class GlueMapWindow;
class Widget;
class RasterTerrain;
class TopographyStore;
class MapWindowProjection;
class PopupMessage;
class PluggableOperationEnvironment;
namespace InfoBoxLayout { struct Layout; }

/**
 * The XCSoar main window.
 */
class MainWindow : public UI::SingleWindow {
  static constexpr const TCHAR *title = _T("XCSoar");

  Look *look = nullptr;

#ifdef HAVE_SHOW_MENU_BUTTON
  ShowMenuButton *show_menu_button = nullptr;
#endif

  GlueMapWindow *map = nullptr;

  /**
   * A #Widget that is shown above the map.
   */
  Widget *top_widget = nullptr;

  /**
   * A #Widget that is shown below the map.
   */
  Widget *bottom_widget = nullptr;

  /**
   * A #Widget that is shown instead of the map.  The #GlueMapWindow
   * is hidden and the DrawThread is suspended while this attribute is
   * non-nullptr.
   */
  Widget *widget = nullptr;

  ManagedWidget vario{*this};

  ManagedWidget traffic_gauge{*this};
  bool suppress_traffic_gauge = false, force_traffic_gauge = false;

  ManagedWidget thermal_assistant{*this};

  bool dragging = false;
  GestureManager gestures;

public:
  PopupMessage *popup = nullptr;

private:
  UI::Notify terrain_loader_notify{[this]{ OnTerrainLoaded(); }};

  std::unique_ptr<PluggableOperationEnvironment> terrain_loader_env;

  /**
   * Called by the #MergeThread when new GPS data is available.
   */
  UI::Notify gps_notify{[this]{ OnGpsNotify(); }};

  /**
   * Called by the calculation thread when new calculation results are
   * available.  This updates the map and the info boxes.
   */
  UI::Notify calculated_notify{[this]{ OnCalculatedNotify(); }};

  /**
   * @see DeferredRestorePage()
   */
  UI::Notify restore_page_notify{[this]{ OnRestorePageNotify(); }};

  UI::PeriodicTimer timer{[this]{ RunTimer(); }};

  BatteryTimer battery_timer;

  PixelRect map_rect;
  bool FullScreen = false;

#ifndef ENABLE_OPENGL
  /**
   * This variable tracks whether the #DrawThread was suspended
   * because the map was replaced by a #Widget.
   */
  bool draw_suspended = false;
#endif

  bool restore_page_pending = false;

  /**
   * Has "late" initialization been done already?  Those are things
   * that must be run from inside the main event loop.  It will be
   * checked and set by OnTimer().
   */
  bool late_initialised = false;

public:
  ~MainWindow() noexcept override;

protected:
  /**
   * Is XCSoar already up and running?
   */
  bool IsRunning() noexcept {
    /* it is safe enough to say that XCSoar initialization is complete
       after the MapWindow has been created */
    return map != nullptr;
  }

  /**
   * Destroy the current Widget, but don't reactivate the map.  The
   * caller is responsible for reactivating the map or another Widget.
   */
  void KillWidget() noexcept;

  bool HaveTopWidget() const noexcept {
    return top_widget != nullptr;
  }

  /**
   * Destroy the current "top" Widget, but don't resize the main area.
   * The caller is responsible for doing that or installing a new top
   * Widget.
   */
  void KillTopWidget() noexcept;

  bool HaveBottomWidget() const noexcept {
    /* currently, the bottom widget is only visible below the map, but
       not below a custom main widget */
    /* TODO: eliminate this limitation; don't forget to remove the
       "widget==nullptr" check from MainWindow::KillBottomWidget() */
    return bottom_widget != nullptr && widget == nullptr;
  }

  /**
   * Destroy the current "bottom" Widget, but don't resize the main
   * area.  The caller is responsible for doing that or installing a
   * new bottom Widget.
   */
  void KillBottomWidget() noexcept;

public:
  void Create(PixelSize size, UI::TopWindowStyle style={});

  void Destroy() noexcept;

  void Initialise();
  void InitialiseConfigured();

  /**
   * Destroy the components of the main view (map, info boxes,
   * gauges).
   */
  void Deinitialise() noexcept;

private:
  [[gnu::pure]]
  const PixelRect &GetMainRect(const PixelRect &full_rc) const noexcept {
    return FullScreen ? full_rc : map_rect;
  }

  [[gnu::pure]]
  PixelRect GetMainRect() const noexcept {
    return FullScreen ? GetClientRect() : map_rect;
  }

  /**
   * Adjust the flarm radar position
   */
  void ReinitialiseLayout_flarm(PixelRect rc,
                                const InfoBoxLayout::Layout &ib_layout) noexcept;

  /**
   * Adjust vario
   */
  void ReinitialiseLayout_vario(const InfoBoxLayout::Layout &layout) noexcept;

  void ReinitialiseLayoutTA(PixelRect rc,
                            const InfoBoxLayout::Layout &layout) noexcept;

public:
  /**
   * Called by XCSoarInterface::Startup() after startup has been
   * completed.
   */
  void FinishStartup() noexcept;

  /**
   * Called by XCSoarInterface::Shutdown() before shutdown begins.
   */
  void BeginShutdown() noexcept;

  /**
   * Destroy and re-create all info boxes, and adjust the map
   * position/size.
   */
  void ReinitialiseLayout() noexcept;

  /**
   * Suspend threads that are owned by this object.
   */
  void SuspendThreads() noexcept;

  /**
   * Resumt threads that are owned by this object.
   */
  void ResumeThreads() noexcept;

  /**
   * Start loading the terrain file (asynchronously).
   */
  void LoadTerrain() noexcept;

  /**
   * Set the keyboard focus on the default element (i.e. the
   * MapWindow).
   */
  void SetDefaultFocus() noexcept;

  void FlushRendererCaches() noexcept;

  /**
   * Trigger a full redraw of the screen.
   */
  void FullRedraw() noexcept;

  bool GetFullScreen() const noexcept {
    return FullScreen;
  }

  void SetFullScreen(bool _full_screen) noexcept;

  void SendGPSUpdate() noexcept {
    gps_notify.SendNotification();
  }

  void SendCalculatedUpdate() noexcept {
    calculated_notify.SendNotification();
  }

  void SetTerrain(RasterTerrain *terrain) noexcept;
  void SetTopography(TopographyStore *topography) noexcept;

  const Look &GetLook() const noexcept {
    assert(look != nullptr);

    return *look;
  }

  Look &SetLook() noexcept {
    assert(look != nullptr);

    return *look;
  }

  void SetComputerSettings(const ComputerSettings &settings_computer) noexcept;
  void SetMapSettings(const MapSettings &settings_map) noexcept;
  void SetUIState(const UIState &ui_state) noexcept;

  /**
   * Returns the map even if it is not active.  May return nullptr if
   * there is no map.
   */
  [[gnu::pure]]
  GlueMapWindow *GetMap() noexcept {
    return map;
  }

  /**
   * Is the map active, i.e. currently visible?
   */
  bool IsMapActive() const noexcept {
    return widget == nullptr;
  }

  /**
   * Returns the map if it is active, or nullptr if the map is not
   * active.
   */
  [[gnu::pure]]
  GlueMapWindow *GetMapIfActive() noexcept;

  /**
   * Activate the map and return a pointer to it.  May return nullptr if
   * there is no map.
   */
  GlueMapWindow *ActivateMap() noexcept;

  /**
   * Schedule a call to PageActions::Restore().  The function returns
   * immediately, and there is no guarantee that it succeeds.
   */
  void DeferredRestorePage() noexcept;

  /**
   * Show this #Widget above the map.  This replaces (deletes) the
   * previous top widget, if any.  To disable this feature, call this
   * method with widget==nullptr.
   */
  void SetTopWidget(Widget *widget) noexcept;

  /**
   * Show this #Widget below the map.  This replaces (deletes) the
   * previous bottom widget, if any.  To disable this feature, call
   * this method with widget==nullptr.
   */
  void SetBottomWidget(Widget *widget) noexcept;

  /**
   * Replace the map with a #Widget.  The Widget instance gets deleted
   * when the map gets reactivated with ActivateMap() or if another
   * Widget gets set.
   */
  void SetWidget(Widget *_widget) noexcept;

  /**
   * Returns the current #Widget, but only if the specified flavour is
   * active.
   *
   * @see InputEvents::IsFlavour(), InputEvents::SetFlavour()
   */
  [[gnu::pure]]
  Widget *GetFlavourWidget(const TCHAR *flavour) noexcept;

  void UpdateGaugeVisibility() noexcept;

  [[gnu::pure]]
  const MapWindowProjection &GetProjection() const noexcept;

  void ToggleSuppressFLARMRadar() noexcept;
  void ToggleForceFLARMRadar() noexcept;

private:
  void UpdateVarioGaugeVisibility() noexcept;
  void UpdateTrafficGaugeVisibility() noexcept;

  void StopDragging() noexcept;

  void LateInitialise() noexcept;

  void RunTimer() noexcept;

  void OnGpsNotify() noexcept;
  void OnCalculatedNotify() noexcept;
  void OnRestorePageNotify() noexcept;

  void OnTerrainLoaded() noexcept;

protected:
  /* virtual methods from class Window */
  virtual void OnDestroy() override;
  virtual void OnResize(PixelSize new_size) override;
  virtual void OnSetFocus() override;
  virtual void OnCancelMode() override;
  bool OnMouseDown(PixelPoint p) override;
  bool OnMouseUp(PixelPoint p) override;
  bool OnMouseMove(PixelPoint p, unsigned keys) override;
  bool OnMouseDouble(PixelPoint p) override;
  virtual bool OnKeyDown(unsigned key_code) override;
  virtual void OnPaint(Canvas &canvas) override;

  /* virtual methods from class TopWindow */
  virtual bool OnClose() noexcept override;
};

#endif
