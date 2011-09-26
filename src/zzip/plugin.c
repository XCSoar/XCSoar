
/*
 * Author: 
 *	Guido Draheim <guidod@gmx.de>
 *      Mike Nordell <tamlin-@-algonet-se>
 *
 * Copyright (c) 2002,2003 Guido Draheim
 * 	    All rights reserved,
 *	    use under the restrictions of the
 *	    Lesser GNU General Public License
 *          or alternatively the restrictions 
 *          of the Mozilla Public License 1.1
 */

#include <zzip/lib.h>
#include <zzip/plugin.h>

#include <string.h>
#include <sys/stat.h>
#ifdef ZZIP_DISABLED
#include <errno.h>
#endif /* ZZIP_DISABLED */
#include <stdlib.h>
#ifdef DEBUG
#include <stdio.h>
#endif

#include <zzip/file.h>
#include <zzip/format.h>

zzip_off_t
zzip_filesize(int fd)
{
    struct stat st;

    if (fstat(fd, &st) < 0)
        return -1;

# if defined DEBUG && ! defined _WIN32
    if (! st.st_size && st.st_blocks > 1)        /* seen on some darwin 10.1 machines */
        fprintf(stderr, "broken fstat(2) ?? st_size=%ld st_blocks=%ld\n",
                (long) st.st_size, (long) st.st_blocks);
# endif

    return st.st_size;
}

#ifdef _WIN32_WCE

#include <windows.h>
#include <fcntl.h>

static int
mingw_open (const char *path, int oflag, ...)
{
    wchar_t wpath[MAX_PATH];
    DWORD fileaccess;
    DWORD fileshare;
    DWORD filecreate;
    DWORD fileattrib;
    HANDLE hnd;

    size_t path_len = strlen (path);
    if (path_len >= MAX_PATH)
	return -1;

    switch (oflag & (O_RDONLY | O_WRONLY | O_RDWR))
    {
    case O_RDONLY:
	fileaccess = GENERIC_READ;
	break;
    case O_WRONLY:
	fileaccess = GENERIC_WRITE;
	break;
    case O_RDWR:
	fileaccess = GENERIC_READ | GENERIC_WRITE;
	break;
    default:
	return -1;
    }

    switch (oflag & (O_CREAT | O_EXCL | O_TRUNC))
    {
    case 0:
    case O_EXCL:               /* ignore EXCL w/o CREAT */
	filecreate = OPEN_EXISTING;
	break;
    case O_CREAT:
	filecreate = OPEN_ALWAYS;
	break;
    case O_CREAT | O_EXCL:
    case O_CREAT | O_TRUNC | O_EXCL:
	filecreate = CREATE_NEW;
	break;

    case O_TRUNC:
    case O_TRUNC | O_EXCL:     /* ignore EXCL w/o CREAT */
	filecreate = TRUNCATE_EXISTING;
	break;
    case O_CREAT | O_TRUNC:
	filecreate = CREATE_ALWAYS;
	break;
    default:
	/* this can't happen ... all cases are covered */
	return -1;
    }

    mbstowcs (wpath, path, path_len + 1);

    fileshare = FILE_SHARE_READ | FILE_SHARE_WRITE;
    fileattrib = FILE_ATTRIBUTE_NORMAL;

    hnd = CreateFileW (wpath, fileaccess, fileshare, NULL, filecreate,
		       fileattrib, NULL);
    if (hnd == INVALID_HANDLE_VALUE)
	return -1;

    if (oflag & O_APPEND)
	SetFilePointer (hnd, 0, NULL, FILE_END);

    return (int) hnd;
}

#endif

#ifdef WIN32

/* these wrappers are necessary because the WIN32 read()/write()
   functions get "int" instead of "size_t" - yuck! */

static zzip_ssize_t
default_io_read(int fd, void *buf, zzip_size_t len)
{
    return _zzip_read(fd, buf, len);
}

static zzip_ssize_t
default_io_write(int fd, const void *buf, zzip_size_t len)
{
    return _zzip_write(fd, buf, len);
}

#endif

static const struct zzip_plugin_io default_io = {
#ifdef _WIN32_WCE
    &mingw_open,
#else
    &open,
#endif
    &close,
#ifdef WIN32
    &default_io_read,
#else
    &_zzip_read,
#endif
    &_zzip_lseek,
    &zzip_filesize,
    1, 1,
#ifdef WIN32
    &default_io_write,
#else
    &_zzip_write
#endif
};

/** => zzip_init_io
 * This function returns a zzip_plugin_io_t handle to static defaults
 * wrapping the posix io file functions for actual file access.
 */
zzip_plugin_io_t
zzip_get_default_io(void)
{
    return (zzip_plugin_io_t) & default_io;
}

/**
 * This function initializes the users handler struct to default values 
 * being the posix io functions in default configured environments.
 */
int
zzip_init_io(zzip_plugin_io_handlers_t io, int flags)
{
    if (! io)
    {
        return ZZIP_ERROR;
    }
    memcpy(io, &default_io, sizeof(default_io));
    io->fd.sys = flags;
    return 0;
}

/* 
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
