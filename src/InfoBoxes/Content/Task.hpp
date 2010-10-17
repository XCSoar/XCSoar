/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef XCSOAR_INFOBOX_CONTENT_TASK_HPP
#define XCSOAR_INFOBOX_CONTENT_TASK_HPP

#include "InfoBoxes/Content/Base.hpp"

class InfoBoxContentBearing : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentBearingDiff : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentNextWaypoint : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
  virtual bool HandleKey(const InfoBoxKeyCodes keycode);
};

class InfoBoxContentNextDistance : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentNextETE : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentNextETA : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentNextAltitudeDiff : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentNextAltitudeRequire : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentNextLD: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentFinalDistance : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentFinalETE : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentFinalETA : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentFinalAltitudeDiff : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentFinalAltitudeRequire : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentFinalLD: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentFinalGR: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentHomeDistance: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentOLC: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
  virtual bool HandleKey(const InfoBoxKeyCodes keycode);
};

class InfoBoxContentTaskSpeed : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentTaskSpeedAchieved : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentTaskSpeedInstant : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentTaskAATime : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentTaskAATimeDelta : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentTaskAADistance : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentTaskAADistanceMax : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentTaskAADistanceMin : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentTaskAASpeed : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentTaskAASpeedMax : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentTaskAASpeedMin : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

#endif
