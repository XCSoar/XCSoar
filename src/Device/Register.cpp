/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Device/Register.hpp"
#include "Device/Driver.hpp"
#include "Device/Driver/CAI302.hpp"
#include "Device/Driver/CaiGpsNav.hpp"
#include "Device/Driver/CaiLNav.hpp"
#include "Device/Driver/EW.hpp"
#include "Device/Driver/Eye.hpp"
#include "Device/Driver/AltairPro.hpp"
#include "Device/Driver/Generic.hpp"
#include "Device/Driver/Vega.hpp"
#include "Device/Driver/NmeaOut.hpp"
#include "Device/Driver/GTAltimeter.hpp"
#include "Device/Driver/PosiGraph.hpp"
#include "Device/Driver/BorgeltB50.hpp"
#include "Device/Driver/Volkslogger.hpp"
#include "Device/Driver/EWMicroRecorder.hpp"
#include "Device/Driver/LX.hpp"
#include "Device/Driver/IMI.hpp"
#include "Device/Driver/Zander.hpp"
#include "Device/Driver/FlymasterF1.hpp"
#include "Device/Driver/XCOM760.hpp"
#include "Device/Driver/Condor.hpp"
#include "Device/Driver/Leonardo.hpp"
#include "Device/Driver/Flytec.hpp"
#include "Device/Driver/ILEC.hpp"
#include "Device/Driver/Westerboer.hpp"
#include "Device/Driver/WesterboerVW921.hpp"
#include "Device/Driver/FLARM.hpp"
#include "Device/Driver/FlyNet.hpp"
#include "Device/Driver/CProbe.hpp"
#include "Device/Driver/LevilAHRS_G.hpp"
#include "Device/Driver/BlueFlyVario.hpp"
#include "Util/Macros.hpp"

#include <assert.h>
#include <string.h>

/** NULL terminated array of available device drivers. */
static const struct DeviceRegister *const driver_list[] = {
  // IMPORTANT: ADD NEW ONES TO BOTTOM OF THIS LIST
  &generic_driver, // MUST BE FIRST
  &cai302_driver,
  &ew_driver,
  &altair_pro_driver,
  &vega_driver,
  &gps_nav_driver,
  &nmea_out_driver,
  &posigraph_driver,
  &b50_driver,
  &volkslogger_driver,
  &ew_microrecorder_driver,
  &lx_driver,
  &zander_driver,
  &flymaster_f1_driver,
  &xcom760_driver,
  &condor_driver,
  &leonardo_driver,
  &flytec_driver,
  &ilec_driver,
  &westerboer_driver,
  &imi_driver,
  &flarm_driver,
  &westerboer_vw921_driver,
  &flynet_driver,
  &gt_altimeter_driver,
  &c_probe_driver,
  &levil_driver,
  &eye_driver,
  &bluefly_driver,
  &cai_lnav_driver,
  NULL
};

const struct DeviceRegister *
GetDriverByIndex(unsigned i)
{
  assert(i < ARRAY_SIZE(driver_list));

  return driver_list[i];
}

const struct DeviceRegister *
FindDriverByName(const TCHAR *name)
{
  for (auto i = driver_list; *i != NULL; ++i) {
    const DeviceRegister &driver = **i;
    if (_tcscmp(driver.name, name) == 0)
      return &driver;
  }

  return driver_list[0];
}

const TCHAR *
FindDriverDisplayName(const TCHAR *name)
{
  assert(name != NULL);

  for (auto i = driver_list; *i != NULL; ++i) {
    const DeviceRegister &driver = **i;
    if (_tcscmp(driver.name, name) == 0)
      return driver.display_name;
  }

  return name;
}
