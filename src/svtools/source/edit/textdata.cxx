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

#include <svtools/textdata.hxx>
#include <textdat2.hxx>

#include <tools/debug.hxx>

SV_IMPL_PTRARR( TextLines, TextLinePtr );
SV_IMPL_VARARR( TEWritingDirectionInfos, TEWritingDirectionInfo );


// -------------------------------------------------------------------------
// (+) class TextSelection
// -------------------------------------------------------------------------

TextSelection::TextSelection()
{
}

TextSelection::TextSelection( const TextPaM& rPaM ) :
	maStartPaM( rPaM ), maEndPaM( rPaM )
{
}

TextSelection::TextSelection( const TextPaM& rStart, const TextPaM& rEnd ) :
	maStartPaM( rStart ), maEndPaM( rEnd )
{
}

void TextSelection::Justify()
{
	if ( maEndPaM < maStartPaM )
	{
		TextPaM aTemp( maStartPaM );
		maStartPaM = maEndPaM;
		maEndPaM = aTemp;
	}
}


// -------------------------------------------------------------------------
// (+) class TETextPortionList
// -------------------------------------------------------------------------
TETextPortionList::TETextPortionList()
{
}

TETextPortionList::~TETextPortionList()
{
	Reset();
}

void TETextPortionList::Reset()
{
	for ( USHORT nPortion = 0; nPortion < Count(); nPortion++ )
		delete GetObject( nPortion );
	Remove( 0, Count() );
}

void TETextPortionList::DeleteFromPortion( USHORT nDelFrom )
{
	DBG_ASSERT( ( nDelFrom < Count() ) || ( (nDelFrom == 0) && (Count() == 0) ), "DeleteFromPortion: Out of range" );
	for ( USHORT nP = nDelFrom; nP < Count(); nP++ )
		delete GetObject( nP );
	Remove( nDelFrom, Count()-nDelFrom );
}

USHORT TETextPortionList::FindPortion( USHORT nCharPos, USHORT& nPortionStart, BOOL bPreferStartingPortion )
{
	// Bei nCharPos an Portion-Grenze wird die linke Portion gefunden
	USHORT nTmpPos = 0;
	for ( USHORT nPortion = 0; nPortion < Count(); nPortion++ )
	{
		TETextPortion* pPortion = GetObject( nPortion );
		nTmpPos = nTmpPos + pPortion->GetLen();
		if ( nTmpPos >= nCharPos )
		{
            // take this one if we don't prefer the starting portion, or if it's the last one
            if ( ( nTmpPos != nCharPos ) || !bPreferStartingPortion || ( nPortion == Count() - 1 ) )
            {
			    nPortionStart = nTmpPos - pPortion->GetLen();
			    return nPortion;
            }
		}
	}
	DBG_ERROR( "FindPortion: Nicht gefunden!" );
	return ( Count() - 1 );
}

/*
USHORT TETextPortionList::GetPortionStartIndex( USHORT nPortion )
{
	USHORT nPos = 0;
	for ( USHORT nP = 0; nP < nPortion; nP++ )
	{
		TETextPortion* pPortion = GetObject( nP );
		nPos += pPortion->GetLen();
	}
	return nPos;
}
*/


// -------------------------------------------------------------------------
// (+) class TEParaPortion
// -------------------------------------------------------------------------
TEParaPortion::TEParaPortion( TextNode* pN )
{
	mpNode = pN;
	mnInvalidPosStart = mnInvalidDiff = 0;
	mbInvalid = TRUE;
	mbSimple = FALSE;
}

TEParaPortion::~TEParaPortion()
{
}

void TEParaPortion::MarkInvalid( USHORT nStart, short nDiff )
{
	if ( mbInvalid == FALSE )
	{
		mnInvalidPosStart = ( nDiff >= 0 ) ? nStart : ( nStart + nDiff );
		mnInvalidDiff = nDiff;
	}
	else
	{
		// Einfaches hintereinander tippen
		if ( ( nDiff > 0 ) && ( mnInvalidDiff > 0 ) &&
			 ( ( mnInvalidPosStart+mnInvalidDiff ) == nStart ) )
		{
			mnInvalidDiff = mnInvalidDiff + nDiff;
		}
		// Einfaches hintereinander loeschen
		else if ( ( nDiff < 0 ) && ( mnInvalidDiff < 0 ) && ( mnInvalidPosStart == nStart ) )
		{
			mnInvalidPosStart = mnInvalidPosStart + nDiff;
			mnInvalidDiff = mnInvalidDiff + nDiff;
		}
		else
		{
			DBG_ASSERT( ( nDiff >= 0 ) || ( (nStart+nDiff) >= 0 ), "MarkInvalid: Diff out of Range" );
			mnInvalidPosStart = Min( mnInvalidPosStart, (USHORT) ( (nDiff < 0) ? nStart+nDiff : nDiff ) );
			mnInvalidDiff = 0;
			mbSimple = FALSE;
		}
	}

    maWritingDirectionInfos.Remove( 0, maWritingDirectionInfos.Count() );

    mbInvalid = TRUE;
}

void TEParaPortion::MarkSelectionInvalid( USHORT nStart, USHORT /*nEnd*/ )
{
	if ( mbInvalid == FALSE )
	{
		mnInvalidPosStart = nStart;
//		nInvalidPosEnd = nEnd;
	}
	else
	{
		mnInvalidPosStart = Min( mnInvalidPosStart, nStart );
//		nInvalidPosEnd = pNode->Len();
	}

    maWritingDirectionInfos.Remove( 0, maWritingDirectionInfos.Count() );

    mnInvalidDiff = 0;
	mbInvalid = TRUE;
	mbSimple = FALSE;
}

USHORT TEParaPortion::GetLineNumber( USHORT nChar, BOOL bInclEnd )
{
	for ( USHORT nLine = 0; nLine < maLines.Count(); nLine++ )
	{
		TextLine* pLine = maLines.GetObject( nLine );
		if ( ( bInclEnd && ( pLine->GetEnd() >= nChar ) ) ||
			 ( pLine->GetEnd() > nChar ) )
		{
			return nLine;
		}
	}

	// Dann sollte es am Ende der letzten Zeile sein!
	DBG_ASSERT( nChar == maLines[ maLines.Count() - 1 ]->GetEnd(), "Index voll daneben!" );
	DBG_ASSERT( !bInclEnd, "Zeile nicht gefunden: FindLine" );
	return ( maLines.Count() - 1 );
}


void TEParaPortion::CorrectValuesBehindLastFormattedLine( USHORT nLastFormattedLine )
{
	USHORT nLines = maLines.Count();
	DBG_ASSERT( nLines, "CorrectPortionNumbersFromLine: Leere Portion?" );
	if ( nLastFormattedLine < ( nLines - 1 ) )
	{
		const TextLine* pLastFormatted = maLines[ nLastFormattedLine ];
		const TextLine* pUnformatted = maLines[ nLastFormattedLine+1 ];
		short nPortionDiff = pUnformatted->GetStartPortion() - pLastFormatted->GetEndPortion();
		short nTextDiff = pUnformatted->GetStart() - pLastFormatted->GetEnd();
		nTextDiff++;	// LastFormatted->GetEnd() war incl. => 1 zuviel abgezogen!

		// Die erste unformatierte muss genau eine Portion hinter der letzten der
		// formatierten beginnen:
		// Wenn in der geaenderten Zeile eine Portion gesplittet wurde,
		// kann nLastEnd > nNextStart sein!
		short nPDiff = sal::static_int_cast< short >(-( nPortionDiff-1 ));
		short nTDiff = sal::static_int_cast< short >(-( nTextDiff-1 ));
		if ( nPDiff || nTDiff )
		{
			for ( USHORT nL = nLastFormattedLine+1; nL < nLines; nL++ )
			{
				TextLine* pLine = maLines[ nL ];

				pLine->GetStartPortion() = pLine->GetStartPortion() + nPDiff;
				pLine->GetEndPortion() = pLine->GetEndPortion() + nPDiff;

				pLine->GetStart() = pLine->GetStart() + nTDiff;
				pLine->GetEnd() = pLine->GetEnd() + nTDiff;

				pLine->SetValid();
			}
		}
	}
}

// -------------------------------------------------------------------------
// (+) class TEParaPortions
// -------------------------------------------------------------------------
TEParaPortions::TEParaPortions()
{
}

TEParaPortions::~TEParaPortions()
{
	Reset();
}

void TEParaPortions::Reset()
{
	TEParaPortions::iterator aIter( begin() );
	while ( aIter != end() )
		delete *aIter++;
	clear();
}

// -------------------------------------------------------------------------
// (+) class IdleFormatter
// -------------------------------------------------------------------------
IdleFormatter::IdleFormatter()
{
	mpView = 0;
	mnRestarts = 0;
}

IdleFormatter::~IdleFormatter()
{
	mpView = 0;
}

void IdleFormatter::DoIdleFormat( TextView* pV, USHORT nMaxRestarts )
{
	mpView = pV;

	if ( IsActive() )
		mnRestarts++;

	if ( mnRestarts > nMaxRestarts )
	{
		mnRestarts = 0;
		((Link&)GetTimeoutHdl()).Call( this );
	}
	else
	{
		Start();
	}
}

void IdleFormatter::ForceTimeout()
{
	if ( IsActive() )
	{
		Stop();
		mnRestarts = 0;
		((Link&)GetTimeoutHdl()).Call( this );
	}
}

TYPEINIT1( TextHint, SfxSimpleHint );

TextHint::TextHint( ULONG Id ) : SfxSimpleHint( Id )
{
	mnValue = 0;
}

TextHint::TextHint( ULONG Id, ULONG nValue ) : SfxSimpleHint( Id )
{
	mnValue = nValue;
}

TEIMEInfos::TEIMEInfos( const TextPaM& rPos, const String& rOldTextAfterStartPos )
: aOldTextAfterStartPos( rOldTextAfterStartPos )
{
	aPos = rPos;
	nLen = 0;
	bCursor = TRUE;
	pAttribs = NULL;
	bWasCursorOverwrite = FALSE;
}

TEIMEInfos::~TEIMEInfos()
{
	delete[] pAttribs;
}

void TEIMEInfos::CopyAttribs( const USHORT* pA, USHORT nL )
{
	nLen = nL;
	delete pAttribs;
	pAttribs = new USHORT[ nL ];
	memcpy( pAttribs, pA, nL*sizeof(USHORT) );
}

void TEIMEInfos::DestroyAttribs()
{
	delete pAttribs;
	pAttribs = NULL;
    nLen = 0;
}


