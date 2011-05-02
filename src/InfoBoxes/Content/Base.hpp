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

#ifndef XCSOAR_INFOBOX_CONTENT_HPP
#define XCSOAR_INFOBOX_CONTENT_HPP

#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Form/TabBar.hpp"

#include <tchar.h>
#include "Language/Language.hpp"
#include "fixed.hpp"

class InfoBoxWindow;
class Waypoint;
class Angle;

class InfoBoxContent
{
public:
  enum InfoBoxKeyCodes {
    ibkLeft = -2,
    ibkDown = -1,
    ibkEnter = 0,
    ibkUp = 1,
    ibkRight = 2
  };

  virtual void Update(InfoBoxWindow &infobox) = 0;
  virtual bool HandleKey(const InfoBoxKeyCodes keycode) {
    return false;
  }

  virtual void on_custom_paint(InfoBoxWindow &infobox, Canvas &canvas) {}

  /**
   * This is a generic handler for the InfoBox. It takes the argument and
   * processes it like the HandleKey handler, but is just more generic.
   * @param misc
   * @return True on success, Fales otherwise
   */
  virtual bool HandleQuickAccess(const TCHAR *misc) {
    return false;
  }

  struct PanelContent {
    PanelContent(const TCHAR* _name,
                        Window* (*_load)(SingleWindow&, TabBarControl*, WndForm*, int),
                        bool (*_preHide)(void) = NULL,
                        bool (*_preShow)(TabBarControl::EventType) = NULL,
                        void (*_postShow)(void) = NULL,
                        void (*_reClick)(void) = NULL ) :
                        name(_name),
                        load(_load),
                        preHide(_preHide),
                        preShow(_preShow),
                        postShow(_postShow),
                        reClick(_reClick) {};
    const TCHAR* name;
    Window* (*load)(SingleWindow&, TabBarControl*, WndForm*, int); // ptr to Load function
    bool (*preHide)(void); // ptr to PreHideFunction
    bool (*preShow)(TabBarControl::EventType); // ptr to PreShowFunction
    void (*postShow)(void); // ptr to PostShowFunction
    void (*reClick)(void); // ptr to ReClickFunction
  };

  struct DialogContent {
    const int PANELSIZE;
    PanelContent* Panels;
    CallBackTableEntry* CallBackTable;
  };

  virtual DialogContent* GetDialogContent() {
    return NULL;
  }

  static void SetTitleFromWaypointName(InfoBoxWindow &infobox,
                                       const Waypoint* waypoint);

  static void SetCommentFromWaypointName(InfoBoxWindow &infobox,
                                         const Waypoint* waypoint);

  static void SetValueFromFixed(InfoBoxWindow &infobox,
                                const TCHAR* format, fixed value);

  static void SetValueBearingDifference(InfoBoxWindow &infobox, Angle delta);
  static void SetValueFromDistance(InfoBoxWindow &infobox, fixed distance);
};

#endif
