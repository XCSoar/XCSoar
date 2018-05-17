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

#ifndef XCSOAR_PROXY_BLACKBOARD_LISTENER_HPP
#define XCSOAR_PROXY_BLACKBOARD_LISTENER_HPP

#include "BlackboardListener.hpp"

/**
 * A #BlackboardListener that forwards all calls to another listener.
 * Use this as a base class for specialised implementations.
 */
class ProxyBlackboardListener : public BlackboardListener {
protected:
  BlackboardListener &next;

public:
  ProxyBlackboardListener(BlackboardListener &_next):next(_next) {}

private:
  virtual void OnGPSUpdate(const MoreData &basic);

  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated);

  virtual void OnComputerSettingsUpdate(const ComputerSettings &settings);

  virtual void OnUISettingsUpdate(const UISettings &settings);
};

#endif
