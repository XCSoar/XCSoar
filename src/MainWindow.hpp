/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "PopupMessage.hpp"
#include "BatteryTimer.hpp"
#include "DisplayMode.hpp"

#include <assert.h>

struct SETTINGS_COMPUTER;
struct SETTINGS_MAP;
struct Look;
class GlueMapWindow;
class GlueGaugeVario;
class GaugeFLARM;
class GaugeThermalAssistant;
class StatusMessageList;
class RasterTerrain;
class TopographyStore;
class MapWindowProjection;

/**
 * The XCSoar main window.
 */
class MainWindow : public SingleWindow {
  enum cmd {
    /**
     * Check the airspace_warning_pending flag and show the airspace
     * warning dialog.
     */
    CMD_AIRSPACE_WARNING,

    /**
     * Called by the calculation thread when new calculation results
     * are available.  This updates the map and the info boxes.
     */
    CMD_CALCULATED_UPDATE,
  };

  Look *look;

  GlueMapWindow *map;

  GlueGaugeVario *vario;
  GaugeFLARM *flarm;
  GaugeThermalAssistant *ta;

public:
  PopupMessage popup;

private:
  timer_t timer_id;

  BatteryTimer battery_timer;

  PixelRect map_rect;
  bool FullScreen;

  bool airspace_warning_pending;

public:
  MainWindow(const StatusMessageList &status_messages);
  virtual ~MainWindow();

  static bool find(const TCHAR *text) {
    return TopWindow::find(_T("XCSoarMain"), text);
  }

#ifdef USE_GDI
  static bool register_class(HINSTANCE hInstance);
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

public:
  void set(const TCHAR *text,
           PixelScalar left, PixelScalar top,
           UPixelScalar width, UPixelScalar height);

  void Initialise();
  void InitialiseConfigured();

  /**
   * Destroy the components of the main view (map, info boxes,
   * gauges).
   */
  void Deinitialise();

  /**
   * Destroy and re-create all info boxes, and adjust the map
   * position/size.
   */
  void ReinitialiseLayout();

  /**
   * Adjust the flarm radar position
   */
  void ReinitialiseLayout_flarm(PixelRect rc, const InfoBoxLayout::Layout ib_layout);

  /**
   * Adjust vario
   */
  void ReinitialiseLayout_vario();

  /**
   * Adjust the window position and size, to make it full-screen again
   * after display rotation.
   */
  void ReinitialisePosition();

  void reset();

  /**
   * Set the keyboard focus on the default element (i.e. the
   * MapWindow).
   */
  void SetDefaultFocus();

  /**
   * Trigger a full redraw of the screen.
   */
  void full_redraw();

  bool GetFullScreen() const {
    return FullScreen;
  }

  void SetFullScreen(bool _full_screen);

  /**
   * A new airspace warning was found.  This method sends the
   * CMD_AIRSPACE_WARNING command to this window, which displays the
   * airspace warning dialog.
   */
  void SendAirspaceWarning() {
    airspace_warning_pending = true;
    send_user(CMD_AIRSPACE_WARNING);
  }

  void SendCalculatedUpdate() {
    send_user(CMD_CALCULATED_UPDATE);
  }

  void SetTerrain(RasterTerrain *terrain);
  void SetTopography(TopographyStore *topography);

  gcc_pure
  DisplayMode GetDisplayMode() const;

  const Look &GetLook() const {
    assert(look != NULL);

    return *look;
  }

  Look &SetLook() {
    assert(look != NULL);

    return *look;
  }

  void SetSettingsComputer(const SETTINGS_COMPUTER &settings_computer);
  void SetSettingsMap(const SETTINGS_MAP &settings_map);

  /**
   * Returns the map even if it is not active.  May return NULL if
   * there is no map.
   */
  gcc_pure
  GlueMapWindow *GetMap() {
    return map;
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

  void UpdateGaugeVisibility();

  void TriggerVarioUpdate();

  gcc_pure
  const MapWindowProjection &GetProjection() const;

  void ToggleSuppressFLARMRadar();
  void ToggleForceFLARMRadar();

protected:
  virtual bool on_resize(UPixelScalar width, UPixelScalar height);
  bool on_activate();
  bool on_setfocus();
  bool on_timer(timer_t id);
  virtual bool on_user(unsigned id);
  bool on_create();
  bool on_destroy();
  bool on_close();

#ifdef ANDROID
  virtual void on_pause();
#endif
};

#endif
