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

#ifndef XCSOAR_BLACKBOARD_LISTENER_HPP
#define XCSOAR_BLACKBOARD_LISTENER_HPP

struct MoreData;
struct DerivedInfo;
struct ComputerSettings;
struct UISettings;

/**
 * This class can be registered to receive GPS updates and other
 * changes.
 */
class BlackboardListener {
public:
  /**
   * New GPS data has been received, but calculations results have not
   * been updated yet.
   */
  virtual void OnGPSUpdate(const MoreData &basic) = 0;

  /**
   * New GPS data has been received and calculations results have been
   * updated.
   */
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated) = 0;

  /**
   * The user has modified the computer settings.
   */
  virtual void OnComputerSettingsUpdate(const ComputerSettings &settings) = 0;

  /**
   * The user has modified the UI settings.
   */
  virtual void OnUISettingsUpdate(const UISettings &settings) = 0;
};

/**
 * A dummy class that implements all abstract methods as no-ops.
 * Inherit this class and only implement the methods you're interested
 * in.
 */
class NullBlackboardListener : public BlackboardListener {
public:
  void OnGPSUpdate(const MoreData &basic) override;

  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;

  void OnComputerSettingsUpdate(const ComputerSettings &settings) override;

  void OnUISettingsUpdate(const UISettings &settings) override;
};

#endif
