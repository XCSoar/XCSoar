// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/SingleWindow.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/event/Notify.hpp"
#include "BatteryTimer.hpp"
#include "Widget/ManagedWidget.hpp"
#include "UIUtil/GestureManager.hpp"

#include <cstdint>
#include <cassert>

#include "Menu/ShowButton.hpp"

struct ComputerSettings;
struct MapSettings;
struct UIState;
struct Look;
class Menu;
class MenuBar;
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

  MenuBar *menu_bar = nullptr;

  ShowMenuButton *show_menu_button = nullptr;
  ShowZoomOutButton *show_zoom_out_button = nullptr;
  ShowZoomInButton *show_zoom_in_button = nullptr;

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
  using SingleWindow::SingleWindow;
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
    /* currently, the top widget is only visible above the map, but
       not above a custom main widget */
    /* TODO: eliminate this limitation; don't forget to remove the
       "widget==nullptr" check from MainWindow::KillTopWidget() */
    return top_widget != nullptr && widget == nullptr;
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
   * Reinitialise the #Look after relevant #UISettings have been
   * changed.
   */
  void ReinitialiseLook() noexcept;

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

  void ShowMenu(const Menu &menu, const Menu *overlay=nullptr,
                bool full=true) noexcept;

  [[gnu::pure]]
  bool IsMenuButtonEnabled(unsigned idx) noexcept;

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
  void OnDestroy() noexcept override;
  void OnResize(PixelSize new_size) noexcept override;
  void OnSetFocus() noexcept override;
  void OnCancelMode() noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseDouble(PixelPoint p) noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
  PixelRect GetShowMenuButtonRect(const PixelRect rc) noexcept;
  PixelRect GetShowZoomOutButtonRect(const PixelRect rc) noexcept;
  PixelRect GetShowZoomInButtonRect(const PixelRect rc) noexcept;

  /* virtual methods from class TopWindow */
  bool OnClose() noexcept override;

#ifdef ANDROID
  void OnLook() noexcept override;
  void OnTaskReceived() noexcept override;
#endif
};
