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
#include "precompiled_sc.hxx"



#include "global.hxx"
#include "scresid.hxx"
#include "impex.hxx"
#include "asciiopt.hxx"
#include "asciiopt.hrc"
#include <tools/debug.hxx>
#include <rtl/tencinfo.h>
#include <unotools/transliterationwrapper.hxx>
// ause
#include "editutil.hxx"

// ============================================================================

static const sal_Char __FAR_DATA pStrFix[] = "FIX";
static const sal_Char __FAR_DATA pStrMrg[] = "MRG";


// ============================================================================

ScAsciiOptions::ScAsciiOptions() :
	bFixedLen		( FALSE ),
	aFieldSeps		( ';' ),
	bMergeFieldSeps	( FALSE ),
	cTextSep		( cDefaultTextSep ),
	eCharSet		( gsl_getSystemTextEncoding() ),
	bCharSetSystem	( FALSE ),
	nStartRow		( 1 ),
	nInfoCount		( 0 ),
    pColStart       ( NULL ),
	pColFormat		( NULL )
{
}


ScAsciiOptions::ScAsciiOptions(const ScAsciiOptions& rOpt) :
	bFixedLen		( rOpt.bFixedLen ),
	aFieldSeps		( rOpt.aFieldSeps ),
	bMergeFieldSeps	( rOpt.bMergeFieldSeps ),
	cTextSep		( rOpt.cTextSep ),
	eCharSet		( rOpt.eCharSet ),
	bCharSetSystem	( rOpt.bCharSetSystem ),
	nStartRow		( rOpt.nStartRow ),
	nInfoCount		( rOpt.nInfoCount )
{
	if (nInfoCount)
	{
		pColStart = new xub_StrLen[nInfoCount];
		pColFormat = new BYTE[nInfoCount];
		for (USHORT i=0; i<nInfoCount; i++)
		{
			pColStart[i] = rOpt.pColStart[i];
			pColFormat[i] = rOpt.pColFormat[i];
		}
	}
	else
	{
		pColStart = NULL;
		pColFormat = NULL;
	}
}


ScAsciiOptions::~ScAsciiOptions()
{
	delete[] pColStart;
	delete[] pColFormat;
}


void ScAsciiOptions::SetColInfo( USHORT nCount, const xub_StrLen* pStart, const BYTE* pFormat )
{
	delete[] pColStart;
	delete[] pColFormat;

	nInfoCount = nCount;

	if (nInfoCount)
	{
		pColStart = new xub_StrLen[nInfoCount];
		pColFormat = new BYTE[nInfoCount];
		for (USHORT i=0; i<nInfoCount; i++)
		{
			pColStart[i] = pStart[i];
			pColFormat[i] = pFormat[i];
		}
	}
	else
	{
		pColStart = NULL;
		pColFormat = NULL;
	}
}


void ScAsciiOptions::SetColumnInfo( const ScCsvExpDataVec& rDataVec )
{
	delete[] pColStart;
    pColStart = NULL;
	delete[] pColFormat;
    pColFormat = NULL;

    nInfoCount = static_cast< sal_uInt16 >( rDataVec.size() );
    if( nInfoCount )
	{
        pColStart = new xub_StrLen[ nInfoCount ];
        pColFormat = new sal_uInt8[ nInfoCount ];
        for( sal_uInt16 nIx = 0; nIx < nInfoCount; ++nIx )
		{
            pColStart[ nIx ] = rDataVec[ nIx ].mnIndex;
            pColFormat[ nIx ] = rDataVec[ nIx ].mnType;
		}
	}
}


ScAsciiOptions&	ScAsciiOptions::operator=( const ScAsciiOptions& rCpy )
{
	SetColInfo( rCpy.nInfoCount, rCpy.pColStart, rCpy.pColFormat );

	bFixedLen		= rCpy.bFixedLen;
	aFieldSeps		= rCpy.aFieldSeps;
	bMergeFieldSeps	= rCpy.bMergeFieldSeps;
	cTextSep		= rCpy.cTextSep;
	eCharSet		= rCpy.eCharSet;
	bCharSetSystem	= rCpy.bCharSetSystem;
	nStartRow		= rCpy.nStartRow;

	return *this;
}


BOOL ScAsciiOptions::operator==( const ScAsciiOptions& rCmp ) const
{
	if ( bFixedLen		 == rCmp.bFixedLen &&
		 aFieldSeps		 == rCmp.aFieldSeps &&
		 bMergeFieldSeps == rCmp.bMergeFieldSeps &&
		 cTextSep		 == rCmp.cTextSep &&
		 eCharSet		 == rCmp.eCharSet &&
		 bCharSetSystem  == rCmp.bCharSetSystem &&
		 nStartRow		 == rCmp.nStartRow &&
		 nInfoCount		 == rCmp.nInfoCount )
	{
		DBG_ASSERT( !nInfoCount || (pColStart && pColFormat && rCmp.pColStart && rCmp.pColFormat),
					 "0-Zeiger in ScAsciiOptions" );
		for (USHORT i=0; i<nInfoCount; i++)
			if ( pColStart[i] != rCmp.pColStart[i] ||
				 pColFormat[i] != rCmp.pColFormat[i] )
				return FALSE;

		return TRUE;
	}
	return FALSE;
}

//
//	Der Options-String darf kein Semikolon mehr enthalten (wegen Pickliste)
//	darum ab Version 336 Komma stattdessen
//


void ScAsciiOptions::ReadFromString( const String& rString )
{
	xub_StrLen nCount = rString.GetTokenCount(',');
	String aToken;
	xub_StrLen nSub;
	xub_StrLen i;

		//
		//	Feld-Trenner
		//

	if ( nCount >= 1 )
	{
		bFixedLen = bMergeFieldSeps = FALSE;
		aFieldSeps.Erase();

		aToken = rString.GetToken(0,',');
		if ( aToken.EqualsAscii(pStrFix) )
			bFixedLen = TRUE;
		nSub = aToken.GetTokenCount('/');
		for ( i=0; i<nSub; i++ )
		{
			String aCode = aToken.GetToken( i, '/' );
			if ( aCode.EqualsAscii(pStrMrg) )
				bMergeFieldSeps = TRUE;
			else
			{
				sal_Int32 nVal = aCode.ToInt32();
				if ( nVal )
					aFieldSeps += (sal_Unicode) nVal;
			}
		}
	}

		//
		//	Text-Trenner
		//

	if ( nCount >= 2 )
	{
		aToken = rString.GetToken(1,',');
		sal_Int32 nVal = aToken.ToInt32();
		cTextSep = (sal_Unicode) nVal;
	}

		//
		//	Zeichensatz
		//

	if ( nCount >= 3 )
	{
		aToken = rString.GetToken(2,',');
		eCharSet = ScGlobal::GetCharsetValue( aToken );
	}

		//
		//	Startzeile
		//

	if ( nCount >= 4 )
	{
		aToken = rString.GetToken(3,',');
		nStartRow = aToken.ToInt32();
	}

		//
		//	Spalten-Infos
		//

	if ( nCount >= 5 )
	{
		delete[] pColStart;
		delete[] pColFormat;

		aToken = rString.GetToken(4,',');
		nSub = aToken.GetTokenCount('/');
		nInfoCount = nSub / 2;
		if (nInfoCount)
		{
			pColStart = new xub_StrLen[nInfoCount];
			pColFormat = new BYTE[nInfoCount];
			for (USHORT nInfo=0; nInfo<nInfoCount; nInfo++)
			{
				pColStart[nInfo]  = (xub_StrLen) aToken.GetToken( 2*nInfo, '/' ).ToInt32();
				pColFormat[nInfo] = (BYTE) aToken.GetToken( 2*nInfo+1, '/' ).ToInt32();
			}
		}
		else
		{
			pColStart = NULL;
			pColFormat = NULL;
		}
	}
}


String ScAsciiOptions::WriteToString() const
{
	String aOutStr;

		//
		//	Feld-Trenner
		//

	if ( bFixedLen )
		aOutStr.AppendAscii(pStrFix);
	else if ( !aFieldSeps.Len() )
		aOutStr += '0';
	else
	{
		xub_StrLen nLen = aFieldSeps.Len();
		for (xub_StrLen i=0; i<nLen; i++)
		{
			if (i)
				aOutStr += '/';
			aOutStr += String::CreateFromInt32(aFieldSeps.GetChar(i));
		}
		if ( bMergeFieldSeps )
		{
			aOutStr += '/';
			aOutStr.AppendAscii(pStrMrg);
		}
	}

	aOutStr += ',';					// Token-Ende

		//
		//	Text-Trenner
		//

	aOutStr += String::CreateFromInt32(cTextSep);
	aOutStr += ',';					// Token-Ende

		//
		//	Zeichensatz
		//

	if ( bCharSetSystem )			// force "SYSTEM"
		aOutStr += ScGlobal::GetCharsetString( RTL_TEXTENCODING_DONTKNOW );
	else
		aOutStr += ScGlobal::GetCharsetString( eCharSet );
	aOutStr += ',';					// Token-Ende

		//
		//	Startzeile
		//

	aOutStr += String::CreateFromInt32(nStartRow);
	aOutStr += ',';					// Token-Ende

		//
		//	Spalten-Infos
		//

	DBG_ASSERT( !nInfoCount || (pColStart && pColFormat), "0-Zeiger in ScAsciiOptions" );
	for (USHORT nInfo=0; nInfo<nInfoCount; nInfo++)
	{
		if (nInfo)
			aOutStr += '/';
		aOutStr += String::CreateFromInt32(pColStart[nInfo]);
		aOutStr += '/';
		aOutStr += String::CreateFromInt32(pColFormat[nInfo]);
	}

	return aOutStr;
}

#if 0
//	Code, um die Spalten-Liste aus einem Excel-kompatiblen String zu erzeugen:
//	(im Moment nicht benutzt)

void ScAsciiOptions::InterpretColumnList( const String& rString )
{
	//	Eingabe ist 1-basiert, pColStart fuer FixedLen ist 0-basiert

	//	Kommas durch Semikolon ersetzen

	String aSemiStr = rString;
	USHORT nPos = 0;
	do
		nPos = aSemiStr.SearchAndReplace( ',', ';', nPos );
	while ( nPos != STRING_NOTFOUND );

	//	Eintraege sortieren

	USHORT nCount = aSemiStr.GetTokenCount();
	USHORT* pTemp = new USHORT[nCount+1];
	pTemp[0] = 1;									// erste Spalte faengt immer bei 1 an
	USHORT nFound = 1;
	USHORT i,j;
	for (i=0; i<nCount; i++)
	{
		USHORT nVal = (USHORT) aSemiStr.GetToken(i);
		if (nVal)
		{
			BOOL bThere = FALSE;
			nPos = 0;
			for (j=0; j<nFound; j++)
			{
				if ( pTemp[j] == nVal )
					bThere = TRUE;
				else if ( pTemp[j] < nVal )
					nPos = j+1;
			}
			if ( !bThere )
			{
				if ( nPos < nFound )
					memmove( &pTemp[nPos+1], &pTemp[nPos], (nFound-nPos)*sizeof(USHORT) );
				pTemp[nPos] = nVal;
				++nFound;
			}
		}
	}

	//	Eintraege uebernehmen

	delete[] pColStart;
	delete[] pColFormat;
	nInfoCount = nFound;
	if (nInfoCount)
	{
		pColStart = new USHORT[nInfoCount];
		pColFormat = new BYTE[nInfoCount];
		for (i=0; i<nInfoCount; i++)
		{
			pColStart[i] = pTemp[i] - 1;
			pColFormat[i] = SC_COL_STANDARD;
		}
	}
	else
	{
		pColStart = NULL;
		pColFormat = NULL;
	}

	bFixedLen = TRUE;			// sonst macht's keinen Sinn

	//	aufraeumen

	delete[] pTemp;
}
#endif

