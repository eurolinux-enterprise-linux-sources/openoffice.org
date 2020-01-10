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

#include <paralist.hxx>
#include <svx/outliner.hxx>		// nur wegen Paragraph, muss geaendert werden!
#include <svx/numdef.hxx>

DBG_NAME(Paragraph)

ParagraphData::ParagraphData()
: nDepth( -1 )
, mnNumberingStartValue( -1 )
, mbParaIsNumberingRestart( sal_False )
{
}

ParagraphData::ParagraphData( const ParagraphData& r )
: nDepth( r.nDepth )
, mnNumberingStartValue( r.mnNumberingStartValue )
, mbParaIsNumberingRestart( r.mbParaIsNumberingRestart )
{
}

ParagraphData& ParagraphData::operator=( const ParagraphData& r)
{
    nDepth = r.nDepth;
    mnNumberingStartValue = r.mnNumberingStartValue;
    mbParaIsNumberingRestart = r.mbParaIsNumberingRestart;
    return *this;
}

bool ParagraphData::operator==(const ParagraphData& rCandidate) const
{
    return (nDepth == rCandidate.nDepth
        && mnNumberingStartValue == rCandidate.mnNumberingStartValue
        && mbParaIsNumberingRestart == rCandidate.mbParaIsNumberingRestart);
}

Paragraph::Paragraph( sal_Int16 nDDepth )
: aBulSize( -1, -1)
{
	DBG_CTOR( Paragraph, 0 );

    DBG_ASSERT(  ( nDDepth >= -1 ) && ( nDDepth < SVX_MAX_NUM ), "Paragraph-CTOR: nDepth invalid!" );

	nDepth = nDDepth;
	nFlags = 0;
	bVisible = TRUE;
}

Paragraph::Paragraph( const Paragraph& rPara )
: ParagraphData( rPara )
, aBulText( rPara.aBulText )
, aBulSize( rPara.aBulSize )
{
	DBG_CTOR( Paragraph, 0 );

	nDepth = rPara.nDepth;
	nFlags = rPara.nFlags;
	bVisible = rPara.bVisible;
}

Paragraph::Paragraph( const ParagraphData& rData )
: nFlags( 0 )
, aBulSize( -1, -1)
, bVisible( TRUE )
{
	DBG_CTOR( Paragraph, 0 );

    nDepth = rData.nDepth;
    mnNumberingStartValue = rData.mnNumberingStartValue;
    mbParaIsNumberingRestart = rData.mbParaIsNumberingRestart;
}

Paragraph::~Paragraph()
{
	DBG_DTOR( Paragraph, 0 );
}

void Paragraph::SetNumberingStartValue( sal_Int16 nNumberingStartValue )
{
    mnNumberingStartValue = nNumberingStartValue;
    if( mnNumberingStartValue != -1 )
        mbParaIsNumberingRestart = true;
}

void Paragraph::SetParaIsNumberingRestart( sal_Bool bParaIsNumberingRestart )
{
    mbParaIsNumberingRestart = bParaIsNumberingRestart;
    if( !mbParaIsNumberingRestart )
        mnNumberingStartValue = -1;
}

void ParagraphList::Clear( BOOL bDestroyParagraphs )
{
	if ( bDestroyParagraphs )
	{
		for ( ULONG n = GetParagraphCount(); n; )
		{
			Paragraph* pPara = GetParagraph( --n );
			delete pPara;
		}
	}
	List::Clear();
}

void ParagraphList::MoveParagraphs( ULONG nStart, ULONG nDest, ULONG _nCount )
{
	if ( ( nDest < nStart ) || ( nDest >= ( nStart + _nCount ) ) )
	{
		ULONG n;
		ParagraphList aParas;
		for ( n = 0; n < _nCount; n++ )
		{
			Paragraph* pPara = GetParagraph( nStart );
			aParas.Insert( pPara, LIST_APPEND );
			Remove( nStart );
		}

		if ( nDest > nStart )
			nDest -= _nCount;

		for ( n = 0; n < _nCount; n++ )
		{
			Paragraph* pPara = aParas.GetParagraph( n );
			Insert( pPara, nDest++ );
		}
	}
	else
	{
		DBG_ERROR( "MoveParagraphs: Invalid Parameters" );
	}
}

Paragraph* ParagraphList::NextVisible( Paragraph* pPara ) const
{
	ULONG n = GetAbsPos( pPara );

	Paragraph* p = GetParagraph( ++n );
	while ( p && !p->IsVisible() )
		p = GetParagraph( ++n );

	return p;
}

Paragraph* ParagraphList::PrevVisible( Paragraph* pPara ) const
{
	ULONG n = GetAbsPos( pPara );

	Paragraph* p = n ? GetParagraph( --n ) : NULL;
	while ( p && !p->IsVisible() )
		p = n ? GetParagraph( --n ) : NULL;

	return p;
}

Paragraph* ParagraphList::LastVisible() const
{
	ULONG n = GetParagraphCount();

	Paragraph* p = n ? GetParagraph( --n ) : NULL;
	while ( p && !p->IsVisible() )
		p = n ? GetParagraph( --n ) : NULL;

	return p;
}

BOOL ParagraphList::HasChilds( Paragraph* pParagraph ) const
{
	ULONG n = GetAbsPos( pParagraph );
	Paragraph* pNext = GetParagraph( ++n );
	return ( pNext && ( pNext->GetDepth() > pParagraph->GetDepth() ) ) ? TRUE : FALSE;
}

BOOL ParagraphList::HasHiddenChilds( Paragraph* pParagraph ) const
{
	ULONG n = GetAbsPos( pParagraph );
	Paragraph* pNext = GetParagraph( ++n );
	return ( pNext && ( pNext->GetDepth() > pParagraph->GetDepth() ) && !pNext->IsVisible() ) ? TRUE : FALSE;
}

BOOL ParagraphList::HasVisibleChilds( Paragraph* pParagraph ) const
{
	ULONG n = GetAbsPos( pParagraph );
	Paragraph* pNext = GetParagraph( ++n );
	return ( pNext && ( pNext->GetDepth() > pParagraph->GetDepth() ) && pNext->IsVisible() ) ? TRUE : FALSE;
}

ULONG ParagraphList::GetChildCount( Paragraph* pParent ) const
{
	ULONG nChildCount = 0;
	ULONG n = GetAbsPos( pParent );
	Paragraph* pPara = GetParagraph( ++n );
	while ( pPara && ( pPara->GetDepth() > pParent->GetDepth() ) )
	{
		nChildCount++;
		pPara = GetParagraph( ++n );
	}
	return nChildCount;
}

Paragraph* ParagraphList::GetParent( Paragraph* pParagraph /*, USHORT& rRelPos */ ) const
{
	/* rRelPos = 0 */;
	ULONG n = GetAbsPos( pParagraph );
	Paragraph* pPrev = GetParagraph( --n );
	while ( pPrev && ( pPrev->GetDepth() >= pParagraph->GetDepth() ) )
	{
//		if ( pPrev->GetDepth() == pParagraph->GetDepth() )
//			rRelPos++;
		pPrev = GetParagraph( --n );
	}

	return pPrev;
}

void ParagraphList::Expand( Paragraph* pParent )
{
	ULONG nChildCount = GetChildCount( pParent );
	ULONG nPos = GetAbsPos( pParent );

	for ( ULONG n = 1; n <= nChildCount; n++  )
	{
		Paragraph* pPara = GetParagraph( nPos+n );
		if ( !( pPara->IsVisible() ) )
		{
			pPara->bVisible = TRUE;
			aVisibleStateChangedHdl.Call( pPara );
		}
	}
}

void ParagraphList::Collapse( Paragraph* pParent )
{
	ULONG nChildCount = GetChildCount( pParent );
	ULONG nPos = GetAbsPos( pParent );

	for ( ULONG n = 1; n <= nChildCount; n++  )
	{
		Paragraph* pPara = GetParagraph( nPos+n );
		if ( pPara->IsVisible() )
		{
			pPara->bVisible = FALSE;
			aVisibleStateChangedHdl.Call( pPara );
		}
	}
}

ULONG ParagraphList::GetVisPos( Paragraph* pPara )
{
	ULONG nVisPos = 0;
	ULONG nPos = GetAbsPos( pPara );
	for ( ULONG n = 0; n < nPos; n++ )
	{
		Paragraph* _pPara = GetParagraph( n );
		if ( _pPara->IsVisible() )
			nVisPos++;
	}
	return nVisPos;
}
