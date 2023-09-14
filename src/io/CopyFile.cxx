// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "CopyFile.hxx"
#include "FileReader.hxx"
#include "FileOutputStream.hxx"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

static void
Copy(Reader &r, OutputStream &w)
{
	while (true) {
		std::byte buffer[65536];
		auto n = r.Read(buffer, sizeof(buffer));
		if (n == 0)
			break;

		w.Write(std::span{buffer}.first(n));
	}
}

void
CopyFile(Path src, Path dest)
{
	FileReader r(src);
	FileOutputStream w(dest);

	// TODO: use copy_file_range() if available (requires Linux 4.5)

	Copy(r, w);
	w.Commit();
}

void
MoveOrCopyFile(Path src, Path dest)
{
	if (File::Replace(src, dest))
		return;

	CopyFile(src, dest);
	File::Delete(src);
}
