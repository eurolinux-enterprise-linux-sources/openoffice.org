/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2000, 2010 Oracle and/or its affiliates.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 ************************************************************************/



#include <_version.h>


struct VersionInfo
{
	const char*	pTime;
	const char*	pDate;
	const char*	pUpd;
	const char*	pMinor;
	const char*	pBuild;
	const char*	pInpath;
};

static const struct VersionInfo g_aVersionInfo =
{
	__TIME__,
	__DATE__,
	_UPD,
	_LAST_MINOR,
	_BUILD,
	_INPATH
};

#if defined(WNT) || defined(OS2)
__declspec(dllexport) const struct VersionInfo* GetVersionInfo(void);
#endif

#if defined(WNT) || defined(OS2)
__declspec(dllexport) const struct VersionInfo* GetVersionInfo(void)
#else
const struct VersionInfo *GetVersionInfo(void)
#endif
{
	return &g_aVersionInfo;
}

#if 0
#include <stdio.h>

int main( int argc, char **argv )
{
	const VersionInfo *pInfo = GetVersionInfo();
	fprintf( stderr, "Date : %s\n", pInfo->pDate);
	fprintf( stderr, "Time : %s\n", pInfo->pTime);
	fprintf( stderr, "UPD : %s\n", pInfo->pUpd);
	delete pInfo;
	return 0;
}
#endif

