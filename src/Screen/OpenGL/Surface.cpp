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

#include "Screen/OpenGL/Surface.hpp"

#include <list>
#include <cassert>

typedef std::list<GLSurfaceListener *> GLSurfaceListenerList;

static GLSurfaceListenerList surface_listeners;

bool surface_valid = true;

void
AddSurfaceListener(GLSurfaceListener &listener)
{
  surface_listeners.push_back(&listener);
}

void
RemoveSurfaceListener(GLSurfaceListener &listener)
{
  surface_listeners.remove(&listener);
}

void
SurfaceCreated()
{
  if (surface_valid)
    return;

  surface_valid = true;

  const GLSurfaceListenerList copy(surface_listeners);
  for (GLSurfaceListener *listener : copy)
    listener->SurfaceCreated();
}

void
SurfaceDestroyed()
{
  const GLSurfaceListenerList copy(surface_listeners);
  for (GLSurfaceListener *listener : copy)
    listener->SurfaceDestroyed();

  surface_valid = false;
}
