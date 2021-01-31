/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_APPLE_INTERNAL_SENSORS_HPP
#define XCSOAR_APPLE_INTERNAL_SENSORS_HPP

#include "util/Compiler.h"

#import <CoreLocation/CoreLocation.h>

@interface LocationDelegate : NSObject <CLLocationManagerDelegate>
{
  @private unsigned int index;
  @private NSCalendar *gregorian_calendar;
}

-(instancetype) init __attribute__((unavailable()));

-(instancetype) init: (unsigned int) index_;
@end

// InternalSensors implementation which uses the Apple CoreLocation API
class InternalSensors {
private:
  unsigned int index;
  CLLocationManager *location_manager;
  LocationDelegate *location_delegate;

  InternalSensors(unsigned int index);

  void Init();
  void Deinit();

public:
  ~InternalSensors();
  gcc_malloc
  static InternalSensors *Create(unsigned int index);
};

#endif
