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



// INCLUDE ---------------------------------------------------------------

#include <map>

#include <svtools/poolcach.hxx>
#include <svtools/zforlist.hxx>
#include <svx/scripttypeitem.hxx>
#include <string.h>

#include "scitems.hxx"
#include "column.hxx"
#include "cell.hxx"
#include "document.hxx"
#include "docpool.hxx"
#include "attarray.hxx"
#include "patattr.hxx"
#include "compiler.hxx"
#include "brdcst.hxx"
#include "markdata.hxx"
#include "detfunc.hxx"			// for Notes in Sort/Swap
#include "postit.hxx"

//#pragma optimize ( "", off )
//	nur Search ohne Optimierung!

// STATIC DATA -----------------------------------------------------------
using namespace formula;

inline BOOL IsAmbiguousScriptNonZero( BYTE nScript )
{
	//!	move to a header file
	return ( nScript != SCRIPTTYPE_LATIN &&
			 nScript != SCRIPTTYPE_ASIAN &&
			 nScript != SCRIPTTYPE_COMPLEX &&
			 nScript != 0 );
}

// -----------------------------------------------------------------------------------------


ScColumn::ScColumn() :
	nCol( 0 ),
	nCount( 0 ),
	nLimit( 0 ),
	pItems( NULL ),
    pAttrArray( NULL ),
    pDocument( NULL )
{
}


ScColumn::~ScColumn()
{
	FreeAll();
	if (pAttrArray) delete pAttrArray;
}


void ScColumn::Init(SCCOL nNewCol, SCTAB nNewTab, ScDocument* pDoc)
{
	nCol = nNewCol;
	nTab = nNewTab;
	pDocument = pDoc;
	pAttrArray = new ScAttrArray( nCol, nTab, pDocument );
}


SCsROW ScColumn::GetNextUnprotected( SCROW nRow, BOOL bUp ) const
{
	return pAttrArray->GetNextUnprotected(nRow, bUp);
}


USHORT ScColumn::GetBlockMatrixEdges( SCROW nRow1, SCROW nRow2, USHORT nMask ) const
{
	// nix:0, mitte:1, unten:2, links:4, oben:8, rechts:16, offen:32
	if ( !pItems )
		return 0;
	if ( nRow1 == nRow2 )
	{
		SCSIZE nIndex;
		if ( Search( nRow1, nIndex ) )
		{
			ScBaseCell* pCell = pItems[nIndex].pCell;
			if ( pCell->GetCellType() == CELLTYPE_FORMULA
				&& ((ScFormulaCell*)pCell)->GetMatrixFlag() )
			{
				ScAddress aOrg( ScAddress::INITIALIZE_INVALID );
				return ((ScFormulaCell*)pCell)->GetMatrixEdge( aOrg );
			}
		}
		return 0;
	}
	else
	{
		ScAddress aOrg( ScAddress::INITIALIZE_INVALID );
		BOOL bOpen = FALSE;
		USHORT nEdges = 0;
		SCSIZE nIndex;
		Search( nRow1, nIndex );
		while ( nIndex < nCount && pItems[nIndex].nRow <= nRow2 )
		{
			ScBaseCell* pCell = pItems[nIndex].pCell;
			if ( pCell->GetCellType() == CELLTYPE_FORMULA
				&& ((ScFormulaCell*)pCell)->GetMatrixFlag() )
			{
				nEdges = ((ScFormulaCell*)pCell)->GetMatrixEdge( aOrg );
				if ( nEdges )
				{
					if ( nEdges & 8 )
						bOpen = TRUE;	// obere Kante oeffnet, weitersehen
					else if ( !bOpen )
						return nEdges | 32;	// es gibt was, was nicht geoeffnet wurde
					else if ( nEdges & 1 )
						return nEdges;	// mittendrin
					// (nMask & 16 und  (4 und nicht 16)) oder
					// (nMask & 4  und (16 und nicht 4))
					if ( ((nMask & 16) && (nEdges & 4)  && !(nEdges & 16))
						|| ((nMask & 4)  && (nEdges & 16) && !(nEdges & 4)) )
						return nEdges;	// nur linke/rechte Kante
					if ( nEdges & 2 )
						bOpen = FALSE;	// untere Kante schliesst
				}
			}
			nIndex++;
		}
		if ( bOpen )
			nEdges |= 32;			// es geht noch weiter
		return nEdges;
	}
}


BOOL ScColumn::HasSelectionMatrixFragment(const ScMarkData& rMark) const
{
	if ( rMark.IsMultiMarked() )
	{
		BOOL bFound = FALSE;

		ScAddress aOrg( ScAddress::INITIALIZE_INVALID );
		ScAddress aCurOrg( ScAddress::INITIALIZE_INVALID );
		SCROW nTop, nBottom;
		ScMarkArrayIter aMarkIter( rMark.GetArray()+nCol );
		while ( !bFound && aMarkIter.Next( nTop, nBottom ) )
		{
			BOOL bOpen = FALSE;
			USHORT nEdges;
			SCSIZE nIndex;
			Search( nTop, nIndex );
			while ( !bFound && nIndex < nCount && pItems[nIndex].nRow <= nBottom )
			{
				ScBaseCell* pCell = pItems[nIndex].pCell;
				if ( pCell->GetCellType() == CELLTYPE_FORMULA
					&& ((ScFormulaCell*)pCell)->GetMatrixFlag() )
				{
					nEdges = ((ScFormulaCell*)pCell)->GetMatrixEdge( aOrg );
					if ( nEdges )
					{
						if ( nEdges & 8 )
							bOpen = TRUE;	// obere Kante oeffnet, weitersehen
						else if ( !bOpen )
							return TRUE;	// es gibt was, was nicht geoeffnet wurde
						else if ( nEdges & 1 )
							bFound = TRUE;	// mittendrin, alles selektiert?
						// (4 und nicht 16) oder (16 und nicht 4)
						if ( (((nEdges & 4) | 16) ^ ((nEdges & 16) | 4)) )
							bFound = TRUE;	// nur linke/rechte Kante, alles selektiert?
						if ( nEdges & 2 )
							bOpen = FALSE;	// untere Kante schliesst

						if ( bFound )
						{	// alles selektiert?
							if ( aCurOrg != aOrg )
							{	// neue Matrix zu pruefen?
								aCurOrg = aOrg;
								ScFormulaCell* pFCell;
								if ( ((ScFormulaCell*)pCell)->GetMatrixFlag()
										== MM_REFERENCE )
									pFCell = (ScFormulaCell*) pDocument->GetCell( aOrg );
								else
									pFCell = (ScFormulaCell*)pCell;
                                SCCOL nC;
                                SCROW nR;
                                pFCell->GetMatColsRows( nC, nR );
								ScRange aRange( aOrg, ScAddress(
									aOrg.Col() + nC - 1, aOrg.Row() + nR - 1,
									aOrg.Tab() ) );
								if ( rMark.IsAllMarked( aRange ) )
									bFound = FALSE;
							}
							else
								bFound = FALSE;		// war schon
						}
					}
				}
				nIndex++;
			}
			if ( bOpen )
				return TRUE;
		}
		return bFound;
	}
	else
		return FALSE;
}


//UNUSED2009-05 BOOL ScColumn::HasLines( SCROW nRow1, SCROW nRow2, Rectangle& rSizes,
//UNUSED2009-05                             BOOL bLeft, BOOL bRight ) const
//UNUSED2009-05 {
//UNUSED2009-05     return pAttrArray->HasLines( nRow1, nRow2, rSizes, bLeft, bRight );
//UNUSED2009-05 }


BOOL ScColumn::HasAttrib( SCROW nRow1, SCROW nRow2, USHORT nMask ) const
{
	return pAttrArray->HasAttrib( nRow1, nRow2, nMask );
}


BOOL ScColumn::HasAttribSelection( const ScMarkData& rMark, USHORT nMask ) const
{
	BOOL bFound = FALSE;

	SCROW nTop;
	SCROW nBottom;

	if (rMark.IsMultiMarked())
	{
		ScMarkArrayIter aMarkIter( rMark.GetArray()+nCol );
		while (aMarkIter.Next( nTop, nBottom ) && !bFound)
		{
			if (pAttrArray->HasAttrib( nTop, nBottom, nMask ))
				bFound = TRUE;
		}
	}

	return bFound;
}


BOOL ScColumn::ExtendMerge( SCCOL nThisCol, SCROW nStartRow, SCROW nEndRow,
							SCCOL& rPaintCol, SCROW& rPaintRow,
							BOOL bRefresh, BOOL bAttrs )
{
	return pAttrArray->ExtendMerge( nThisCol, nStartRow, nEndRow, rPaintCol, rPaintRow, bRefresh, bAttrs );
}


void ScColumn::MergeSelectionPattern( ScMergePatternState& rState, const ScMarkData& rMark, BOOL bDeep ) const
{
	SCROW nTop;
	SCROW nBottom;

	if ( rMark.IsMultiMarked() )
	{
        const ScMarkArray* pArray = rMark.GetArray() + nCol;
        if ( pArray->HasMarks() )
        {
            ScMarkArrayIter aMarkIter( pArray );
            while (aMarkIter.Next( nTop, nBottom ))
                pAttrArray->MergePatternArea( nTop, nBottom, rState, bDeep );
        }
	}
}


void ScColumn::MergePatternArea( ScMergePatternState& rState, SCROW nRow1, SCROW nRow2, BOOL bDeep ) const
{
	pAttrArray->MergePatternArea( nRow1, nRow2, rState, bDeep );
}


void ScColumn::MergeBlockFrame( SvxBoxItem* pLineOuter, SvxBoxInfoItem* pLineInner,
							ScLineFlags& rFlags,
							SCROW nStartRow, SCROW nEndRow, BOOL bLeft, SCCOL nDistRight ) const
{
	pAttrArray->MergeBlockFrame( pLineOuter, pLineInner, rFlags, nStartRow, nEndRow, bLeft, nDistRight );
}


void ScColumn::ApplyBlockFrame( const SvxBoxItem* pLineOuter, const SvxBoxInfoItem* pLineInner,
							SCROW nStartRow, SCROW nEndRow, BOOL bLeft, SCCOL nDistRight )
{
	pAttrArray->ApplyBlockFrame( pLineOuter, pLineInner, nStartRow, nEndRow, bLeft, nDistRight );
}


const ScPatternAttr* ScColumn::GetPattern( SCROW nRow ) const
{
	return pAttrArray->GetPattern( nRow );
}


const SfxPoolItem* ScColumn::GetAttr( SCROW nRow, USHORT nWhich ) const
{
	return &pAttrArray->GetPattern( nRow )->GetItemSet().Get(nWhich);
}


const ScPatternAttr* ScColumn::GetMostUsedPattern( SCROW nStartRow, SCROW nEndRow ) const
{
    ::std::map< const ScPatternAttr*, size_t > aAttrMap;
    const ScPatternAttr* pMaxPattern = 0;
    size_t nMaxCount = 0;

    ScAttrIterator aAttrIter( pAttrArray, nStartRow, nEndRow );
    const ScPatternAttr* pPattern;
    SCROW nAttrRow1 = 0, nAttrRow2 = 0;

    while( (pPattern = aAttrIter.Next( nAttrRow1, nAttrRow2 )) != 0 )
    {
        size_t& rnCount = aAttrMap[ pPattern ];
        rnCount += (nAttrRow2 - nAttrRow1 + 1);
        if( rnCount > nMaxCount )
        {
            pMaxPattern = pPattern;
            nMaxCount = rnCount;
        }
    }

    return pMaxPattern;
}


ULONG ScColumn::GetNumberFormat( SCROW nRow ) const
{
	return pAttrArray->GetPattern( nRow )->GetNumberFormat( pDocument->GetFormatTable() );
}


SCsROW ScColumn::ApplySelectionCache( SfxItemPoolCache* pCache, const ScMarkData& rMark )
{
    SCROW nTop = 0;
    SCROW nBottom = 0;
	BOOL bFound = FALSE;

	if ( rMark.IsMultiMarked() )
	{
		ScMarkArrayIter aMarkIter( rMark.GetArray() + nCol );
		while (aMarkIter.Next( nTop, nBottom ))
		{
			pAttrArray->ApplyCacheArea( nTop, nBottom, pCache );
			bFound = TRUE;
		}
	}

	if (!bFound)
		return -1;
	else if (nTop==0 && nBottom==MAXROW)
		return 0;
	else
		return nBottom;
}


void ScColumn::ChangeSelectionIndent( BOOL bIncrement, const ScMarkData& rMark )
{
	SCROW nTop;
	SCROW nBottom;

	if ( pAttrArray && rMark.IsMultiMarked() )
	{
		ScMarkArrayIter aMarkIter( rMark.GetArray() + nCol );
		while (aMarkIter.Next( nTop, nBottom ))
			pAttrArray->ChangeIndent(nTop, nBottom, bIncrement);
	}
}


void ScColumn::ClearSelectionItems( const USHORT* pWhich,const ScMarkData& rMark )
{
	SCROW nTop;
	SCROW nBottom;

	if ( pAttrArray && rMark.IsMultiMarked() )
	{
		ScMarkArrayIter aMarkIter( rMark.GetArray() + nCol );
		while (aMarkIter.Next( nTop, nBottom ))
			pAttrArray->ClearItems(nTop, nBottom, pWhich);
	}
}


void ScColumn::DeleteSelection( USHORT nDelFlag, const ScMarkData& rMark )
{
	SCROW nTop;
	SCROW nBottom;

	if ( rMark.IsMultiMarked() )
	{
		ScMarkArrayIter aMarkIter( rMark.GetArray() + nCol );
		while (aMarkIter.Next( nTop, nBottom ))
            DeleteArea(nTop, nBottom, nDelFlag);
	}
}


void ScColumn::ApplyPattern( SCROW nRow, const ScPatternAttr& rPatAttr )
{
	const SfxItemSet* pSet = &rPatAttr.GetItemSet();
	SfxItemPoolCache aCache( pDocument->GetPool(), pSet );

	const ScPatternAttr* pPattern = pAttrArray->GetPattern( nRow );

	//	TRUE = alten Eintrag behalten

	ScPatternAttr* pNewPattern = (ScPatternAttr*) &aCache.ApplyTo( *pPattern, TRUE );
	ScDocumentPool::CheckRef( *pPattern );
	ScDocumentPool::CheckRef( *pNewPattern );

	if (pNewPattern != pPattern)
	  pAttrArray->SetPattern( nRow, pNewPattern );
}


void ScColumn::ApplyPatternArea( SCROW nStartRow, SCROW nEndRow, const ScPatternAttr& rPatAttr )
{
	const SfxItemSet* pSet = &rPatAttr.GetItemSet();
	SfxItemPoolCache aCache( pDocument->GetPool(), pSet );
	pAttrArray->ApplyCacheArea( nStartRow, nEndRow, &aCache );
}


void ScColumn::ApplyPatternIfNumberformatIncompatible( const ScRange& rRange,
		const ScPatternAttr& rPattern, short nNewType )
{
	const SfxItemSet* pSet = &rPattern.GetItemSet();
	SfxItemPoolCache aCache( pDocument->GetPool(), pSet );
	SvNumberFormatter* pFormatter = pDocument->GetFormatTable();
	SCROW nEndRow = rRange.aEnd.Row();
	for ( SCROW nRow = rRange.aStart.Row(); nRow <= nEndRow; nRow++ )
	{
		SCROW nRow1, nRow2;
		const ScPatternAttr* pPattern = pAttrArray->GetPatternRange(
			nRow1, nRow2, nRow );
		ULONG nFormat = pPattern->GetNumberFormat( pFormatter );
		short nOldType = pFormatter->GetType( nFormat );
		if ( nOldType == nNewType || pFormatter->IsCompatible( nOldType, nNewType ) )
			nRow = nRow2;
		else
		{
			SCROW nNewRow1 = Max( nRow1, nRow );
			SCROW nNewRow2 = Min( nRow2, nEndRow );
			pAttrArray->ApplyCacheArea( nNewRow1, nNewRow2, &aCache );
			nRow = nNewRow2;
		}
	}
}


void ScColumn::ApplyStyle( SCROW nRow, const ScStyleSheet& rStyle )
{
	const ScPatternAttr* pPattern = pAttrArray->GetPattern(nRow);
	ScPatternAttr* pNewPattern = new ScPatternAttr(*pPattern);
	if (pNewPattern)
	{
		pNewPattern->SetStyleSheet((ScStyleSheet*)&rStyle);
		pAttrArray->SetPattern(nRow, pNewPattern, TRUE);
		delete pNewPattern;
	}
}


void ScColumn::ApplyStyleArea( SCROW nStartRow, SCROW nEndRow, const ScStyleSheet& rStyle )
{
	pAttrArray->ApplyStyleArea(nStartRow, nEndRow, (ScStyleSheet*)&rStyle);
}


void ScColumn::ApplySelectionStyle(const ScStyleSheet& rStyle, const ScMarkData& rMark)
{
	SCROW nTop;
	SCROW nBottom;

	if ( rMark.IsMultiMarked() )
	{
		ScMarkArrayIter aMarkIter( rMark.GetArray() + nCol );
		while (aMarkIter.Next( nTop, nBottom ))
			pAttrArray->ApplyStyleArea(nTop, nBottom, (ScStyleSheet*)&rStyle);
	}
}


void ScColumn::ApplySelectionLineStyle( const ScMarkData& rMark,
									const SvxBorderLine* pLine, BOOL bColorOnly )
{
	if ( bColorOnly && !pLine )
		return;

	SCROW nTop;
	SCROW nBottom;

	if (rMark.IsMultiMarked())
	{
		ScMarkArrayIter aMarkIter( rMark.GetArray()+nCol );
		while (aMarkIter.Next( nTop, nBottom ))
			pAttrArray->ApplyLineStyleArea(nTop, nBottom, pLine, bColorOnly );
	}
}


const ScStyleSheet* ScColumn::GetStyle( SCROW nRow ) const
{
	return pAttrArray->GetPattern( nRow )->GetStyleSheet();
}


const ScStyleSheet* ScColumn::GetSelectionStyle( const ScMarkData& rMark, BOOL& rFound ) const
{
	rFound = FALSE;
	if (!rMark.IsMultiMarked())
	{
		DBG_ERROR("ScColumn::GetSelectionStyle ohne Selektion");
		return NULL;
	}

	BOOL bEqual = TRUE;

	const ScStyleSheet* pStyle = NULL;
	const ScStyleSheet* pNewStyle;

	ScMarkArrayIter aMarkIter( rMark.GetArray() + nCol );
	SCROW nTop;
	SCROW nBottom;
	while (bEqual && aMarkIter.Next( nTop, nBottom ))
	{
		ScAttrIterator aAttrIter( pAttrArray, nTop, nBottom );
		SCROW nRow;
		SCROW nDummy;
		const ScPatternAttr* pPattern;
        while (bEqual && ( pPattern = aAttrIter.Next( nRow, nDummy ) ) != NULL)
		{
			pNewStyle = pPattern->GetStyleSheet();
			rFound = TRUE;
			if ( !pNewStyle || ( pStyle && pNewStyle != pStyle ) )
				bEqual = FALSE;												// unterschiedliche
			pStyle = pNewStyle;
		}
	}

	return bEqual ? pStyle : NULL;
}


const ScStyleSheet*	ScColumn::GetAreaStyle( BOOL& rFound, SCROW nRow1, SCROW nRow2 ) const
{
	rFound = FALSE;

	BOOL bEqual = TRUE;

	const ScStyleSheet* pStyle = NULL;
	const ScStyleSheet* pNewStyle;

	ScAttrIterator aAttrIter( pAttrArray, nRow1, nRow2 );
	SCROW nRow;
	SCROW nDummy;
	const ScPatternAttr* pPattern;
    while (bEqual && ( pPattern = aAttrIter.Next( nRow, nDummy ) ) != NULL)
	{
		pNewStyle = pPattern->GetStyleSheet();
		rFound = TRUE;
		if ( !pNewStyle || ( pStyle && pNewStyle != pStyle ) )
			bEqual = FALSE;												// unterschiedliche
		pStyle = pNewStyle;
	}

	return bEqual ? pStyle : NULL;
}


void ScColumn::FindStyleSheet( const SfxStyleSheetBase* pStyleSheet, BOOL* pUsed, BOOL bReset )
{
	pAttrArray->FindStyleSheet( pStyleSheet, pUsed, bReset );
}


BOOL ScColumn::IsStyleSheetUsed( const ScStyleSheet& rStyle, BOOL bGatherAllStyles ) const
{
	return pAttrArray->IsStyleSheetUsed( rStyle, bGatherAllStyles );
}


BOOL ScColumn::ApplyFlags( SCROW nStartRow, SCROW nEndRow, INT16 nFlags )
{
	return pAttrArray->ApplyFlags( nStartRow, nEndRow, nFlags );
}


BOOL ScColumn::RemoveFlags( SCROW nStartRow, SCROW nEndRow, INT16 nFlags )
{
	return pAttrArray->RemoveFlags( nStartRow, nEndRow, nFlags );
}


void ScColumn::ClearItems( SCROW nStartRow, SCROW nEndRow, const USHORT* pWhich )
{
	pAttrArray->ClearItems( nStartRow, nEndRow, pWhich );
}


void ScColumn::SetPattern( SCROW nRow, const ScPatternAttr& rPatAttr, BOOL bPutToPool )
{
	pAttrArray->SetPattern( nRow, &rPatAttr, bPutToPool );
}


void ScColumn::SetPatternArea( SCROW nStartRow, SCROW nEndRow,
								const ScPatternAttr& rPatAttr, BOOL bPutToPool )
{
	pAttrArray->SetPatternArea( nStartRow, nEndRow, &rPatAttr, bPutToPool );
}


void ScColumn::ApplyAttr( SCROW nRow, const SfxPoolItem& rAttr )
{
	//	um nur ein neues SetItem zu erzeugen, brauchen wir keinen SfxItemPoolCache.
	//!	Achtung: der SfxItemPoolCache scheint zuviele Refs fuer das neue SetItem zu erzeugen ??

	ScDocumentPool* pDocPool = pDocument->GetPool();

	const ScPatternAttr* pOldPattern = pAttrArray->GetPattern( nRow );
	ScPatternAttr* pTemp = new ScPatternAttr(*pOldPattern);
	pTemp->GetItemSet().Put(rAttr);
	const ScPatternAttr* pNewPattern = (const ScPatternAttr*) &pDocPool->Put( *pTemp );

	if ( pNewPattern != pOldPattern )
		pAttrArray->SetPattern( nRow, pNewPattern );
	else
		pDocPool->Remove( *pNewPattern );		// ausser Spesen nichts gewesen

	delete pTemp;

		// alte Version mit SfxItemPoolCache:
#if 0
	SfxItemPoolCache aCache( pDocument->GetPool(), &rAttr );

	const ScPatternAttr* pPattern = pAttrArray->GetPattern( nRow );

	//	TRUE = alten Eintrag behalten

	ScPatternAttr* pNewPattern = (ScPatternAttr*) &aCache.ApplyTo( *pPattern, TRUE );
	ScDocumentPool::CheckRef( *pPattern );
	ScDocumentPool::CheckRef( *pNewPattern );

	if (pNewPattern != pPattern)
	  pAttrArray->SetPattern( nRow, pNewPattern );
#endif
}

#ifdef _MSC_VER
#pragma optimize ( "", off )
#endif


BOOL ScColumn::Search( SCROW nRow, SCSIZE& nIndex ) const
{
	if ( !pItems || !nCount )
	{
		nIndex = 0;
		return FALSE;
	}
	SCROW nMinRow = pItems[0].nRow;
	if ( nRow <= nMinRow )
	{
		nIndex = 0;
		return nRow == nMinRow;
	}
	SCROW nMaxRow = pItems[nCount-1].nRow;
	if ( nRow >= nMaxRow )
	{
		if ( nRow == nMaxRow )
		{
			nIndex = nCount - 1;
			return TRUE;
		}
		else
		{
			nIndex = nCount;
			return FALSE;
		}
	}

	long nOldLo, nOldHi;
	long	nLo 	= nOldLo = 0;
    long	nHi 	= nOldHi = Min(static_cast<long>(nCount)-1, static_cast<long>(nRow) );
	long	i		= 0;
	BOOL	bFound	= FALSE;
	// quite continuous distribution? => interpolating search
	BOOL	bInterpol = (static_cast<SCSIZE>(nMaxRow - nMinRow) < nCount * 2);
	SCROW	nR;

	while ( !bFound && nLo <= nHi )
	{
		if ( !bInterpol || nHi - nLo < 3 )
			i = (nLo+nHi) / 2;			// no effort, no division by zero
		else
		{	// interpolating search
			long nLoRow = pItems[nLo].nRow;		// no unsigned underflow upon substraction
			i = nLo + (long)((long)(nRow - nLoRow) * (nHi - nLo)
				/ (pItems[nHi].nRow - nLoRow));
			if ( i < 0 || static_cast<SCSIZE>(i) >= nCount )
			{	// oops ...
				i = (nLo+nHi) / 2;
				bInterpol = FALSE;
			}
		}
		nR = pItems[i].nRow;
		if ( nR < nRow )
		{
			nLo = i+1;
			if ( bInterpol )
			{
				if ( nLo <= nOldLo )
					bInterpol = FALSE;
				else
					nOldLo = nLo;
			}
		}
		else
		{
			if ( nR > nRow )
			{
				nHi = i-1;
				if ( bInterpol )
				{
					if ( nHi >= nOldHi )
						bInterpol = FALSE;
					else
						nOldHi = nHi;
				}
			}
			else
				bFound = TRUE;
		}
	}
	if (bFound)
		nIndex = static_cast<SCSIZE>(i);
	else
		nIndex = static_cast<SCSIZE>(nLo); // rear index
	return bFound;
}

#ifdef _MSC_VER
#pragma optimize ( "", on )
#endif


ScBaseCell* ScColumn::GetCell( SCROW nRow ) const
{
	SCSIZE nIndex;
	if (Search(nRow, nIndex))
		return pItems[nIndex].pCell;
	return NULL;
}


void ScColumn::Resize( SCSIZE nSize )
{
    if (nSize > sal::static_int_cast<SCSIZE>(MAXROWCOUNT))
		nSize = MAXROWCOUNT;
	if (nSize < nCount)
		nSize = nCount;

	ColEntry* pNewItems;
	if (nSize)
	{
		SCSIZE nNewSize = nSize + COLUMN_DELTA - 1;
		nNewSize -= nNewSize % COLUMN_DELTA;
		nLimit = nNewSize;
		pNewItems = new ColEntry[nLimit];
	}
	else
	{
		nLimit = 0;
		pNewItems = NULL;
	}
	if (pItems)
	{
		if (pNewItems)
			memmove( pNewItems, pItems, nCount * sizeof(ColEntry) );
		delete[] pItems;
	}
	pItems = pNewItems;
}

//	SwapRow zum Sortieren

namespace {

/** Moves broadcaster from old cell to new cell if exists, otherwise creates a new note cell. */
void lclTakeBroadcaster( ScBaseCell*& rpCell, SvtBroadcaster* pBC )
{
    if( pBC )
    {
        if( rpCell )
            rpCell->TakeBroadcaster( pBC );
        else
            rpCell = new ScNoteCell( pBC );
    }
}

} // namespace

void ScColumn::SwapRow(SCROW nRow1, SCROW nRow2)
{
    /*  Simple swap of cell pointers does not work if broadcasters exist (crash
        if cell broadcasts directly or indirectly to itself). While swapping
        the cells, broadcasters have to remain at old positions! */

    /*  While cloning cells, do not clone notes, but move note pointers to new
        cells. This prevents creation of new caption drawing objects for every
        swap operation while sorting. */

	ScBaseCell* pCell1 = 0;
	SCSIZE nIndex1;
	if ( Search( nRow1, nIndex1 ) )
		pCell1 = pItems[nIndex1].pCell;

    ScBaseCell* pCell2 = 0;
	SCSIZE nIndex2;
	if ( Search( nRow2, nIndex2 ) )
		pCell2 = pItems[nIndex2].pCell;

    // no cells found, nothing to do
	if ( !pCell1 && !pCell2 )
		return ;

    // swap variables if first cell is empty, to save some code below
    if ( !pCell1 )
    {
        ::std::swap( nRow1, nRow2 );
        ::std::swap( nIndex1, nIndex2 );
        ::std::swap( pCell1, pCell2 );
    }

    // from here: first cell (pCell1, nIndex1) exists always

    ScAddress aPos1( nCol, nRow1, nTab );
    ScAddress aPos2( nCol, nRow2, nTab );

	CellType eType1 = pCell1->GetCellType();
	CellType eType2 = pCell2 ? pCell2->GetCellType() : CELLTYPE_NONE;

    ScFormulaCell* pFmlaCell1 = (eType1 == CELLTYPE_FORMULA) ? static_cast< ScFormulaCell* >( pCell1 ) : 0;
    ScFormulaCell* pFmlaCell2 = (eType2 == CELLTYPE_FORMULA) ? static_cast< ScFormulaCell* >( pCell2 ) : 0;

    // simple swap if no formula cells present
	if ( !pFmlaCell1 && !pFmlaCell2 )
	{
        // remember cell broadcasters, must remain at old position
        SvtBroadcaster* pBC1 = pCell1->ReleaseBroadcaster();

		if ( pCell2 )
		{
            /*  Both cells exist, no formula cells involved, a simple swap can
                be performed (but keep broadcasters and notes at old position). */
			pItems[nIndex1].pCell = pCell2;
			pItems[nIndex2].pCell = pCell1;

            SvtBroadcaster* pBC2 = pCell2->ReleaseBroadcaster();
            pCell1->TakeBroadcaster( pBC2 );
            pCell2->TakeBroadcaster( pBC1 );

            ScHint aHint1( SC_HINT_DATACHANGED, aPos1, pCell2 );
			pDocument->Broadcast( aHint1 );
            ScHint aHint2( SC_HINT_DATACHANGED, aPos2, pCell1 );
			pDocument->Broadcast( aHint2 );
		}
		else
		{
			ScNoteCell* pDummyCell = pBC1 ? new ScNoteCell( pBC1 ) : 0;
			if ( pDummyCell )
			{
                // insert dummy note cell (without note) containing old broadcaster
				pItems[nIndex1].pCell = pDummyCell;
			}
			else
			{
                // remove ColEntry at old position
				--nCount;
				memmove( &pItems[nIndex1], &pItems[nIndex1 + 1], (nCount - nIndex1) * sizeof(ColEntry) );
				pItems[nCount].nRow = 0;
				pItems[nCount].pCell = 0;
			}

            // insert ColEntry at new position
			Insert( nRow2, pCell1 );
			pDocument->Broadcast( ScHint( SC_HINT_DATACHANGED, aPos1, pDummyCell ) );
		}

		return;
	}

	// from here: at least one of the cells is a formula cell

    /*  Never move any array formulas. Disabling sort if parts of array
        formulas are contained is done at UI. */
    if ( (pFmlaCell1 && (pFmlaCell1->GetMatrixFlag() != 0)) || (pFmlaCell2 && (pFmlaCell2->GetMatrixFlag() != 0)) )
        return;

    // do not swap, if formulas are equal
	if ( pFmlaCell1 && pFmlaCell2 )
	{
		ScTokenArray* pCode1 = pFmlaCell1->GetCode();
		ScTokenArray* pCode2 = pFmlaCell2->GetCode();

		if (pCode1->GetLen() == pCode2->GetLen())		// nicht-UPN
		{
			BOOL bEqual = TRUE;
			USHORT nLen = pCode1->GetLen();
			FormulaToken** ppToken1 = pCode1->GetArray();
			FormulaToken** ppToken2 = pCode2->GetArray();
			for (USHORT i=0; i<nLen; i++)
            {
                if ( !ppToken1[i]->TextEqual(*(ppToken2[i])) ||
                        ppToken1[i]->Is3DRef() || ppToken2[i]->Is3DRef() )
				{
					bEqual = FALSE;
					break;
				}
            }

            // do not swap formula cells with equal formulas, but swap notes
			if (bEqual)
            {
                ScPostIt* pNote1 = pCell1->ReleaseNote();
                pCell1->TakeNote( pCell2->ReleaseNote() );
                pCell2->TakeNote( pNote1 );
				return;
            }
		}
	}

	//	hier kein UpdateReference wegen #30529# - mitsortiert werden nur noch relative Referenzen
//	long dy = (long)nRow2 - (long)nRow1;

    /*  Create clone of pCell1 at position of pCell2 (pCell1 exists always, see
        variable swapping above). Do not clone the note, but move pointer of
        old note to new cell. */
    ScBaseCell* pNew2 = pCell1->CloneWithoutNote( *pDocument, aPos2, SC_CLONECELL_ADJUST3DREL );
    pNew2->TakeNote( pCell1->ReleaseNote() );

    /*  Create clone of pCell2 at position of pCell1. Do not clone the note,
        but move pointer of old note to new cell. */
	ScBaseCell* pNew1 = 0;
	if ( pCell2 )
	{
        pNew1 = pCell2->CloneWithoutNote( *pDocument, aPos1, SC_CLONECELL_ADJUST3DREL );
        pNew1->TakeNote( pCell2->ReleaseNote() );
	}

    // move old broadcasters new cells at the same old position
    SvtBroadcaster* pBC1 = pCell1->ReleaseBroadcaster();
    lclTakeBroadcaster( pNew1, pBC1 );
    SvtBroadcaster* pBC2 = pCell2 ? pCell2->ReleaseBroadcaster() : 0;
    lclTakeBroadcaster( pNew2, pBC2 );

	/*  Insert the new cells. Old cell has to be deleted, if there is no new
        cell (call to Insert deletes old cell by itself). */
	if ( !pNew1 )
		Delete( nRow1 );            // deletes pCell1
	else
        Insert( nRow1, pNew1 );     // deletes pCell1, inserts pNew1

	if ( pCell2 && !pNew2 )
		Delete( nRow2 );            // deletes pCell2
	else if ( pNew2 )
		Insert( nRow2, pNew2 );     // deletes pCell2 (if existing), inserts pNew2

	//	#64122# Bei Formeln hinterher nochmal broadcasten, damit die Formel nicht in irgendwelchen
	//	FormulaTrack-Listen landet, ohne die Broadcaster beruecksichtigt zu haben
	//	(erst hier, wenn beide Zellen eingefuegt sind)
	if ( pBC1 && pFmlaCell2 )
		pDocument->Broadcast( ScHint( SC_HINT_DATACHANGED, aPos1, pNew1 ) );
	if ( pBC2 && pFmlaCell1 )
		pDocument->Broadcast( ScHint( SC_HINT_DATACHANGED, aPos2, pNew2 ) );
}


void ScColumn::SwapCell( SCROW nRow, ScColumn& rCol)
{
	ScBaseCell* pCell1 = 0;
	SCSIZE nIndex1;
	if ( Search( nRow, nIndex1 ) )
		pCell1 = pItems[nIndex1].pCell;

    ScBaseCell* pCell2 = 0;
	SCSIZE nIndex2;
	if ( rCol.Search( nRow, nIndex2 ) )
		pCell2 = rCol.pItems[nIndex2].pCell;

    // reverse call if own cell is missing (ensures own existing cell in following code)
    if( !pCell1 )
    {
        if( pCell2 )
            rCol.SwapCell( nRow, *this );
        return;
    }

    // from here: own cell (pCell1, nIndex1) exists always

    ScFormulaCell* pFmlaCell1 = (pCell1->GetCellType() == CELLTYPE_FORMULA) ? static_cast< ScFormulaCell* >( pCell1 ) : 0;
	ScFormulaCell* pFmlaCell2 = (pCell2 && (pCell2->GetCellType() == CELLTYPE_FORMULA)) ? static_cast< ScFormulaCell* >( pCell2 ) : 0;

	if ( pCell2 )
	{
		// Tauschen
		pItems[nIndex1].pCell = pCell2;
		rCol.pItems[nIndex2].pCell = pCell1;
		// Referenzen aktualisieren
		SCsCOL dx = rCol.nCol - nCol;
		if ( pFmlaCell1 )
		{
			ScRange aRange( ScAddress( rCol.nCol, 0, nTab ),
							ScAddress( rCol.nCol, MAXROW, nTab ) );
			pFmlaCell1->aPos.SetCol( rCol.nCol );
			pFmlaCell1->UpdateReference(URM_MOVE, aRange, dx, 0, 0);
		}
		if ( pFmlaCell2 )
		{
			ScRange aRange( ScAddress( nCol, 0, nTab ),
							ScAddress( nCol, MAXROW, nTab ) );
			pFmlaCell2->aPos.SetCol( nCol );
			pFmlaCell2->UpdateReference(URM_MOVE, aRange, -dx, 0, 0);
		}
	}
	else
    {
        // Loeschen
        --nCount;
        memmove( &pItems[nIndex1], &pItems[nIndex1 + 1], (nCount - nIndex1) * sizeof(ColEntry) );
        pItems[nCount].nRow = 0;
        pItems[nCount].pCell = 0;
        // Referenzen aktualisieren
        SCsCOL dx = rCol.nCol - nCol;
        if ( pFmlaCell1 )
        {
            ScRange aRange( ScAddress( rCol.nCol, 0, nTab ),
                            ScAddress( rCol.nCol, MAXROW, nTab ) );
            pFmlaCell1->aPos.SetCol( rCol.nCol );
            pFmlaCell1->UpdateReference(URM_MOVE, aRange, dx, 0, 0);
        }
        // Einfuegen
        rCol.Insert(nRow, pCell1);
    }
}


BOOL ScColumn::TestInsertCol( SCROW nStartRow, SCROW nEndRow) const
{
	if (!IsEmpty())
	{
		BOOL bTest = TRUE;
		if (pItems)
			for (SCSIZE i=0; (i<nCount) && bTest; i++)
				bTest = (pItems[i].nRow < nStartRow) || (pItems[i].nRow > nEndRow)
                        || pItems[i].pCell->IsBlank();

		//	AttrArray testet nur zusammengefasste

		if ((bTest) && (pAttrArray))
			bTest = pAttrArray->TestInsertCol(nStartRow, nEndRow);

		//!		rausgeschobene Attribute bei Undo beruecksichtigen

		return bTest;
	}
	else
		return TRUE;
}


BOOL ScColumn::TestInsertRow( SCSIZE nSize ) const
{
    //  AttrArray only looks for merged cells

	if ( pItems && nCount )
        return ( nSize <= sal::static_int_cast<SCSIZE>(MAXROW) &&
                 pItems[nCount-1].nRow <= MAXROW-(SCROW)nSize && pAttrArray->TestInsertRow( nSize ) );
	else
		return pAttrArray->TestInsertRow( nSize );

#if 0
	//!		rausgeschobene Attribute bei Undo beruecksichtigen

	if ( nSize > static_cast<SCSIZE>(MAXROW) )
		return FALSE;

	SCSIZE nVis = nCount;
    while ( nVis && pItems[nVis-1].pCell->IsBlank() )
		--nVis;

	if ( nVis )
		return ( pItems[nVis-1].nRow <= MAXROW-nSize );
	else
		return TRUE;
#endif
}


void ScColumn::InsertRow( SCROW nStartRow, SCSIZE nSize )
{
	pAttrArray->InsertRow( nStartRow, nSize );

	//!	Search

	if ( !pItems || !nCount )
		return;

	SCSIZE i;
	Search( nStartRow, i );
	if ( i >= nCount )
		return ;

	BOOL bOldAutoCalc = pDocument->GetAutoCalc();
	pDocument->SetAutoCalc( FALSE );	// Mehrfachberechnungen vermeiden

	SCSIZE nNewCount = nCount;
	BOOL bCountChanged = FALSE;
	ScAddress aAdr( nCol, 0, nTab );
    ScHint aHint( SC_HINT_DATACHANGED, aAdr, NULL );    // only areas (ScBaseCell* == NULL)
    ScAddress& rAddress = aHint.GetAddress();
    // for sparse occupation use single broadcasts, not ranges
    BOOL bSingleBroadcasts = (((pItems[nCount-1].nRow - pItems[i].nRow) /
                (nCount - i)) > 1);
    if ( bSingleBroadcasts )
    {
        SCROW nLastBroadcast = MAXROW+1;
        for ( ; i < nCount; i++)
        {
            SCROW nOldRow = pItems[i].nRow;
            // #43940# Aenderung Quelle broadcasten
            if ( nLastBroadcast != nOldRow )
            {   // direkt aufeinanderfolgende nicht doppelt broadcasten
                rAddress.SetRow( nOldRow );
                pDocument->AreaBroadcast( aHint );
            }
            SCROW nNewRow = (pItems[i].nRow += nSize);
            // #43940# Aenderung Ziel broadcasten
            rAddress.SetRow( nNewRow );
            pDocument->AreaBroadcast( aHint );
            nLastBroadcast = nNewRow;
            ScBaseCell* pCell = pItems[i].pCell;
            if ( pCell->GetCellType() == CELLTYPE_FORMULA )
                ((ScFormulaCell*)pCell)->aPos.SetRow( nNewRow );
            if ( nNewRow > MAXROW && !bCountChanged )
            {
                nNewCount = i;
                bCountChanged = TRUE;
            }
        }
    }
    else
    {
        rAddress.SetRow( pItems[i].nRow );
        ScRange aRange( rAddress );
        for ( ; i < nCount; i++)
        {
            SCROW nNewRow = (pItems[i].nRow += nSize);
            ScBaseCell* pCell = pItems[i].pCell;
            if ( pCell->GetCellType() == CELLTYPE_FORMULA )
                ((ScFormulaCell*)pCell)->aPos.SetRow( nNewRow );
            if ( nNewRow > MAXROW && !bCountChanged )
            {
                nNewCount = i;
                bCountChanged = TRUE;
                aRange.aEnd.SetRow( MAXROW );
            }
        }
        if ( !bCountChanged )
            aRange.aEnd.SetRow( pItems[nCount-1].nRow );
        pDocument->AreaBroadcastInRange( aRange, aHint );
    }

	if (bCountChanged)
	{
		SCSIZE nDelCount = nCount - nNewCount;
		ScBaseCell** ppDelCells = new ScBaseCell*[nDelCount];
		SCROW* pDelRows = new SCROW[nDelCount];
		for (i = 0; i < nDelCount; i++)
		{
			ppDelCells[i] = pItems[nNewCount+i].pCell;
			pDelRows[i] = pItems[nNewCount+i].nRow;
		}
		nCount = nNewCount;

		for (i = 0; i < nDelCount; i++)
		{
			ScBaseCell* pCell = ppDelCells[i];
            DBG_ASSERT( pCell->IsBlank(), "sichtbare Zelle weggeschoben" );
			SvtBroadcaster* pBC = pCell->GetBroadcaster();
			if (pBC)
			{
				MoveListeners( *pBC, pDelRows[i] - nSize );
                pCell->DeleteBroadcaster();
				pCell->Delete();
			}
		}

		delete pDelRows;
		delete ppDelCells;
	}

	pDocument->SetAutoCalc( bOldAutoCalc );
}


void ScColumn::CopyToClip(SCROW nRow1, SCROW nRow2, ScColumn& rColumn, BOOL bKeepScenarioFlags, BOOL bCloneNoteCaptions)
{
	pAttrArray->CopyArea( nRow1, nRow2, 0, *rColumn.pAttrArray,
							bKeepScenarioFlags ? (SC_MF_ALL & ~SC_MF_SCENARIO) : SC_MF_ALL );

	SCSIZE i;
	SCSIZE nBlockCount = 0;
    SCSIZE nStartIndex = 0, nEndIndex = 0;
	for (i = 0; i < nCount; i++)
		if ((pItems[i].nRow >= nRow1) && (pItems[i].nRow <= nRow2))
		{
			if (!nBlockCount)
				nStartIndex = i;
			nEndIndex = i;
			++nBlockCount;

			//	im Clipboard muessen interpretierte Zellen stehen, um andere Formate
			//	(Text, Grafik...) erzueugen zu koennen

			if ( pItems[i].pCell->GetCellType() == CELLTYPE_FORMULA )
			{
				ScFormulaCell* pFCell = (ScFormulaCell*) pItems[i].pCell;
				if (pFCell->GetDirty() && pDocument->GetAutoCalc())
					pFCell->Interpret();
			}
		}

	if (nBlockCount)
	{
        int nCloneFlags = bCloneNoteCaptions ? SC_CLONECELL_DEFAULT : SC_CLONECELL_NOCAPTION;
		rColumn.Resize( rColumn.GetCellCount() + nBlockCount );
        ScAddress aOwnPos( nCol, 0, nTab );
        ScAddress aDestPos( rColumn.nCol, 0, rColumn.nTab );
		for (i = nStartIndex; i <= nEndIndex; i++)
        {
            aOwnPos.SetRow( pItems[i].nRow );
            aDestPos.SetRow( pItems[i].nRow );
            ScBaseCell* pNewCell = pItems[i].pCell->CloneWithNote( aOwnPos, *rColumn.pDocument, aDestPos, nCloneFlags );
            rColumn.Append( aDestPos.Row(), pNewCell );
        }
	}
}


void ScColumn::CopyToColumn(SCROW nRow1, SCROW nRow2, USHORT nFlags, BOOL bMarked,
								ScColumn& rColumn, const ScMarkData* pMarkData, BOOL bAsLink )
{
	if (bMarked)
	{
		SCROW nStart, nEnd;
		if (pMarkData && pMarkData->IsMultiMarked())
		{
			ScMarkArrayIter aIter( pMarkData->GetArray()+nCol );

			while ( aIter.Next( nStart, nEnd ) && nStart <= nRow2 )
			{
				if ( nEnd >= nRow1 )
					CopyToColumn( Max(nRow1,nStart), Min(nRow2,nEnd),
									nFlags, FALSE, rColumn, pMarkData, bAsLink );
			}
		}
		else
		{
			DBG_ERROR("CopyToColumn: bMarked, aber keine Markierung");
		}
		return;
	}

	if ( (nFlags & IDF_ATTRIB) != 0 )
	{
		if ( (nFlags & IDF_STYLES) != IDF_STYLES )
		{	// StyleSheets im Zieldokument bleiben erhalten
			// z.B. DIF und RTF Clipboard-Import
			for ( SCROW nRow = nRow1; nRow <= nRow2; nRow++ )
			{
				const ScStyleSheet* pStyle =
					rColumn.pAttrArray->GetPattern( nRow )->GetStyleSheet();
				const ScPatternAttr* pPattern = pAttrArray->GetPattern( nRow );
				ScPatternAttr* pNewPattern = new ScPatternAttr( *pPattern );
				pNewPattern->SetStyleSheet( (ScStyleSheet*)pStyle );
				rColumn.pAttrArray->SetPattern( nRow, pNewPattern, TRUE );
				delete pNewPattern;
			}
		}
		else
			pAttrArray->CopyArea( nRow1, nRow2, 0, *rColumn.pAttrArray);
	}


	if ((nFlags & IDF_CONTENTS) != 0)
	{
		SCSIZE i;
		SCSIZE nBlockCount = 0;
        SCSIZE nStartIndex = 0, nEndIndex = 0;
		for (i = 0; i < nCount; i++)
			if ((pItems[i].nRow >= nRow1) && (pItems[i].nRow <= nRow2))
			{
				if (!nBlockCount)
					nStartIndex = i;
				nEndIndex = i;
				++nBlockCount;
			}

		if (nBlockCount)
		{
			rColumn.Resize( rColumn.GetCellCount() + nBlockCount );
			ScAddress aDestPos( rColumn.nCol, 0, rColumn.nTab );
			for (i = nStartIndex; i <= nEndIndex; i++)
			{
				aDestPos.SetRow( pItems[i].nRow );
				ScBaseCell* pNew = bAsLink ?
                    CreateRefCell( rColumn.pDocument, aDestPos, i, nFlags ) :
                    CloneCell( i, nFlags, *rColumn.pDocument, aDestPos );

				if (pNew)
					rColumn.Insert(pItems[i].nRow, pNew);
			}
		}
	}
}


void ScColumn::UndoToColumn(SCROW nRow1, SCROW nRow2, USHORT nFlags, BOOL bMarked,
								ScColumn& rColumn, const ScMarkData* pMarkData )
{
	if (nRow1 > 0)
		CopyToColumn( 0, nRow1-1, IDF_FORMULA, FALSE, rColumn );

	CopyToColumn( nRow1, nRow2, nFlags, bMarked, rColumn, pMarkData );		//! bMarked ????

	if (nRow2 < MAXROW)
		CopyToColumn( nRow2+1, MAXROW, IDF_FORMULA, FALSE, rColumn );
}


void ScColumn::CopyUpdated( const ScColumn& rPosCol, ScColumn& rDestCol ) const
{
	ScDocument& rDestDoc = *rDestCol.pDocument;
    ScAddress aOwnPos( nCol, 0, nTab );
    ScAddress aDestPos( rDestCol.nCol, 0, rDestCol.nTab );

	SCSIZE nPosCount = rPosCol.nCount;
	for (SCSIZE nPosIndex = 0; nPosIndex < nPosCount; nPosIndex++)
	{
        aOwnPos.SetRow( rPosCol.pItems[nPosIndex].nRow );
        aDestPos.SetRow( aOwnPos.Row() );
		SCSIZE nThisIndex;
		if ( Search( aDestPos.Row(), nThisIndex ) )
		{
            ScBaseCell* pNew = pItems[nThisIndex].pCell->CloneWithNote( aOwnPos, rDestDoc, aDestPos );
			rDestCol.Insert( aDestPos.Row(), pNew );
		}
	}

	//	Dummy:
	//	CopyToColumn( 0,MAXROW, IDF_FORMULA, FALSE, rDestCol, NULL, FALSE );
}


void ScColumn::CopyScenarioFrom( const ScColumn& rSrcCol )
{
	//	Dies ist die Szenario-Tabelle, die Daten werden hineinkopiert

	ScAttrIterator aAttrIter( pAttrArray, 0, MAXROW );
	SCROW nStart, nEnd;
	const ScPatternAttr* pPattern = aAttrIter.Next( nStart, nEnd );
	while (pPattern)
	{
		if ( ((ScMergeFlagAttr&)pPattern->GetItem( ATTR_MERGE_FLAG )).IsScenario() )
		{
			DeleteArea( nStart, nEnd, IDF_CONTENTS );
			((ScColumn&)rSrcCol).
				CopyToColumn( nStart, nEnd, IDF_CONTENTS, FALSE, *this );

			//	UpdateUsed nicht noetig, schon in TestCopyScenario passiert

			SCsTAB nDz = nTab - rSrcCol.nTab;
			UpdateReference(URM_COPY, nCol, nStart, nTab,
									  nCol, nEnd,   nTab,
									  0, 0, nDz, NULL);
			UpdateCompile();
		}

		//!	CopyToColumn "const" machen !!!

		pPattern = aAttrIter.Next( nStart, nEnd );
	}
}


void ScColumn::CopyScenarioTo( ScColumn& rDestCol ) const
{
	//	Dies ist die Szenario-Tabelle, die Daten werden in die andere kopiert

	ScAttrIterator aAttrIter( pAttrArray, 0, MAXROW );
	SCROW nStart, nEnd;
	const ScPatternAttr* pPattern = aAttrIter.Next( nStart, nEnd );
	while (pPattern)
	{
		if ( ((ScMergeFlagAttr&)pPattern->GetItem( ATTR_MERGE_FLAG )).IsScenario() )
		{
			rDestCol.DeleteArea( nStart, nEnd, IDF_CONTENTS );
			((ScColumn*)this)->
				CopyToColumn( nStart, nEnd, IDF_CONTENTS, FALSE, rDestCol );

			//	UpdateUsed nicht noetig, schon in TestCopyScenario passiert

			SCsTAB nDz = rDestCol.nTab - nTab;
			rDestCol.UpdateReference(URM_COPY, rDestCol.nCol, nStart, rDestCol.nTab,
											   rDestCol.nCol, nEnd,   rDestCol.nTab,
											   0, 0, nDz, NULL);
			rDestCol.UpdateCompile();
		}

		//!	CopyToColumn "const" machen !!!

		pPattern = aAttrIter.Next( nStart, nEnd );
	}
}


BOOL ScColumn::TestCopyScenarioTo( const ScColumn& rDestCol ) const
{
	BOOL bOk = TRUE;
	ScAttrIterator aAttrIter( pAttrArray, 0, MAXROW );
	SCROW nStart = 0, nEnd = 0;
	const ScPatternAttr* pPattern = aAttrIter.Next( nStart, nEnd );
	while (pPattern && bOk)
	{
		if ( ((ScMergeFlagAttr&)pPattern->GetItem( ATTR_MERGE_FLAG )).IsScenario() )
			if ( rDestCol.pAttrArray->HasAttrib( nStart, nEnd, HASATTR_PROTECTED ) )
				bOk = FALSE;

		pPattern = aAttrIter.Next( nStart, nEnd );
	}
	return bOk;
}


void ScColumn::MarkScenarioIn( ScMarkData& rDestMark ) const
{
	ScRange aRange( nCol, 0, nTab );

	ScAttrIterator aAttrIter( pAttrArray, 0, MAXROW );
	SCROW nStart, nEnd;
	const ScPatternAttr* pPattern = aAttrIter.Next( nStart, nEnd );
	while (pPattern)
	{
		if ( ((ScMergeFlagAttr&)pPattern->GetItem( ATTR_MERGE_FLAG )).IsScenario() )
		{
			aRange.aStart.SetRow( nStart );
			aRange.aEnd.SetRow( nEnd );
			rDestMark.SetMultiMarkArea( aRange, TRUE );
		}

		pPattern = aAttrIter.Next( nStart, nEnd );
	}
}


void ScColumn::SwapCol(ScColumn& rCol)
{
	SCSIZE nTemp;

	nTemp = rCol.nCount;
	rCol.nCount  = nCount;
	nCount = nTemp;

	nTemp = rCol.nLimit;
	rCol.nLimit = nLimit;
	nLimit = nTemp;

	ColEntry* pTempItems = rCol.pItems;
	rCol.pItems = pItems;
	pItems = pTempItems;

	ScAttrArray* pTempAttr = rCol.pAttrArray;
	rCol.pAttrArray = pAttrArray;
	pAttrArray = pTempAttr;

	// #38415# AttrArray muss richtige Spaltennummer haben
	pAttrArray->SetCol(nCol);
	rCol.pAttrArray->SetCol(rCol.nCol);

	SCSIZE i;
	if (pItems)
		for (i = 0; i < nCount; i++)
		{
			ScFormulaCell* pCell = (ScFormulaCell*) pItems[i].pCell;
			if( pCell->GetCellType() == CELLTYPE_FORMULA)
				pCell->aPos.SetCol(nCol);
		}
	if (rCol.pItems)
		for (i = 0; i < rCol.nCount; i++)
		{
			ScFormulaCell* pCell = (ScFormulaCell*) rCol.pItems[i].pCell;
			if( pCell->GetCellType() == CELLTYPE_FORMULA)
				pCell->aPos.SetCol(rCol.nCol);
		}
}


void ScColumn::MoveTo(SCROW nStartRow, SCROW nEndRow, ScColumn& rCol)
{
	pAttrArray->MoveTo(nStartRow, nEndRow, *rCol.pAttrArray);

	if (pItems)
	{
        ::std::vector<SCROW> aRows;
        bool bConsecutive = true;
		SCSIZE i;
        Search( nStartRow, i);  // i points to start row or position thereafter
		SCSIZE nStartPos = i;
		for ( ; i < nCount && pItems[i].nRow <= nEndRow; ++i)
		{
            SCROW nRow = pItems[i].nRow;
            aRows.push_back( nRow);
            rCol.Insert( nRow, pItems[i].pCell);
            if (nRow != pItems[i].nRow)
            {   // Listener inserted
                bConsecutive = false;
                Search( nRow, i);
            }
		}
        SCSIZE nStopPos = i;
		if (nStartPos < nStopPos)
		{
            // Create list of ranges of cell entry positions
            typedef ::std::pair<SCSIZE,SCSIZE> PosPair;
            typedef ::std::vector<PosPair> EntryPosPairs;
            EntryPosPairs aEntries;
            if (bConsecutive)
                aEntries.push_back( PosPair(nStartPos, nStopPos));
            else
            {
                bool bFirst = true;
                nStopPos = 0;
                for (::std::vector<SCROW>::const_iterator it( aRows.begin());
                        it != aRows.end() && nStopPos < nCount; ++it,
                        ++nStopPos)
                {
                    if (!bFirst && *it != pItems[nStopPos].nRow)
                    {
                        aEntries.push_back( PosPair(nStartPos, nStopPos));
                        bFirst = true;
                    }
                    if (bFirst && Search( *it, nStartPos))
                    {
                        bFirst = false;
                        nStopPos = nStartPos;
                    }
                }
                if (!bFirst && nStartPos < nStopPos)
                    aEntries.push_back( PosPair(nStartPos, nStopPos));
            }
			// Broadcast changes
            ScAddress aAdr( nCol, 0, nTab );
            ScHint aHint( SC_HINT_DYING, aAdr, NULL );  // areas only
            ScAddress& rAddress = aHint.GetAddress();
            ScNoteCell* pNoteCell = new ScNoteCell;		// Dummy like in DeleteRange

            // #121990# must iterate backwards, because indexes of following cells become invalid
            for (EntryPosPairs::reverse_iterator it( aEntries.rbegin());
                    it != aEntries.rend(); ++it)
            {
                nStartPos = (*it).first;
                nStopPos = (*it).second;
                for (i=nStartPos; i<nStopPos; ++i)
                    pItems[i].pCell = pNoteCell;
                for (i=nStartPos; i<nStopPos; ++i)
                {
                    rAddress.SetRow( pItems[i].nRow );
                    pDocument->AreaBroadcast( aHint );
                }
                nCount -= nStopPos - nStartPos;
                memmove( &pItems[nStartPos], &pItems[nStopPos],
                        (nCount - nStartPos) * sizeof(ColEntry) );
            }
            delete pNoteCell;
            pItems[nCount].nRow = 0;
            pItems[nCount].pCell = NULL;
		}
	}
}


void ScColumn::UpdateReference( UpdateRefMode eUpdateRefMode, SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
			 SCCOL nCol2, SCROW nRow2, SCTAB nTab2, SCsCOL nDx, SCsROW nDy, SCsTAB nDz,
			 ScDocument* pUndoDoc )
{
	if (pItems)
	{
		ScRange aRange( ScAddress( nCol1, nRow1, nTab1 ),
						ScAddress( nCol2, nRow2, nTab2 ) );
		if ( eUpdateRefMode == URM_COPY && nRow1 == nRow2 )
		{	// z.B. eine einzelne Zelle aus dem Clipboard eingefuegt
			SCSIZE nIndex;
			if ( Search( nRow1, nIndex ) )
			{
				ScFormulaCell* pCell = (ScFormulaCell*) pItems[nIndex].pCell;
				if( pCell->GetCellType() == CELLTYPE_FORMULA)
					pCell->UpdateReference(	eUpdateRefMode, aRange, nDx, nDy, nDz, pUndoDoc );
			}
		}
		else
		{
            // #90279# For performance reasons two loop bodies instead of
            // testing for update mode in each iteration.
            // Anyways, this is still a bottleneck on large arrays with few
            // formulas cells.
            if ( eUpdateRefMode == URM_COPY )
            {
                SCSIZE i;
                Search( nRow1, i );
                for ( ; i < nCount; i++ )
                {
                    SCROW nRow = pItems[i].nRow;
                    if ( nRow > nRow2 )
                        break;
                    ScBaseCell* pCell = pItems[i].pCell;
                    if( pCell->GetCellType() == CELLTYPE_FORMULA)
                    {
                        ((ScFormulaCell*)pCell)->UpdateReference( eUpdateRefMode, aRange, nDx, nDy, nDz, pUndoDoc );
                        if ( nRow != pItems[i].nRow )
                            Search( nRow, i );  // Listener removed/inserted?
                    }
                }
            }
            else
            {
                SCSIZE i = 0;
                for ( ; i < nCount; i++ )
                {
                    ScBaseCell* pCell = pItems[i].pCell;
                    if( pCell->GetCellType() == CELLTYPE_FORMULA)
                    {
                        SCROW nRow = pItems[i].nRow;
                        // When deleting rows on several sheets, the formula's position may be updated with the first call,
                        // so the undo position must be passed from here.
                        ScAddress aUndoPos( nCol, nRow, nTab );
                        ((ScFormulaCell*)pCell)->UpdateReference( eUpdateRefMode, aRange, nDx, nDy, nDz, pUndoDoc, &aUndoPos );
                        if ( nRow != pItems[i].nRow )
                            Search( nRow, i );  // Listener removed/inserted?
                    }
                }
            }
		}
	}
}


void ScColumn::UpdateTranspose( const ScRange& rSource, const ScAddress& rDest,
									ScDocument* pUndoDoc )
{
	if (pItems)
		for (SCSIZE i=0; i<nCount; i++)
		{
			ScBaseCell* pCell = pItems[i].pCell;
			if (pCell->GetCellType() == CELLTYPE_FORMULA)
			{
				SCROW nRow = pItems[i].nRow;
				((ScFormulaCell*)pCell)->UpdateTranspose( rSource, rDest, pUndoDoc );
				if ( nRow != pItems[i].nRow )
					Search( nRow, i );				// Listener geloescht/eingefuegt?
			}
		}
}


void ScColumn::UpdateGrow( const ScRange& rArea, SCCOL nGrowX, SCROW nGrowY )
{
	if (pItems)
		for (SCSIZE i=0; i<nCount; i++)
		{
			ScBaseCell* pCell = pItems[i].pCell;
			if (pCell->GetCellType() == CELLTYPE_FORMULA)
			{
				SCROW nRow = pItems[i].nRow;
				((ScFormulaCell*)pCell)->UpdateGrow( rArea, nGrowX, nGrowY );
				if ( nRow != pItems[i].nRow )
					Search( nRow, i );				// Listener geloescht/eingefuegt?
			}
		}
}


void ScColumn::UpdateInsertTab( SCTAB nTable)
{
    if (nTab >= nTable)
        pAttrArray->SetTab(++nTab);
	if( pItems )
		UpdateInsertTabOnlyCells( nTable );
}


void ScColumn::UpdateInsertTabOnlyCells( SCTAB nTable)
{
	if (pItems)
		for (SCSIZE i = 0; i < nCount; i++)
		{
			ScFormulaCell* pCell = (ScFormulaCell*) pItems[i].pCell;
			if( pCell->GetCellType() == CELLTYPE_FORMULA)
			{
				SCROW nRow = pItems[i].nRow;
				pCell->UpdateInsertTab(nTable);
				if ( nRow != pItems[i].nRow )
					Search( nRow, i );		// Listener geloescht/eingefuegt?
			}
		}
}


void ScColumn::UpdateInsertTabAbs(SCTAB nTable)
{
	if (pItems)
		for (SCSIZE i = 0; i < nCount; i++)
		{
			ScFormulaCell* pCell = (ScFormulaCell*) pItems[i].pCell;
			if( pCell->GetCellType() == CELLTYPE_FORMULA)
			{
				SCROW nRow = pItems[i].nRow;
				pCell->UpdateInsertTabAbs(nTable);
				if ( nRow != pItems[i].nRow )
					Search( nRow, i );		// Listener geloescht/eingefuegt?
			}
		}
}


void ScColumn::UpdateDeleteTab( SCTAB nTable, BOOL bIsMove, ScColumn* pRefUndo )
{
	if (nTab > nTable)
		pAttrArray->SetTab(--nTab);

	if (pItems)
		for (SCSIZE i = 0; i < nCount; i++)
			if ( pItems[i].pCell->GetCellType() == CELLTYPE_FORMULA )
			{
				SCROW nRow = pItems[i].nRow;
				ScFormulaCell* pOld = (ScFormulaCell*)pItems[i].pCell;

                /*  Do not copy cell note to the undo document. Undo will copy
                    back the formula cell while keeping the original note. */
                ScBaseCell* pSave = pRefUndo ? pOld->CloneWithoutNote( *pDocument ) : 0;

				BOOL bChanged = pOld->UpdateDeleteTab(nTable, bIsMove);
				if ( nRow != pItems[i].nRow )
					Search( nRow, i );		// Listener geloescht/eingefuegt?

				if (pRefUndo)
				{
					if (bChanged)
						pRefUndo->Insert( nRow, pSave );
					else if(pSave)
						pSave->Delete();
				}
			}
}


void ScColumn::UpdateMoveTab( SCTAB nOldPos, SCTAB nNewPos, SCTAB nTabNo )
{
	nTab = nTabNo;
	pAttrArray->SetTab( nTabNo );
	if (pItems)
		for (SCSIZE i = 0; i < nCount; i++)
		{
			ScFormulaCell* pCell = (ScFormulaCell*) pItems[i].pCell;
			if ( pCell->GetCellType() == CELLTYPE_FORMULA )
			{
				SCROW nRow = pItems[i].nRow;
				pCell->UpdateMoveTab( nOldPos, nNewPos, nTabNo );
				if ( nRow != pItems[i].nRow )
					Search( nRow, i );		// Listener geloescht/eingefuegt?
			}
		}
}


void ScColumn::UpdateCompile( BOOL bForceIfNameInUse )
{
	if (pItems)
		for (SCSIZE i = 0; i < nCount; i++)
		{
			ScFormulaCell* p = (ScFormulaCell*) pItems[i].pCell;
			if( p->GetCellType() == CELLTYPE_FORMULA )
			{
				SCROW nRow = pItems[i].nRow;
				p->UpdateCompile( bForceIfNameInUse );
				if ( nRow != pItems[i].nRow )
					Search( nRow, i );		// Listener geloescht/eingefuegt?
			}
		}
}


void ScColumn::SetTabNo(SCTAB nNewTab)
{
	nTab = nNewTab;
	pAttrArray->SetTab( nNewTab );
	if (pItems)
		for (SCSIZE i = 0; i < nCount; i++)
		{
			ScFormulaCell* p = (ScFormulaCell*) pItems[i].pCell;
			if( p->GetCellType() == CELLTYPE_FORMULA )
				p->aPos.SetTab( nNewTab );
		}
}


BOOL ScColumn::IsRangeNameInUse(SCROW nRow1, SCROW nRow2, USHORT nIndex) const
{
	BOOL bInUse = FALSE;
	if (pItems)
		for (SCSIZE i = 0; !bInUse && (i < nCount); i++)
			if ((pItems[i].nRow >= nRow1) &&
				(pItems[i].nRow <= nRow2) &&
				(pItems[i].pCell->GetCellType() == CELLTYPE_FORMULA))
					bInUse = ((ScFormulaCell*)pItems[i].pCell)->IsRangeNameInUse(nIndex);
	return bInUse;
}

void ScColumn::FindRangeNamesInUse(SCROW nRow1, SCROW nRow2, std::set<USHORT>& rIndexes) const
{
    if (pItems)
        for (SCSIZE i = 0; i < nCount; i++)
            if ((pItems[i].nRow >= nRow1) &&
                (pItems[i].nRow <= nRow2) &&
                (pItems[i].pCell->GetCellType() == CELLTYPE_FORMULA))
                    ((ScFormulaCell*)pItems[i].pCell)->FindRangeNamesInUse(rIndexes);
}

void ScColumn::ReplaceRangeNamesInUse(SCROW nRow1, SCROW nRow2,
                                     const ScRangeData::IndexMap& rMap )
{
    if (pItems)
        for (SCSIZE i = 0; i < nCount; i++)
        {
            if ((pItems[i].nRow >= nRow1) &&
                (pItems[i].nRow <= nRow2) &&
                (pItems[i].pCell->GetCellType() == CELLTYPE_FORMULA))
            {
                SCROW nRow = pItems[i].nRow;
                ((ScFormulaCell*)pItems[i].pCell)->ReplaceRangeNamesInUse( rMap );
                if ( nRow != pItems[i].nRow )
                    Search( nRow, i );      // Listener geloescht/eingefuegt?
            }
        }
}

void ScColumn::SetDirtyVar()
{
	for (SCSIZE i=0; i<nCount; i++)
	{
		ScFormulaCell* p = (ScFormulaCell*) pItems[i].pCell;
		if( p->GetCellType() == CELLTYPE_FORMULA )
			p->SetDirtyVar();
	}
}


void ScColumn::SetDirty()
{
	// wird nur dokumentweit verwendet, kein FormulaTrack
	BOOL bOldAutoCalc = pDocument->GetAutoCalc();
	pDocument->SetAutoCalc( FALSE );	// Mehrfachberechnungen vermeiden
	for (SCSIZE i=0; i<nCount; i++)
	{
		ScFormulaCell* p = (ScFormulaCell*) pItems[i].pCell;
		if( p->GetCellType() == CELLTYPE_FORMULA )
		{
			p->SetDirtyVar();
			if ( !pDocument->IsInFormulaTree( p ) )
				pDocument->PutInFormulaTree( p );
		}
	}
	pDocument->SetAutoCalc( bOldAutoCalc );
}


void ScColumn::SetDirty( const ScRange& rRange )
{	// broadcastet alles innerhalb eines Range, mit FormulaTrack
	if ( !pItems || !nCount )
		return ;
	BOOL bOldAutoCalc = pDocument->GetAutoCalc();
	pDocument->SetAutoCalc( FALSE );	// Mehrfachberechnungen vermeiden
	SCROW nRow2 = rRange.aEnd.Row();
	ScAddress aPos( nCol, 0, nTab );
    ScHint aHint( SC_HINT_DATACHANGED, aPos, NULL );
	SCROW nRow;
	SCSIZE nIndex;
	Search( rRange.aStart.Row(), nIndex );
	while ( nIndex < nCount && (nRow = pItems[nIndex].nRow) <= nRow2 )
	{
		ScBaseCell* pCell = pItems[nIndex].pCell;
		if ( pCell->GetCellType() == CELLTYPE_FORMULA )
			((ScFormulaCell*)pCell)->SetDirty();
		else
		{
			aHint.GetAddress().SetRow( nRow );
            aHint.SetCell( pCell );
			pDocument->Broadcast( aHint );
		}
		nIndex++;
	}
	pDocument->SetAutoCalc( bOldAutoCalc );
}


void ScColumn::SetTableOpDirty( const ScRange& rRange )
{
	if ( !pItems || !nCount )
		return ;
	BOOL bOldAutoCalc = pDocument->GetAutoCalc();
	pDocument->SetAutoCalc( FALSE );	// no multiple recalculation
	SCROW nRow2 = rRange.aEnd.Row();
	ScAddress aPos( nCol, 0, nTab );
    ScHint aHint( SC_HINT_TABLEOPDIRTY, aPos, NULL );
	SCROW nRow;
	SCSIZE nIndex;
	Search( rRange.aStart.Row(), nIndex );
	while ( nIndex < nCount && (nRow = pItems[nIndex].nRow) <= nRow2 )
	{
		ScBaseCell* pCell = pItems[nIndex].pCell;
		if ( pCell->GetCellType() == CELLTYPE_FORMULA )
			((ScFormulaCell*)pCell)->SetTableOpDirty();
		else
		{
			aHint.GetAddress().SetRow( nRow );
            aHint.SetCell( pCell );
			pDocument->Broadcast( aHint );
		}
		nIndex++;
	}
	pDocument->SetAutoCalc( bOldAutoCalc );
}


void ScColumn::SetDirtyAfterLoad()
{
	BOOL bOldAutoCalc = pDocument->GetAutoCalc();
	pDocument->SetAutoCalc( FALSE );	// Mehrfachberechnungen vermeiden
	for (SCSIZE i=0; i<nCount; i++)
	{
		ScFormulaCell* p = (ScFormulaCell*) pItems[i].pCell;
#if 1
        // Simply set dirty and append to FormulaTree, without broadcasting,
        // which is a magnitude faster. This is used to calculate the entire
        // document, e.g. when loading alien file formats.
        if ( p->GetCellType() == CELLTYPE_FORMULA )
            p->SetDirtyAfterLoad();
#else
/* This was used with the binary file format that stored results, where only
 * newly compiled and volatile functions and their dependents had to be
 * recalculated, which was faster then. Since that was moved to 'binfilter' to
 * convert to an XML file this isn't needed anymore, and not used for other
 * file formats. Kept for reference in case mechanism needs to be reactivated
 * for some file formats, we'd have to introduce a controlling parameter to
 * this method here then.
*/

        // If the cell was alsready dirty because of CalcAfterLoad,
        // FormulaTracking has to take place.
        if ( p->GetCellType() == CELLTYPE_FORMULA && p->GetDirty() )
            p->SetDirty();
#endif
	}
	pDocument->SetAutoCalc( bOldAutoCalc );
}


void ScColumn::SetRelNameDirty()
{
	BOOL bOldAutoCalc = pDocument->GetAutoCalc();
	pDocument->SetAutoCalc( FALSE );	// Mehrfachberechnungen vermeiden
	for (SCSIZE i=0; i<nCount; i++)
	{
		ScFormulaCell* p = (ScFormulaCell*) pItems[i].pCell;
		if( p->GetCellType() == CELLTYPE_FORMULA && p->HasRelNameReference() )
			p->SetDirty();
	}
	pDocument->SetAutoCalc( bOldAutoCalc );
}


void ScColumn::CalcAll()
{
	if (pItems)
		for (SCSIZE i=0; i<nCount; i++)
		{
			ScBaseCell* pCell = pItems[i].pCell;
			if (pCell->GetCellType() == CELLTYPE_FORMULA)
			{
#if OSL_DEBUG_LEVEL > 1
				// nach F9 ctrl-F9: ueberprueft die Berechnung per FormulaTree
				ScFormulaCell* pFCell = (ScFormulaCell*)pCell;
				double nOldVal, nNewVal;
				nOldVal = pFCell->GetValue();
#endif
				((ScFormulaCell*)pCell)->Interpret();
#if OSL_DEBUG_LEVEL > 1
				if ( pFCell->GetCode()->IsRecalcModeNormal() )
					nNewVal = pFCell->GetValue();
				else
					nNewVal = nOldVal;	// random(), jetzt() etc.
				DBG_ASSERT( nOldVal==nNewVal, "CalcAll: nOldVal != nNewVal" );
#endif
			}
		}
}


void ScColumn::CompileAll()
{
	if (pItems)
		for (SCSIZE i = 0; i < nCount; i++)
		{
			ScBaseCell* pCell = pItems[i].pCell;
			if ( pCell->GetCellType() == CELLTYPE_FORMULA )
			{
				SCROW nRow = pItems[i].nRow;
				// fuer unbedingtes kompilieren
				// bCompile=TRUE und pCode->nError=0
				((ScFormulaCell*)pCell)->GetCode()->SetCodeError( 0 );
				((ScFormulaCell*)pCell)->SetCompile( TRUE );
				((ScFormulaCell*)pCell)->CompileTokenArray();
				if ( nRow != pItems[i].nRow )
					Search( nRow, i );		// Listener geloescht/eingefuegt?
			}
		}
}


void ScColumn::CompileXML( ScProgress& rProgress )
{
	if (pItems)
		for (SCSIZE i = 0; i < nCount; i++)
		{
			ScBaseCell* pCell = pItems[i].pCell;
			if ( pCell->GetCellType() == CELLTYPE_FORMULA )
			{
				SCROW nRow = pItems[i].nRow;
				((ScFormulaCell*)pCell)->CompileXML( rProgress );
				if ( nRow != pItems[i].nRow )
					Search( nRow, i );		// Listener geloescht/eingefuegt?
			}
		}
}


void ScColumn::CalcAfterLoad()
{
	if (pItems)
		for (SCSIZE i = 0; i < nCount; i++)
		{
			ScBaseCell* pCell = pItems[i].pCell;
			if ( pCell->GetCellType() == CELLTYPE_FORMULA )
				((ScFormulaCell*)pCell)->CalcAfterLoad();
		}
}


bool ScColumn::MarkUsedExternalReferences()
{
    bool bAllMarked = false;
	if (pItems)
    {
		for (SCSIZE i = 0; i < nCount && !bAllMarked; ++i)
		{
			ScBaseCell* pCell = pItems[i].pCell;
			if ( pCell->GetCellType() == CELLTYPE_FORMULA )
				bAllMarked = ((ScFormulaCell*)pCell)->MarkUsedExternalReferences();
		}
    }
    return bAllMarked;
}


void ScColumn::ResetChanged( SCROW nStartRow, SCROW nEndRow )
{
	if (pItems)
	{
		SCSIZE nIndex;
		Search(nStartRow,nIndex);
		while (nIndex<nCount && pItems[nIndex].nRow <= nEndRow)
		{
			ScBaseCell* pCell = pItems[nIndex].pCell;
			if (pCell->GetCellType() == CELLTYPE_FORMULA)
				((ScFormulaCell*)pCell)->ResetChanged();
			++nIndex;
		}
	}
}


BOOL ScColumn::HasEditCells(SCROW nStartRow, SCROW nEndRow, SCROW& rFirst) const
{
	//	used in GetOptimalHeight - ambiguous script type counts as edit cell

    SCROW nRow = 0;
	SCSIZE nIndex;
	Search(nStartRow,nIndex);
	while ( (nIndex < nCount) ? ((nRow=pItems[nIndex].nRow) <= nEndRow) : FALSE )
	{
		ScBaseCell* pCell = pItems[nIndex].pCell;
        CellType eCellType = pCell->GetCellType();
		if ( eCellType == CELLTYPE_EDIT ||
			 IsAmbiguousScriptNonZero( pDocument->GetScriptType(nCol, nRow, nTab, pCell) ) ||
             ((eCellType == CELLTYPE_FORMULA) && ((ScFormulaCell*)pCell)->IsMultilineResult()) )
		{
			rFirst = nRow;
			return TRUE;
		}
		++nIndex;
	}

	return FALSE;
}


SCsROW ScColumn::SearchStyle( SCsROW nRow, const ScStyleSheet* pSearchStyle,
								BOOL bUp, BOOL bInSelection, const ScMarkData& rMark )
{
	if (bInSelection)
	{
		if (rMark.IsMultiMarked())
			return pAttrArray->SearchStyle( nRow, pSearchStyle, bUp,
									(ScMarkArray*) rMark.GetArray()+nCol );		//! const
		else
			return -1;
	}
	else
		return pAttrArray->SearchStyle( nRow, pSearchStyle, bUp, NULL );
}


BOOL ScColumn::SearchStyleRange( SCsROW& rRow, SCsROW& rEndRow, const ScStyleSheet* pSearchStyle,
									BOOL bUp, BOOL bInSelection, const ScMarkData& rMark )
{
	if (bInSelection)
	{
		if (rMark.IsMultiMarked())
			return pAttrArray->SearchStyleRange( rRow, rEndRow, pSearchStyle, bUp,
									(ScMarkArray*) rMark.GetArray()+nCol );		//! const
		else
			return FALSE;
	}
	else
		return pAttrArray->SearchStyleRange( rRow, rEndRow, pSearchStyle, bUp, NULL );
}


