// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Client.hpp"
#include "Thermal.hpp"

class Serialiser;
class Deserialiser;

struct CloudData {
  CloudClientContainer clients;
  CloudThermalContainer thermals;

  void DumpClients();

  void Save(Serialiser &s) const;
  void Load(Deserialiser &s);
};
