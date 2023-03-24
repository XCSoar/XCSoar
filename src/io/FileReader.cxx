// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "FileReader.hxx"
#include "Open.hxx"
#include "system/Error.hxx"

#include <cassert>

#ifdef _WIN32

FileReader::FileReader(Path _path)
	:path(_path),
	 handle(CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
			   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
			   nullptr))
{
	if (handle == INVALID_HANDLE_VALUE)
		throw FormatLastError("Failed to open %s", path.ToUTF8().c_str());
}

std::size_t
FileReader::Read(void *data, std::size_t size)
{
	assert(IsDefined());

	DWORD nbytes;
	if (!ReadFile(handle, data, size, &nbytes, nullptr))
		throw FormatLastError("Failed to read from %s",
				      path.ToUTF8().c_str());

	return nbytes;
}

void
FileReader::Seek(off_t offset)
{
	assert(IsDefined());

	auto result = SetFilePointer(handle, offset, nullptr, FILE_BEGIN);
	if (result == INVALID_SET_FILE_POINTER)
		throw MakeLastError("Failed to seek");
}

void
FileReader::Skip(off_t offset)
{
	assert(IsDefined());

	auto result = SetFilePointer(handle, offset, nullptr, FILE_CURRENT);
	if (result == INVALID_SET_FILE_POINTER)
		throw MakeLastError("Failed to seek");
}

void
FileReader::Close() noexcept
{
	assert(IsDefined());

	CloseHandle(handle);
	handle = INVALID_HANDLE_VALUE;
}

#else

FileReader::FileReader(Path _path)
	:path(_path),
	 fd(OpenReadOnly(path.c_str()))
{
}

std::size_t
FileReader::Read(void *data, std::size_t size)
{
	assert(IsDefined());

	ssize_t nbytes = fd.Read(data, size);
	if (nbytes < 0)
		throw FormatErrno("Failed to read from %s", path.ToUTF8().c_str());

	return nbytes;
}

void
FileReader::Seek(off_t offset)
{
	assert(IsDefined());

	auto result = fd.Seek(offset);
	const bool success = result >= 0;
	if (!success)
		throw MakeErrno("Failed to seek");
}

void
FileReader::Skip(off_t offset)
{
	assert(IsDefined());

	auto result = fd.Skip(offset);
	const bool success = result >= 0;
	if (!success)
		throw MakeErrno("Failed to seek");
}

void
FileReader::Close() noexcept
{
	assert(IsDefined());

	fd.Close();
}

#endif
