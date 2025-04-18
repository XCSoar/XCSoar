// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/XCOM760.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "RadioFrequency.hpp"

#include <stdio.h>

class XCOM760Device : public AbstractDevice {
private:
  Port &port;

public:
  XCOM760Device(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  bool PutVolume(unsigned volume, OperationEnvironment &env) override;
  bool PutActiveFrequency(RadioFrequency frequency,
                          const TCHAR *name,
                          OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const TCHAR *name,
                           OperationEnvironment &env) override;
};

bool
XCOM760Device::PutVolume(unsigned volume, OperationEnvironment &env)
{
  char szTmp[32];
  sprintf(szTmp, "$RVOL=%u\r\n", volume);
  port.FullWrite(szTmp, env, std::chrono::milliseconds{100});
  return true;
}

bool
XCOM760Device::PutActiveFrequency(RadioFrequency frequency,
                                  [[maybe_unused]] const TCHAR *name,
                                  OperationEnvironment &env)
{
  char szTmp[32];
  sprintf(szTmp, "$TXAF=%u.%03u\r\n",
          frequency.GetKiloHertz() / 1000,
          frequency.GetKiloHertz() % 1000);
  port.FullWrite(szTmp, env, std::chrono::milliseconds{100});
  return true;
}

bool
XCOM760Device::PutStandbyFrequency(RadioFrequency frequency,
                                   [[maybe_unused]] const TCHAR *name,
                                   OperationEnvironment &env)
{
  char szTmp[32];
  sprintf(szTmp, "$TXSF=%u.%03u\r\n",
          frequency.GetKiloHertz() / 1000,
          frequency.GetKiloHertz() % 1000);
  port.FullWrite(szTmp, env, std::chrono::milliseconds{100});
  return true;
}

static Device *
XCOM760CreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new XCOM760Device(com_port);
}

const struct DeviceRegister xcom760_driver = {
  _T("XCOM760"),
  _T("XCOM760"),
  DeviceRegister::NO_TIMEOUT | DeviceRegister::SEND_SETTINGS,
  XCOM760CreateOnPort,
};

/* Commands

  $TOGG: return to main screen or toggle active and standby
  $DUAL=on/off
*/
