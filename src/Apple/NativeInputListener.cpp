// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NativeInputListener.hpp"
#include "io/DataHandler.hpp"
#include "LogFile.hpp"

NativeInputListener::NativeInputListener() = default;

bool NativeInputListener::DataReceived(std::span<const std::byte> s) noexcept {


	LogFormat("[DEBUG] NativeInputListener::DataReceived");
	(void)s;
	return true;
}

