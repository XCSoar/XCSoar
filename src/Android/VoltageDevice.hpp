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

#ifndef XCSOAR_ANDROID_VOLTAGE_DEVICE_HPP
#define XCSOAR_ANDROID_VOLTAGE_DEVICE_HPP

#include "VoltageListener.hpp"
#include "Java/Object.hxx"
#include "Compiler.h"
#include "Math/WindowFilter.hpp"

#include <jni.h>

#define NUMBER_OF_VOLTAGES 1

class VoltageDevice final : private VoltageListener {
  unsigned index;
  Java::GlobalObject obj;
  double offset;
  double factor;
  WindowFilter<16> voltage_filter[NUMBER_OF_VOLTAGES];
  WindowFilter<64> temperature_filter;

public:
  static void Initialise(JNIEnv *env);
  static void Deinitialise(JNIEnv *env);

  VoltageDevice(unsigned index,
               JNIEnv *env, jobject holder,
               double _offset, double _factor, unsigned sample_rate);

  ~VoltageDevice();

private:
  /* virtual methods from class VoltageListener */
  virtual void onVoltageValues(int temp_adc, int voltage_index, int volt_adc) override;
  virtual void onVoltageError() override;
};

#endif
