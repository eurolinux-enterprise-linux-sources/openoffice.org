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
#ifndef _VCOMPAT_HXX
#define _VCOMPAT_HXX

#include "tools/toolsdllapi.h"
#include <tools/solar.h>

// -----------
// - Defines -
// -----------

#define COMPAT_FORMAT( char1, char2, char3, char4 ) \
	((UINT32)((((UINT32)(char)(char1)))|				\
	(((UINT32)(char)(char2))<<8UL)|					\
	(((UINT32)(char)(char3))<<16UL)|					\
	((UINT32)(char)(char4))<<24UL))

// --------------
// - ImplCompat -
// --------------

class SvStream;

class TOOLS_DLLPUBLIC VersionCompat
{
    SvStream*		mpRWStm;
    UINT32			mnCompatPos;
    UINT32			mnTotalSize;
    UINT16			mnStmMode;
    UINT16			mnVersion;

					VersionCompat() {}
					VersionCompat( const VersionCompat& ) {}
    VersionCompat&	operator=( const VersionCompat& ) { return *this; }
    BOOL			operator==( const VersionCompat& ) { return FALSE; }

public:

					VersionCompat( SvStream& rStm, USHORT nStreamMode, UINT16 nVersion = 1 );
					~VersionCompat();

    UINT16			GetVersion() const { return mnVersion; }
};

#endif // _VCOMPAT_HXX
