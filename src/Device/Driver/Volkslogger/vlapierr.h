/***********************************************************************
**
**   vlapierr.h
**
**   This file is part of libkfrgcs.
**
************************************************************************
**
**   Copyright (c):  2002 by Garrecht Ingenieurgesellschaft
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#ifndef VLAPIERR_H
#define VLAPIERR_H

enum VLA_ERROR {
	VLA_ERR_NOERR,
	VLA_ERR_FILE,
	VLA_ERR_COMM,
	VLA_ERR_MEM,
	VLA_ERR_CRC,
	VLA_ERR_NOCHAR,
	VLA_ERR_NOANSWER,
	VLA_ERR_BADFLIGHTID,
	VLA_ERR_USERCANCELED,
	VLA_ERR_BADCOMMAND,
	VLA_ERR_NODATA,
	VLA_ERR_NOFLIGHTS,
	VLA_ERR_APINOTOPENED,
	VLA_ERR_TIMEOUT,
	VLA_ERR_MISC
};

#endif

