// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Port/Listener.hpp"
#include "LogFile.hpp"

class NativePortListener : public PortListener {
public:
	NativePortListener();

	void PortStateChanged() noexcept override {
		LogFormat("MyNativePortListener: Port state changed");
	}

	void PortError(const char *msg) noexcept override {
		LogFormat("MyNativePortListener: Port error: %s", msg ? msg : "(null)");
	}
}; // namespace NativePortListener
