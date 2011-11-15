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

#include "Language/Language.hpp"
#include "fixed.hpp"

#include <tchar.h>

struct InfoBoxData;
class InfoBoxWindow;
struct Waypoint;
class Angle;
class Widget;
class Canvas;
struct CallBackTableEntry;

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

  virtual ~InfoBoxContent() {}

  virtual void Update(InfoBoxData &data) = 0;
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
    gcc_constexpr_ctor
    PanelContent(const TCHAR* _name,
                        Widget *(*_load)(unsigned id)) :
                        name(_name),
                        load(_load) {};
    const TCHAR* name;
    Widget *(*load)(unsigned id); // ptr to Load function
  };

  struct DialogContent {
    const int PANELSIZE;
    const PanelContent *Panels;
  };

  virtual const DialogContent *GetDialogContent() {
    return NULL;
  }

  static void SetTitleFromWaypointName(InfoBoxData &data,
                                       const Waypoint* waypoint);

  static void SetCommentFromWaypointName(InfoBoxData &data,
                                         const Waypoint* waypoint);

  static void SetValueFromFixed(InfoBoxData &data,
                                const TCHAR* format, fixed value);

  static void SetValueBearingDifference(InfoBoxData &data, Angle delta);
  static void SetCommentBearingDifference(InfoBoxData &data, Angle delta);
  static void SetValueFromDistance(InfoBoxData &data, fixed distance);
};

#endif
