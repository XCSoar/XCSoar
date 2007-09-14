/*  wcecompat: Windows CE C Runtime Library "compatibility" library.
 *
 *  Copyright (C) 2001-2002 Essemer Pty Ltd.  All rights reserved.
 *  http://www.essemer.com.au/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef __wcecompat__ChildData_h__
#define __wcecompat__ChildData_h__


#include "args.h"


class ChildData
{
	typedef struct
	{
		TCHAR*	name;
		TCHAR*	value;
	} NameValuePair;

private:

	TCHAR*			currentDirectory;
	RedirArg*		redirArgs;
	int				numRedirArgs;
	NameValuePair*	environment;
	int				environmentLength;	// num of environment variables

public:

	ChildData();
	~ChildData();

	void dump();

	const TCHAR* getCurrentDirectory() const;
	void setCurrentDirectory(const TCHAR* directory);

	void resetCurrentDirectory();

	void resetRedirArgs();
	RedirArg* getRedirArg(int fd);
	bool setRedirArg(RedirArg* redirArg);
	bool setRedirArgs(RedirArg* redirArgs, int numRedirArgs);

	void resetEnvironment();
	bool addEnvironmentVar(WCHAR* env);
	bool addEnvironmentList(WCHAR* env);
	bool saveEnvironment();
	bool restoreEnvironment();

	// if childData is NULL, the size will be returned in childDataLen
	bool encode(void* childData, int* childDataSize);

	// if childData is NULL, the size will be returned in childDataLen
	bool decode(void* childData);//, int childDataSize);
};


#endif /* __wcecompat__ChildData_h__ */
