/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Compiler.h"

#include <tchar.h>

struct PixelRect;
struct InfoBoxData;
class Widget;
class Canvas;

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

  virtual ~InfoBoxContent();

  virtual void Update(InfoBoxData &data) = 0;
  virtual bool HandleKey(const InfoBoxKeyCodes keycode);

  virtual void OnCustomPaint(Canvas &canvas, const PixelRect &rc);

  struct PanelContent {
    constexpr
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

  gcc_pure
  virtual const DialogContent *GetDialogContent();
};

#endif
