/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Screen/SingleWindow.hpp"
#include "Screen/Timer.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "PopupMessage.hpp"
#include "BatteryTimer.hpp"
#include "Widget/ManagedWidget.hpp"
#include "UIUtil/GestureManager.hpp"

#include <stdint.h>
#include <assert.h>

struct ComputerSettings;
struct MapSettings;
struct UIState;
struct Look;
class GlueMapWindow;
class Widget;
class StatusMessageList;
class RasterTerrain;
class TopographyStore;
class MapWindowProjection;

/**
 * The XCSoar main window.
 */
class MainWindow : public SingleWindow {
  enum class Command: uint8_t {
    /**
     * Check the airspace_warning_pending flag and show the airspace
     * warning dialog.
     */
    AIRSPACE_WARNING,

    /**
     * Called by the #MergeThread when new GPS data is available.
     */
    GPS_UPDATE,

    /**
     * Called by the calculation thread when new calculation results
     * are available.  This updates the map and the info boxes.
     */
    CALCULATED_UPDATE,

    /**
     * @see DeferredRestorePage()
     */
    RESTORE_PAGE,

#ifdef ANDROID
    /**
     * A previous crash has been detected, and the crash log was
     * saved.
     */
    CRASH,
#endif
  };

  static constexpr const TCHAR *title = _T("XCSoar");

  Look *look;

  GlueMapWindow *map;

  /**
   * A #Widget that is shown below the map.
   */
  Widget *bottom_widget;

  /**
   * A #Widget that is shown instead of the map.  The #GlueMapWindow
   * is hidden and the DrawThread is suspended while this attribute is
   * non-NULL.
   */
  Widget *widget;

  ManagedWidget vario;

  ManagedWidget traffic_gauge;
  bool suppress_traffic_gauge, force_traffic_gauge;

  ManagedWidget thermal_assistant;

  bool dragging;
  GestureManager gestures;

public:
  PopupMessage popup;

private:
  WindowTimer timer;

  BatteryTimer battery_timer;

  PixelRect map_rect;
  bool FullScreen;

#ifndef ENABLE_OPENGL
  /**
   * This variable tracks whether the #DrawThread was suspended
   * because the map was replaced by a #Widget.
   */
  bool draw_suspended;
#endif

  bool restore_page_pending;

  bool airspace_warning_pending;

public:
  MainWindow(const StatusMessageList &status_messages);
  virtual ~MainWindow();

#ifdef USE_GDI
  static bool Find() {
    return SingleWindow::Find(title);
  }
#endif

protected:
  /**
   * Is XCSoar already up and running?
   */
  bool IsRunning() {
    /* it is safe enough to say that XCSoar initialization is complete
       after the MapWindow has been created */
    return map != NULL;
  }

  /**
   * Destroy the current Widget, but don't reactivate the map.  The
   * caller is responsible for reactivating the map or another Widget.
   */
  void KillWidget();

  /**
   * Destroy the current "bottom" Widget, but don't resize the main
   * area.  The caller is responsible for doing that or installing a
   * new bottom Widget.
   */
  void KillBottomWidget();

public:
  void Create(PixelSize size, TopWindowStyle style=TopWindowStyle());

  void Destroy();

  void Initialise();
  void InitialiseConfigured();

  /**
   * Destroy the components of the main view (map, info boxes,
   * gauges).
   */
  void Deinitialise();

private:
  gcc_pure
  const PixelRect &GetMainRect(const PixelRect &full_rc) const {
    return FullScreen ? full_rc : map_rect;
  }

  gcc_pure
  PixelRect GetMainRect() const {
    return FullScreen ? GetClientRect() : map_rect;
  }

  /**
   * Adjust the flarm radar position
   */
  void ReinitialiseLayout_flarm(PixelRect rc, const InfoBoxLayout::Layout ib_layout);

  /**
   * Adjust vario
   */
  void ReinitialiseLayout_vario(const InfoBoxLayout::Layout &layout);

  void ReinitialiseLayoutTA(PixelRect rc, const InfoBoxLayout::Layout &layout);

public:
  /**
   * Called by XCSoarInterface::Startup() after startup has been
   * completed.
   */
  void FinishStartup();

  /**
   * Called by XCSoarInterface::Shutdown() before shutdown begins.
   */
  void BeginShutdown();

  /**
   * Destroy and re-create all info boxes, and adjust the map
   * position/size.
   */
  void ReinitialiseLayout();

  /**
   * Suspend threads that are owned by this object.
   */
  void SuspendThreads();

  /**
   * Resumt threads that are owned by this object.
   */
  void ResumeThreads();

  /**
   * Set the keyboard focus on the default element (i.e. the
   * MapWindow).
   */
  void SetDefaultFocus();

  /**
   * Trigger a full redraw of the screen.
   */
  void FullRedraw();

  bool GetFullScreen() const {
    return FullScreen;
  }

  void SetFullScreen(bool _full_screen);

  /**
   * A new airspace warning was found.  This method sends the
   * Command::AIRSPACE_WARNING command to this window, which displays the
   * airspace warning dialog.
   */
  void SendAirspaceWarning() {
    airspace_warning_pending = true;
    SendUser((unsigned)Command::AIRSPACE_WARNING);
  }

  void SendGPSUpdate() {
    SendUser((unsigned)Command::GPS_UPDATE);
  }

  void SendCalculatedUpdate() {
    SendUser((unsigned)Command::CALCULATED_UPDATE);
  }

#ifdef ANDROID
  void SendCrash() {
    SendUser((unsigned)Command::CRASH);
  }
#endif

  void SetTerrain(RasterTerrain *terrain);
  void SetTopography(TopographyStore *topography);

  const Look &GetLook() const {
    assert(look != NULL);

    return *look;
  }

  Look &SetLook() {
    assert(look != NULL);

    return *look;
  }

  void SetComputerSettings(const ComputerSettings &settings_computer);
  void SetMapSettings(const MapSettings &settings_map);
  void SetUIState(const UIState &ui_state);

  /**
   * Returns the map even if it is not active.  May return NULL if
   * there is no map.
   */
  gcc_pure
  GlueMapWindow *GetMap() {
    return map;
  }

  /**
   * Is the map active, i.e. currently visible?
   */
  bool IsMapActive() const {
    return widget == NULL;
  }

  /**
   * Returns the map if it is active, or NULL if the map is not
   * active.
   */
  gcc_pure
  GlueMapWindow *GetMapIfActive();

  /**
   * Activate the map and return a pointer to it.  May return NULL if
   * there is no map.
   */
  GlueMapWindow *ActivateMap();

  /**
   * Schedule a call to PageActions::Restore().  The function returns
   * immediately, and there is no guarantee that it succeeds.
   */
  void DeferredRestorePage();

  /**
   * Show this #Widget below the map.  This replaces (deletes) the
   * previous bottom widget, if any.  To disable this feature, call
   * this method with widget==nullptr.
   */
  void SetBottomWidget(Widget *widget);

  /**
   * Replace the map with a #Widget.  The Widget instance gets deleted
   * when the map gets reactivated with ActivateMap() or if another
   * Widget gets set.
   */
  void SetWidget(Widget *_widget);

  /**
   * Returns the current #Widget, but only if the specified flavour is
   * active.
   *
   * @see InputEvents::IsFlavour(), InputEvents::SetFlavour()
   */
  gcc_pure
  Widget *GetFlavourWidget(const TCHAR *flavour);

  void UpdateGaugeVisibility();

  gcc_pure
  const MapWindowProjection &GetProjection() const;

  void ToggleSuppressFLARMRadar();
  void ToggleForceFLARMRadar();

private:
  void UpdateVarioGaugeVisibility();
  void UpdateTrafficGaugeVisibility();

  void StopDragging();

protected:
  /* virtual methods from class Window */
  virtual void OnDestroy() override;
  virtual void OnResize(PixelSize new_size) override;
  virtual void OnSetFocus() override;
  virtual void OnCancelMode() override;
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseMove(PixelScalar x, PixelScalar y,
                           unsigned keys) override;
  virtual bool OnMouseDouble(PixelScalar x, PixelScalar y) override;
  virtual bool OnKeyDown(unsigned key_code) override;
  virtual bool OnUser(unsigned id) override;
  virtual bool OnTimer(WindowTimer &timer) override;

  /* virtual methods from class TopWindow */
  virtual bool OnClose() override;
  virtual bool OnActivate() override;

#ifdef ANDROID
  virtual void OnPause() override;
#endif
};

#endif
