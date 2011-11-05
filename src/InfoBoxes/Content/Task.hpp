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

#ifndef XCSOAR_INFOBOX_CONTENT_TASK_HPP
#define XCSOAR_INFOBOX_CONTENT_TASK_HPP

#include "InfoBoxes/Content/Base.hpp"

class InfoBoxContentBearing : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentBearingDiff : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentNextWaypoint : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
  virtual bool HandleKey(const InfoBoxKeyCodes keycode);
};

class InfoBoxContentNextDistance : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentNextETE : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentNextETA : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentNextAltitudeDiff : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentNextAltitudeRequire : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentNextAltitudeArrival : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentNextLD: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentFinalDistance : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentFinalETE : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentFinalETA : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentFinalAltitudeDiff : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentFinalAltitudeRequire : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentFinalLD: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentFinalGR: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentHomeDistance: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentOLC: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
  virtual bool HandleKey(const InfoBoxKeyCodes keycode);
};

class InfoBoxContentTaskSpeed : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentTaskSpeedAchieved : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentTaskSpeedInstant : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentTaskAATime : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentTaskAATimeDelta : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentTaskAADistance : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentTaskAADistanceMax : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentTaskAADistanceMin : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentTaskAASpeed : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentTaskAASpeedMax : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentTaskAASpeedMin : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentTaskTimeUnderMaxHeight : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentNextETEVMG : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

class InfoBoxContentFinalETEVMG : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data);
};

#endif
