// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <memory>

class Port;
class PortListener;
class DataHandler;

/**
 * Open a Condor Spectate.json polling port.
 *
 * The real implementation lives with SpectateFilePort; tests that link
 * ConfiguredPort without DRIVER may provide a stub instead.
 */
std::unique_ptr<Port>
OpenSpectateFilePort(Path path, const char *own_cn,
                     PortListener *listener,
                     DataHandler &handler);
