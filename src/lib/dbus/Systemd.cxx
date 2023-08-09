// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Systemd.hxx"
#include "Connection.hxx"
#include "Message.hxx"
#include "AppendIter.hxx"
#include "PendingCall.hxx"
#include "ReadIter.hxx"
#include "Error.hxx"
#include "util/StringAPI.hxx"

#include <string>

namespace Systemd {

void
WaitJobRemoved(ODBus::Connection &connection, const char *object_path)
{
	using namespace ODBus;

	while (true) {
		auto msg = Message::Pop(*connection);
		if (!msg.IsDefined()) {
			if (dbus_connection_read_write(connection, -1))
				continue;
			else
				break;
		}

		if (msg.IsSignal("org.freedesktop.systemd1.Manager", "JobRemoved")) {
			Error error;
			dbus_uint32_t job_id;
			const char *removed_object_path, *unit_name, *result_string;
			if (!msg.GetArgs(error,
					 DBUS_TYPE_UINT32, &job_id,
					 DBUS_TYPE_OBJECT_PATH, &removed_object_path,
					 DBUS_TYPE_STRING, &unit_name,
					 DBUS_TYPE_STRING, &result_string))
				error.Throw("JobRemoved failed");

			if (StringIsEqual(removed_object_path, object_path))
				break;
		}
	}
}

bool
WaitUnitRemoved(ODBus::Connection &connection, const char *name,
		int timeout_ms) noexcept
{
	using namespace ODBus;

	bool was_empty = false;

	while (true) {
		auto msg = Message::Pop(*connection);
		if (!msg.IsDefined()) {
			if (was_empty)
				return false;

			was_empty = true;

			if (dbus_connection_read_write(connection, timeout_ms))
				continue;
			else
				return false;
		}

		if (msg.IsSignal("org.freedesktop.systemd1.Manager", "UnitRemoved")) {
			DBusError err;
			dbus_error_init(&err);

			const char *unit_name, *object_path;
			if (!msg.GetArgs(err,
					 DBUS_TYPE_STRING, &unit_name,
					 DBUS_TYPE_OBJECT_PATH, &object_path)) {
				dbus_error_free(&err);
				return false;
			}

			if (StringIsEqual(unit_name, name))
				return true;
		}
	}
}

static UnitFileState
ParseUnitFileState(const char *s)
{
	if (StringIsEqual(s, "enabled"))
		return UnitFileState::ENABLED;
	else if (StringIsEqual(s, "enabled-runtime"))
		return UnitFileState::ENABLED_RUNTIME;
	else if (StringIsEqual(s, "linked"))
		return UnitFileState::LINKED;
	else if (StringIsEqual(s, "linked-runtime"))
		return UnitFileState::LINKED_RUNTIME;
	else if (StringIsEqual(s, "masked"))
		return UnitFileState::MASKED;
	else if (StringIsEqual(s, "masked-runtime"))
		return UnitFileState::MASKED_RUNTIME;
	else if (StringIsEqual(s, "static"))
		return UnitFileState::STATIC;
	else if (StringIsEqual(s, "disabled"))
		return UnitFileState::DISABLED;
	else
		throw std::invalid_argument{"Unrecognized UnitFileState"};
}

UnitFileState
GetUnitFileState(ODBus::Connection &connection, const char *name)
{
	using namespace ODBus;

	auto msg = Message::NewMethodCall("org.freedesktop.systemd1",
					  "/org/freedesktop/systemd1",
					  "org.freedesktop.systemd1.Manager",
					  "GetUnitFileState");

	AppendMessageIter{*msg.Get()}.Append(name);

	auto pending = PendingCall::SendWithReply(connection, msg.Get());

	dbus_connection_flush(connection);

	pending.Block();

	Message reply = Message::StealReply(*pending.Get());
	reply.CheckThrowError();

	Error error;
	const char *state;
	if (!reply.GetArgs(error, DBUS_TYPE_STRING, &state))
		error.Throw("StartUnit reply failed");

	return ParseUnitFileState(state);
}

bool
IsUnitEnabled(ODBus::Connection &connection, const char *name)
{
	switch (GetUnitFileState(connection, name)) {
	case UnitFileState::ENABLED:
	case UnitFileState::ENABLED_RUNTIME:
		return true;

	default:
		return false;
	}
}

void
EnableUnitFile(ODBus::Connection &connection, const char *name,
	       bool runtime, bool force)
{
	using namespace ODBus;

	auto msg = Message::NewMethodCall("org.freedesktop.systemd1",
					  "/org/freedesktop/systemd1",
					  "org.freedesktop.systemd1.Manager",
					  "EnableUnitFiles");

	AppendMessageIter args{*msg.Get()};

	AppendMessageIter{args, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING}
		.Append(name)
		.CloseContainer(args);
	args.Append(Boolean{runtime}).Append(Boolean{force});

	auto pending = PendingCall::SendWithReply(connection, msg.Get());

	dbus_connection_flush(connection);

	pending.Block();

	Message reply = Message::StealReply(*pending.Get());
	reply.CheckThrowError();
}

void
DisableUnitFile(ODBus::Connection &connection, const char *name,
		bool runtime)
{
	using namespace ODBus;

	auto msg = Message::NewMethodCall("org.freedesktop.systemd1",
					  "/org/freedesktop/systemd1",
					  "org.freedesktop.systemd1.Manager",
					  "DisableUnitFiles");

	AppendMessageIter args{*msg.Get()};

	AppendMessageIter{args, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING}
		.Append(name)
		.CloseContainer(args);
	args.Append(Boolean{runtime});

	auto pending = PendingCall::SendWithReply(connection, msg.Get());

	dbus_connection_flush(connection);

	pending.Block();

	Message reply = Message::StealReply(*pending.Get());
	reply.CheckThrowError();
}

static ActiveState
ParseActiveState(const char *s)
{
	if (StringIsEqual(s, "active"))
		return ActiveState::ACTIVE;
	else if (StringIsEqual(s, "reloading"))
		return ActiveState::RELOADING;
	else if (StringIsEqual(s, "inactive"))
		return ActiveState::INACTIVE;
	else if (StringIsEqual(s, "failed"))
		return ActiveState::FAILED;
	else if (StringIsEqual(s, "activating"))
		return ActiveState::ACTIVATING;
	else if (StringIsEqual(s, "deactivating"))
		return ActiveState::DEACTIVATING;
	else
		throw std::invalid_argument{"Unrecognized ActiveState"};
}

static std::string
GetUnit(ODBus::Connection &connection, const char *name)
{
	using namespace ODBus;

	auto msg = Message::NewMethodCall("org.freedesktop.systemd1",
					  "/org/freedesktop/systemd1",
					  "org.freedesktop.systemd1.Manager",
					  "GetUnit");

	AppendMessageIter{*msg.Get()}.Append(name);

	auto pending = PendingCall::SendWithReply(connection, msg.Get());

	dbus_connection_flush(connection);

	pending.Block();

	Message reply = Message::StealReply(*pending.Get());
	if (reply.IsError("org.freedesktop.systemd1.NoSuchUnit"))
		return {};

	reply.CheckThrowError();

	Error error;
	const char *path;
	if (!reply.GetArgs(error, DBUS_TYPE_OBJECT_PATH, &path))
		error.Throw("GetUnit reply failed");

	return path;
}

ActiveState
GetUnitActiveState(ODBus::Connection &connection, const char *name)
{
	using namespace ODBus;

	const auto path = GetUnit(connection, name);
	if (path.empty())
		return ActiveState::INACTIVE;

	auto msg = Message::NewMethodCall("org.freedesktop.systemd1",
					  path.c_str(),
					  "org.freedesktop.DBus.Properties",
					  "Get");

	AppendMessageIter{*msg.Get()}
		.Append("org.freedesktop.systemd1.Unit")
		.Append("ActiveState");

	auto pending = PendingCall::SendWithReply(connection, msg.Get());

	dbus_connection_flush(connection);

	pending.Block();

	Message reply = Message::StealReply(*pending.Get());
	reply.CheckThrowError();

	ReadMessageIter iter = ReadMessageIter{*reply.Get()}.Recurse();
	return ParseActiveState(iter.GetString());
}

bool
IsUnitActive(ODBus::Connection &connection, const char *name)
{
	switch (GetUnitActiveState(connection, name)) {
	case ActiveState::ACTIVE:
		return true;

	default:
		return false;
	}
}

void
StartUnit(ODBus::Connection &connection,
	     const char *name, const char *mode)
{
	using namespace ODBus;

	auto msg = Message::NewMethodCall("org.freedesktop.systemd1",
					  "/org/freedesktop/systemd1",
					  "org.freedesktop.systemd1.Manager",
					  "StartUnit");

	AppendMessageIter{*msg.Get()}.Append(name).Append(mode);

	auto pending = PendingCall::SendWithReply(connection, msg.Get());

	dbus_connection_flush(connection);

	pending.Block();

	Message reply = Message::StealReply(*pending.Get());
	reply.CheckThrowError();

	Error error;
	const char *object_path;
	if (!reply.GetArgs(error, DBUS_TYPE_OBJECT_PATH, &object_path))
		error.Throw("StartUnit reply failed");

	WaitJobRemoved(connection, object_path);
}

void
StopUnit(ODBus::Connection &connection,
	    const char *name, const char *mode)
{
	using namespace ODBus;

	auto msg = Message::NewMethodCall("org.freedesktop.systemd1",
					  "/org/freedesktop/systemd1",
					  "org.freedesktop.systemd1.Manager",
					  "StopUnit");

	AppendMessageIter{*msg.Get()}.Append(name).Append(mode);

	auto pending = PendingCall::SendWithReply(connection, msg.Get());

	dbus_connection_flush(connection);

	pending.Block();

	Message reply = Message::StealReply(*pending.Get());
	reply.CheckThrowError();

	Error error;
	const char *object_path;
	if (!reply.GetArgs(error, DBUS_TYPE_OBJECT_PATH, &object_path))
		error.Throw("StopUnit reply failed");

	WaitJobRemoved(connection, object_path);
}

void
ResetFailedUnit(ODBus::Connection &connection, const char *name)
{
	using namespace ODBus;

	auto msg = Message::NewMethodCall("org.freedesktop.systemd1",
					  "/org/freedesktop/systemd1",
					  "org.freedesktop.systemd1.Manager",
					  "ResetFailedUnit");

	AppendMessageIter{*msg.Get()}.Append(name);

	auto pending = PendingCall::SendWithReply(connection, msg.Get());

	dbus_connection_flush(connection);

	pending.Block();

	Message reply = Message::StealReply(*pending.Get());
	reply.CheckThrowError();
}

} // namespace Systemd
