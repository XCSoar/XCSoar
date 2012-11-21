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

#ifndef XCSOAR_INFOBOX_CONTENT_TASK_HPP
#define XCSOAR_INFOBOX_CONTENT_TASK_HPP

#include "InfoBoxes/Content/Base.hpp"

class InfoBoxContentBearing : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentBearingDiff : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentNextWaypoint : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
  virtual bool HandleKey(const InfoBoxKeyCodes keycode) gcc_override;
};

class InfoBoxContentNextDistance : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentNextETE : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentNextETA : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentNextAltitudeDiff : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentNextMC0AltitudeDiff : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentNextAltitudeRequire : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentNextAltitudeArrival : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentNextGR: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentFinalDistance : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentFinalETE : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentFinalETA : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentFinalAltitudeDiff : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentFinalAltitudeRequire : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentFinalGRTE: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentFinalGR: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentHomeDistance: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentOLC: public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
  virtual bool HandleKey(const InfoBoxKeyCodes keycode) gcc_override;
};

class InfoBoxContentTaskSpeed : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentTaskSpeedAchieved : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentTaskSpeedInstant : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentTaskAATime : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentTaskAATimeDelta : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentTaskAADistance : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentTaskAADistanceMax : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentTaskAADistanceMin : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentTaskAASpeed : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentTaskAASpeedMax : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentTaskAASpeedMin : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentTaskTimeUnderMaxHeight : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentNextETEVMG : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentFinalETEVMG : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};

class InfoBoxContentCruiseEfficiency : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxData &data) gcc_override;
};
#endif
