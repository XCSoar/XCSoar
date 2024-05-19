// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Register.hpp"
#include "Device/Driver.hpp"
#include "Device/Driver/AirControlDisplay.hpp"
#include "Device/Driver/CAI302.hpp"
#include "Device/Driver/CaiGpsNav.hpp"
#include "Device/Driver/CaiLNav.hpp"
#include "Device/Driver/EW.hpp"
#include "Device/Driver/Eye.hpp"
#include "Device/Driver/AltairPro.hpp"
#include "Device/Driver/Generic.hpp"
#include "Device/Driver/Vega.hpp"
#include "Device/Driver/NmeaOut.hpp"
#include "Device/Driver/PosiGraph.hpp"
#include "Device/Driver/BorgeltB50.hpp"
#include "Device/Driver/XCVario.hpp"
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
#include "Device/Driver/FLARM.hpp"
#include "Device/Driver/FlyNet.hpp"
#include "Device/Driver/ThermalExpress.hpp"
#include "Device/Driver/CProbe.hpp"
#include "Device/Driver/LevilAHRS_G.hpp"
#include "Device/Driver/BlueFlyVario.hpp"
#include "Device/Driver/OpenVario.hpp"
#include "Device/Driver/Larus.hpp"
#include "Device/Driver/Vaulter.hpp"
#include "Device/Driver/ATR833/Register.hpp"
#include "Device/Driver/XCTracer.hpp"
#include "Device/Driver/KRT2.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"

#include <cassert>

/** nullptr terminated array of available device drivers. */
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
  &xcv_driver,
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
  &flynet_driver,
  &c_probe_driver,
  &levil_driver,
  &eye_driver,
  &bluefly_driver,
  &cai_lnav_driver,
  &open_vario_driver,
  &larus_driver,
  &vaulter_driver,
  &krt2_driver,
  &atr833_driver,
  &xctracer_driver,
  &thermalexpress_driver,
  &acd_driver,
  nullptr
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
  for (auto i = driver_list; *i != nullptr; ++i) {
    const DeviceRegister &driver = **i;
    if (StringIsEqual(driver.name, name))
      return &driver;
  }

  return driver_list[0];
}

const TCHAR *
FindDriverDisplayName(const TCHAR *name)
{
  assert(name != nullptr);

  for (auto i = driver_list; *i != nullptr; ++i) {
    const DeviceRegister &driver = **i;
    if (StringIsEqual(driver.name, name))
      return driver.display_name;
  }

  return name;
}
