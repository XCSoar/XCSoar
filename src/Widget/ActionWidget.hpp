/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_ACTION_WIDGET_HPP
#define XCSOAR_ACTION_WIDGET_HPP

#include "Screen/Features.hpp"

#ifdef HAVE_CLIPPING
#include "PanelWidget.hpp"
#else
#include "Widget.hpp"
#endif

class ActionListener;

/**
 * A #Widget implementation that triggers an action via
 * #ActionListener when clicked.
 */
class ActionWidget
#ifdef HAVE_CLIPPING
/* need PanelWidget on GDI so dialog background gets rendered in the
   Widget area just in case this Widget becomes "visible", to avoid
   uninitialised screen area */
  : public PanelWidget
#else
/* on OpenGL, we can avoid the overhead of creating a panel window */
  : public NullWidget
#endif
{
  ActionListener &listener;
  const int id;

public:
  ActionWidget(ActionListener &_listener, int _id)
    :listener(_listener), id(_id) {}

public:
  bool Click() override;
  void ReClick() override;

#ifndef HAVE_CLIPPING
  void Show(const PixelRect &rc) override;
  void Hide() override;
#endif
};

#endif
