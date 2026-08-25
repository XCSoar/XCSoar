// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

namespace ODBus {
class Connection;
}

namespace Systemd {

constexpr auto job_removed_match = "type='signal',"
	"sender='org.freedesktop.systemd1',"
	"interface='org.freedesktop.systemd1.Manager',"
	"member='JobRemoved',"
	"path='/org/freedesktop/systemd1'";

constexpr auto unit_removed_match = "type='signal',"
	"sender='org.freedesktop.systemd1',"
	"interface='org.freedesktop.systemd1.Manager',"
	"member='UnitRemoved',"
	"path='/org/freedesktop/systemd1'";

/**
 * Throws on error.
 *
 * @param timeout_ms maximum time to wait for completion, or -1 to wait
 * indefinitely
 */
void
WaitJobRemoved(ODBus::Connection &connection, const char *object_path,
	       int timeout_ms=-1);

/**
 * Wait for the UnitRemoved signal for the specified unit name.
 */
bool
WaitUnitRemoved(ODBus::Connection &connection, const char *name,
		int timeout_ms) noexcept;

enum class UnitFileState {
	ENABLED,
	ENABLED_RUNTIME,
	LINKED,
	LINKED_RUNTIME,
	MASKED,
	MASKED_RUNTIME,
	STATIC,
	DISABLED,
	INVALID,
};

UnitFileState
GetUnitFileState(ODBus::Connection &connection, const char *name);

bool
IsUnitEnabled(ODBus::Connection &connection, const char *name);

void
EnableUnitFile(ODBus::Connection &connection, const char *name,
	       bool runtime=false, bool force=true);

void
DisableUnitFile(ODBus::Connection &connection, const char *name,
		bool runtime=false);

enum class ActiveState {
	ACTIVE,
	RELOADING,
	INACTIVE,
	FAILED,
	ACTIVATING,
	DEACTIVATING,
};

ActiveState
GetUnitActiveState(ODBus::Connection &connection, const char *name);

bool
IsUnitActive(ODBus::Connection &connection, const char *name);

/**
 * Note: the caller must establish a match on "JobRemoved".
 *
 * Throws on error.
 */
void
StartUnit(ODBus::Connection &connection,
	  const char *name, const char *mode="replace", int timeout_ms=-1);

/**
 * Note: the caller must establish a match on "JobRemoved".
 *
 * Throws on error.
 */
void
StopUnit(ODBus::Connection &connection,
	 const char *name, const char *mode="replace", int timeout_ms=-1);

/**
 * Returns whether a unit file with the specified name is installed.
 *
 * This is intended for optional, explicitly configured units.  A D-Bus
 * failure is treated like a missing unit.
 */
bool
UnitExists(ODBus::Connection &connection, const char *name) noexcept;

/**
 * Restart a unit.
 *
 * Note: the caller must establish a match on "JobRemoved".
 *
 * Throws on error.
 */
void
RestartUnit(ODBus::Connection &connection,
	    const char *name, const char *mode="replace", int timeout_ms=-1);

/**
 * Resets the "failed" state of a specific unit.
 *
 * Throws on error.
 */
void
ResetFailedUnit(ODBus::Connection &connection, const char *name);

}
