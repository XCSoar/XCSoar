// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "io/DataHandler.hpp"
#include <vector>
#include <cstddef>
#include <span>

class NativeInputListener : public DataHandler {
public:
	NativeInputListener();

	/**
	 * Verarbeitet eingehende Daten.
	 * @param s die empfangenen Daten als Byte-Span
	 * @return false, wenn keine weiteren Daten empfangen werden sollen
	 */
	bool DataReceived(std::span<const std::byte> s) noexcept override;


};
