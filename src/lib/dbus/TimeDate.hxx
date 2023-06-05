// SPDX-License-Identifier: BSD-2-Clause
// Copyright Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <chrono>

namespace ODBus {
class Connection;
}

/**
 * Wrappers for org.freedesktop.timedate1
 *
 * @see https://www.freedesktop.org/software/systemd/man/org.freedesktop.timedate1.html
 */
namespace TimeDate {

/**
 * Throws on error.
 */
bool
IsNTPSynchronized(ODBus::Connection &connection);

void
SetTime(ODBus::Connection &connection,
	std::chrono::system_clock::time_point t);

}
