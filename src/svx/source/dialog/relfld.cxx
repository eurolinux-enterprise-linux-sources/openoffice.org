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
#include "precompiled_svx.hxx"

// include ---------------------------------------------------------------

#include <tools/ref.hxx>
#include "relfld.hxx"

// -----------------------------------------------------------------------

SvxRelativeField::SvxRelativeField( Window* pParent, WinBits nWinSize ) :
	MetricField( pParent, nWinSize )
{
    bNegativeEnabled = FALSE;
    bRelativeMode = FALSE;
	bRelative 	  = FALSE;

	SetDecimalDigits( 2 );
	SetMin( 0 );
	SetMax( 9999 );
}

// -----------------------------------------------------------------------

SvxRelativeField::SvxRelativeField( Window* pParent, const ResId& rResId ) :
	MetricField( pParent, rResId )
{
    bNegativeEnabled = FALSE;
    bRelativeMode = FALSE;
	bRelative	  = FALSE;

	SetDecimalDigits( 2 );
	SetMin( 0 );
	SetMax( 9999 );
}

// -----------------------------------------------------------------------

void SvxRelativeField::Modify()
{
	MetricField::Modify();

	if ( bRelativeMode )
	{
		String  aStr = GetText();
		BOOL    bNewMode = bRelative;

		if ( bRelative )
		{
			const sal_Unicode* pStr = aStr.GetBuffer();

			while ( *pStr )
			{
				if( ( ( *pStr < sal_Unicode( '0' ) ) || ( *pStr > sal_Unicode( '9' ) ) ) &&
					( *pStr != sal_Unicode( '%' ) ) )
				{
					bNewMode = FALSE;
					break;
				}
				pStr++;
			}
		}
		else
		{
			xub_StrLen nPos = aStr.Search( sal_Unicode( '%' ) );

			if ( nPos != STRING_NOTFOUND )
				bNewMode = TRUE;
		}

		if ( bNewMode != bRelative )
			SetRelative( bNewMode );

		MetricField::Modify();
	}
}

// -----------------------------------------------------------------------

void SvxRelativeField::EnableRelativeMode( USHORT nMin,
										   USHORT nMax, USHORT nStep )
{
	bRelativeMode = TRUE;
	nRelMin       = nMin;
	nRelMax       = nMax;
	nRelStep      = nStep;
	SetUnit( FUNIT_CM );
}

// -----------------------------------------------------------------------

void SvxRelativeField::SetRelative( BOOL bNewRelative )
{
	Selection aSelection = GetSelection();
	String aStr = GetText();

	if ( bNewRelative )
	{
		bRelative = TRUE;
		SetDecimalDigits( 0 );
		SetMin( nRelMin );
		SetMax( nRelMax );
		SetCustomUnitText( String( sal_Unicode( '%' ) ) );
		SetUnit( FUNIT_CUSTOM );
	}
	else
	{
		bRelative = FALSE;
		SetDecimalDigits( 2 );
        SetMin( bNegativeEnabled ? -9999 : 0 );
        SetMax( 9999 );
		SetUnit( FUNIT_CM );
	}

	SetText( aStr );
	SetSelection( aSelection );
}


