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
#include "GlueMapWindow.hpp"
#include "PopupMessage.hpp"

class GlueGaugeVario;
class GaugeFLARM;
class GaugeThermalAssistant;
class StatusMessageList;

/**
 * The XCSoar main window.
 */
class MainWindow : public SingleWindow {
public:
  GlueMapWindow map;
  GlueGaugeVario *vario;
  GaugeFLARM *flarm;
  GaugeThermalAssistant *ta;
  PopupMessage popup;

private:
  timer_t timer_id;

  RECT map_rect;
  bool FullScreen;

  /**
   * True enables the use of a custom map rect.  Used by dlgTarget.
   */
  bool CustomView;

public:
  MainWindow(const StatusMessageList &status_messages)
    :vario(NULL), flarm(NULL), ta(NULL), popup(status_messages, *this),
     FullScreen(false), CustomView(false) {}
  virtual ~MainWindow();

  static bool find(const TCHAR *text) {
    return TopWindow::find(_T("XCSoarMain"), text);
  }

  static bool register_class(HINSTANCE hInstance);

protected:
  /**
   * Is XCSoar already up and running?
   */
  bool IsRunning() {
    /* it is safe enough to say that XCSoar initialization is complete
       after the MapWindow has been created */
    return map.defined();
  }

public:
  void set(const TCHAR *text,
           int left, int top, unsigned width, unsigned height);

  void Initialise();
  void InitialiseConfigured();

  /**
   * Destroy and re-create all info boxes, and adjust the map
   * position/size.
   */
  void ReinitialiseLayout();

  /**
   * Adjust the window position and size, to make it full-screen again
   * after display rotation.
   */
  void ReinitialisePosition();

  void reset();

  /**
   * Trigger a full redraw of the screen.
   */
  void full_redraw();

  bool GetFullScreen() const {
    return FullScreen;
  }

  void SetFullScreen(bool _full_screen);

  void SetCustomView(RECT rc);
  void LeaveCustomView();

protected:
  virtual bool on_resize(unsigned width, unsigned height);
  bool on_activate();
  bool on_setfocus();
  bool on_timer(timer_t id);
  bool on_create();
  bool on_destroy();
  bool on_close();
};

#endif
