// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/XCOM760.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "RadioFrequency.hpp"
#include "util/StringFormat.hpp"

class XCOM760Device : public AbstractDevice {
private:
  Port &port;

public:
  XCOM760Device(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  bool PutVolume(unsigned volume, OperationEnvironment &env) override;
  bool PutActiveFrequency(RadioFrequency frequency,
                          const char *name,
                          OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const char *name,
                           OperationEnvironment &env) override;
  bool ExchangeRadioFrequencies(OperationEnvironment &env,
                                NMEAInfo &info) override;
};

bool
XCOM760Device::PutVolume(unsigned volume, OperationEnvironment &env)
{
  char szTmp[32];
  const int written = StringFormat(szTmp, sizeof(szTmp), "$RVOL=%u\r\n", volume);
  if (written < 0 || written >= (int)sizeof(szTmp))
    return false;

  port.FullWrite(szTmp, env, std::chrono::milliseconds{100});
  return true;
}

bool
XCOM760Device::PutActiveFrequency(RadioFrequency frequency,
                                  [[maybe_unused]] const char *name,
                                  OperationEnvironment &env)
{
  char szTmp[32];
  const int written = StringFormat(szTmp, sizeof(szTmp), "$TXAF=%u.%03u\r\n",
                                   frequency.GetKiloHertz() / 1000,
                                   frequency.GetKiloHertz() % 1000);
  if (written < 0 || written >= (int)sizeof(szTmp))
    return false;

  port.FullWrite(szTmp, env, std::chrono::milliseconds{100});
  return true;
}

bool
XCOM760Device::PutStandbyFrequency(RadioFrequency frequency,
                                   [[maybe_unused]] const char *name,
                                   OperationEnvironment &env)
{
  char szTmp[32];
  const int written = StringFormat(szTmp, sizeof(szTmp), "$TXSF=%u.%03u\r\n",
                                   frequency.GetKiloHertz() / 1000,
                                   frequency.GetKiloHertz() % 1000);
  if (written < 0 || written >= (int)sizeof(szTmp))
    return false;

  port.FullWrite(szTmp, env, std::chrono::milliseconds{100});
  return true;
}

bool
XCOM760Device::ExchangeRadioFrequencies(OperationEnvironment &env,
                                        [[maybe_unused]] NMEAInfo &info)
{
  const char szTmp[32] = "$TOGG\r\n";
  port.FullWrite(szTmp, env, std::chrono::milliseconds{100});
  return true;
}

static Device *
XCOM760CreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new XCOM760Device(com_port);
}

const struct DeviceRegister xcom760_driver = {
  "XCOM760",
  "XCOM760",
  DeviceRegister::NO_TIMEOUT | DeviceRegister::SEND_SETTINGS,
  XCOM760CreateOnPort,
};

/* Commands

  $TOGG: return to main screen or toggle active and standby
  $DUAL=on/off
*/
