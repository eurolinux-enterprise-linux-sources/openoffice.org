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

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_svtools.hxx"
#include <svtools/ttprops.hxx>
#include <vcl/svapp.hxx>
#include <vcl/bitmap.hxx>
#include <tools/rtti.hxx>

TYPEINIT1( TTProperties, ApplicationProperty )

BOOL TTProperties::RequestProperty( USHORT nRequest )
{
	if ( (( nRequest & TT_PR_ONCE ) == 0) || (nDonePRs & (nRequest & 0x0ff)) == 0 )
	{
		nActualPR = nRequest;
		nDonePRs |= nRequest;
		GetpApp()->Property( *this );
		return nActualPR == 0;
	}
	return TRUE;
}


BOOL TTProperties::GetSlots()
{
	RequestProperty( TT_PR_SLOTS );
	return HasSlots();
}

USHORT TTProperties::ExecuteFunction( USHORT nSID, SfxPoolItem** ppArgs, USHORT nMode )
{
	mnSID = nSID;
	mppArgs = ppArgs;
	mnMode = nMode;
	RequestProperty( TT_PR_DISPATCHER );
	mppArgs = NULL;
	return nActualPR;
}

BOOL TTProperties::Img( Bitmap *pBmp )
{
	BOOL bRet;
	mpBmp = pBmp;
	bRet = RequestProperty( TT_PR_IMG );
	mpBmp = NULL;
	return bRet;
}

SvtResId TTProperties::GetSvtResId( USHORT nId )
{
	return SvtResId( nId );
}

