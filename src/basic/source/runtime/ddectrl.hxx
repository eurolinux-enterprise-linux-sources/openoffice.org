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

#ifndef _DDECTRL_HXX
#define _DDECTRL_HXX

#include <tools/link.hxx>
#ifndef _SBERRORS_HXX
#include <basic/sberrors.hxx>
#endif
#include <tools/string.hxx>

class DdeConnection;
class DdeConnections;
class DdeData;

class SbiDdeControl
{
private:
	DECL_LINK( Data, DdeData* );
	SbError GetLastErr( DdeConnection* );
	INT16 GetFreeChannel();
	DdeConnections*	pConvList;
	String aData;

public:

	SbiDdeControl();
	~SbiDdeControl();

	SbError Initiate( const String& rService, const String& rTopic,
					 INT16& rnHandle );
	SbError Terminate( INT16 nChannel );
	SbError TerminateAll();
	SbError Request( INT16 nChannel, const String& rItem, String& rResult );
	SbError Execute( INT16 nChannel, const String& rCommand );
	SbError Poke( INT16 nChannel, const String& rItem, const String& rData );
};

#endif
