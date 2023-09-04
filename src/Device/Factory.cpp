// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Factory.hpp"
#include "Config.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Port/Port.hpp"

#ifdef ANDROID
#include "Android/BluetoothHelper.hpp"
#include "Android/GliderLink.hpp"
#include "Android/I2CbaroDevice.hpp"
#include "Android/InternalSensors.hpp"
#include "Android/IOIOHelper.hpp"
#include "Android/NunchuckDevice.hpp"
#include "Android/VoltageDevice.hpp"
#include "java/Closeable.hxx"
#endif

#ifdef __APPLE__
#include "Apple/InternalSensors.hpp"
#endif

std::unique_ptr<Port>
DeviceFactory::OpenPort(const DeviceConfig &config, PortListener *listener,
                        DataHandler &handler)
{
  return ::OpenPort(event_loop, cares,
#ifdef ANDROID
                    bluetooth_helper,
                    ioio_helper,
                    usb_serial_helper,
#endif
                    config, listener, handler);
}

#ifdef HAVE_INTERNAL_GPS

InternalSensors *
DeviceFactory::OpenInternalSensors(SensorListener &listener)
{
#ifdef ANDROID
  JNIEnv *const env = Java::GetEnv() ;
  auto *internal_sensors = InternalSensors::Create(env, &context,
                                                   permission_manager,
                                                   listener);
  if (internal_sensors) {
    // TODO: Allow user to specify whether they want certain sensors.
    internal_sensors->SubscribeToSensor(env, InternalSensors::TYPE_PRESSURE);
    internal_sensors->SubscribeToSensor(env, InternalSensors::TYPE_ACCELEROMETER);
  }

  return internal_sensors;
#elif defined(__APPLE__)
  return new InternalSensors(listener);
#endif
}

#endif // HAVE_INTERNAL_GPS

#ifdef ANDROID

std::pair<Java::LocalCloseable, Java::LocalCloseable>
DeviceFactory::OpenDroidSoarV2(SensorListener &listener)
{
  if (ioio_helper == nullptr)
    throw std::runtime_error{"IOIO not available"};

  return {
    I2CbaroDevice::Create(Java::GetEnv(),
                          ioio_helper->GetHolder(),
                          0,
                          2 + (0x77 << 8) + (27 << 16), 0, // bus, address
                          5, // update freq.
                          0, // flags
                          listener),
    I2CbaroDevice::Create(Java::GetEnv(),
                          ioio_helper->GetHolder(),
                          1,
                          1 + (0x77 << 8) + (46 << 16), 0 ,
                          5,
                          0,
                          listener),
  };
}

Java::LocalCloseable
DeviceFactory::OpenI2Cbaro(const DeviceConfig &config, SensorListener &listener)
{
  if (ioio_helper == nullptr)
    throw std::runtime_error{"IOIO not available"};

  return I2CbaroDevice::Create(Java::GetEnv(),
                               ioio_helper->GetHolder(),
                               0,
                               config.i2c_bus, config.i2c_addr,
                               config.press_use == DeviceConfig::PressureUse::TEK_PRESSURE ? 20 : 5,
                               0, // called flags, actually reserved for future use.
                               listener);
}

Java::LocalCloseable
DeviceFactory::OpenNunchuck(const DeviceConfig &config, SensorListener &listener)
{
  if (ioio_helper == nullptr)
    throw std::runtime_error{"IOIO not available"};

  return NunchuckDevice::Create(Java::GetEnv(),
                                ioio_helper->GetHolder(),
                                config.i2c_bus, 5, // twi, sample_rate
                                listener);
}

Java::LocalCloseable
DeviceFactory::OpenVoltage(SensorListener &listener)
{
  if (ioio_helper == nullptr)
    throw std::runtime_error{"IOIO not available"};

  return VoltageDevice::Create(Java::GetEnv(),
                               ioio_helper->GetHolder(),
                               60, // sample_rate per minute
                               listener);
}

Java::LocalCloseable
DeviceFactory::OpenGliderLink(SensorListener &listener)
{
  return GliderLink::Create(Java::GetEnv(), context, listener);
}

Java::LocalCloseable
DeviceFactory::OpenBluetoothSensor(const DeviceConfig &config,
                                   SensorListener &listener)
{
  if (bluetooth_helper == nullptr)
    throw std::runtime_error{"Bluetooth not available"};

  if (config.bluetooth_mac.empty())
    throw std::runtime_error{"No Bluetooth MAC configured"};

  return bluetooth_helper->connectSensor(Java::GetEnv(),
                                         config.bluetooth_mac,
                                         listener);
}

#endif // ANDROID
