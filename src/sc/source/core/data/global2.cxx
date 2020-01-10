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

#include <sfx2/docfile.hxx>
#include <sfx2/objsh.hxx>
#include <unotools/textsearch.hxx>
#include <svtools/pathoptions.hxx>
#include <svtools/useroptions.hxx>
#include <tools/urlobj.hxx>
#include <unotools/charclass.hxx>
#include <stdlib.h>
#include <ctype.h>
#include <svtools/syslocale.hxx>

#include "global.hxx"
#include "rangeutl.hxx"
#include "pivot.hxx"
#include "rechead.hxx"
#include "compiler.hxx"
#include "paramisc.hxx"

#include "sc.hrc"
#include "globstr.hrc"


// -----------------------------------------------------------------------



#define MAX_LABELS 256 //!!! aus fieldwnd.hxx, muss noch nach global.hxx ???

//------------------------------------------------------------------------
// struct ScImportParam:

ScImportParam::ScImportParam() :
	nCol1(0),
	nRow1(0),
	nCol2(0),
	nRow2(0),
	bImport(FALSE),
	bNative(FALSE),
	bSql(TRUE),
	nType(ScDbTable)
{
}

ScImportParam::ScImportParam( const ScImportParam& r ) :
	nCol1		(r.nCol1),
	nRow1		(r.nRow1),
	nCol2		(r.nCol2),
	nRow2		(r.nRow2),
	bImport		(r.bImport),
	aDBName		(r.aDBName),
	aStatement	(r.aStatement),
	bNative		(r.bNative),
	bSql		(r.bSql),
	nType		(r.nType)
{
}

ScImportParam::~ScImportParam()
{
}

//UNUSED2009-05 void ScImportParam::Clear()
//UNUSED2009-05 {
//UNUSED2009-05     nCol1 = nCol2 = 0;
//UNUSED2009-05     nRow1 = nRow2 = 0;
//UNUSED2009-05     bImport = FALSE;
//UNUSED2009-05     bNative = FALSE;
//UNUSED2009-05     bSql = TRUE;
//UNUSED2009-05     nType = ScDbTable;
//UNUSED2009-05     aDBName.Erase();
//UNUSED2009-05     aStatement.Erase();
//UNUSED2009-05 }

ScImportParam& ScImportParam::operator=( const ScImportParam& r )
{
	nCol1			= r.nCol1;
	nRow1			= r.nRow1;
	nCol2			= r.nCol2;
	nRow2			= r.nRow2;
	bImport			= r.bImport;
	aDBName			= r.aDBName;
	aStatement		= r.aStatement;
	bNative			= r.bNative;
	bSql			= r.bSql;
	nType			= r.nType;

	return *this;
}

BOOL ScImportParam::operator==( const ScImportParam& rOther ) const
{
	return(	nCol1		== rOther.nCol1 &&
			nRow1		== rOther.nRow1 &&
			nCol2		== rOther.nCol2 &&
			nRow2		== rOther.nRow2 &&
			bImport		== rOther.bImport &&
			aDBName		== rOther.aDBName &&
			aStatement	== rOther.aStatement &&
			bNative		== rOther.bNative &&
			bSql		== rOther.bSql &&
			nType		== rOther.nType );

	//!	nQuerySh und pConnection sind gleich ?
}


//------------------------------------------------------------------------
// struct ScQueryParam:

ScQueryEntry::ScQueryEntry()
{
	bDoQuery		= FALSE;
	bQueryByString	= FALSE;
	eOp				= SC_EQUAL;
	eConnect		= SC_AND;
	nField			= 0;
	nVal			= 0.0;
	pStr			= new String;
	pSearchParam	= NULL;
	pSearchText		= NULL;
}

ScQueryEntry::ScQueryEntry(const ScQueryEntry& r)
{
	bDoQuery		= r.bDoQuery;
	bQueryByString	= r.bQueryByString;
	eOp				= r.eOp;
	eConnect		= r.eConnect;
	nField			= r.nField;
	nVal			= r.nVal;
	pStr			= new String(*r.pStr);
	pSearchParam	= NULL;
	pSearchText		= NULL;
}

ScQueryEntry::~ScQueryEntry()
{
	delete pStr;
	if ( pSearchParam )
	{
		delete pSearchParam;
		delete pSearchText;
	}
}

ScQueryEntry& ScQueryEntry::operator=( const ScQueryEntry& r )
{
	bDoQuery		= r.bDoQuery;
	bQueryByString	= r.bQueryByString;
	eOp				= r.eOp;
	eConnect		= r.eConnect;
	nField			= r.nField;
	nVal			= r.nVal;
	*pStr			= *r.pStr;
	if ( pSearchParam )
	{
		delete pSearchParam;
		delete pSearchText;
	}
	pSearchParam	= NULL;
	pSearchText		= NULL;

	return *this;
}

void ScQueryEntry::Clear()
{
	bDoQuery		= FALSE;
	bQueryByString	= FALSE;
	eOp				= SC_EQUAL;
	eConnect		= SC_AND;
	nField			= 0;
	nVal			= 0.0;
	pStr->Erase();
	if ( pSearchParam )
	{
		delete pSearchParam;
		delete pSearchText;
	}
	pSearchParam	= NULL;
	pSearchText		= NULL;
}

BOOL ScQueryEntry::operator==( const ScQueryEntry& r ) const
{
	return bDoQuery			== r.bDoQuery
		&& bQueryByString	== r.bQueryByString
		&& eOp				== r.eOp
		&& eConnect			== r.eConnect
		&& nField			== r.nField
		&& nVal				== r.nVal
		&& *pStr			== *r.pStr;
	//! pSearchParam und pSearchText nicht vergleichen
}

utl::TextSearch* ScQueryEntry::GetSearchTextPtr( BOOL bCaseSens )
{
	if ( !pSearchParam )
	{
		pSearchParam = new utl::SearchParam( *pStr, utl::SearchParam::SRCH_REGEXP,
			bCaseSens, FALSE, FALSE );
		pSearchText = new utl::TextSearch( *pSearchParam, *ScGlobal::pCharClass );
	}
	return pSearchText;
}

//------------------------------------------------------------------------

ScQueryParam::ScQueryParam()
{
	nEntryCount = 0;
	Clear();
}

//------------------------------------------------------------------------

ScQueryParam::ScQueryParam( const ScQueryParam& r ) :
		nCol1(r.nCol1),nRow1(r.nRow1),nCol2(r.nCol2),nRow2(r.nRow2),nTab(r.nTab),
		bHasHeader(r.bHasHeader), bByRow(r.bByRow), bInplace(r.bInplace), bCaseSens(r.bCaseSens),
		bRegExp(r.bRegExp), bMixedComparison(r.bMixedComparison),
        bDuplicate(r.bDuplicate), bDestPers(r.bDestPers),
		nDestTab(r.nDestTab), nDestCol(r.nDestCol), nDestRow(r.nDestRow)
{
	nEntryCount = 0;

	Resize( r.nEntryCount );
	for (USHORT i=0; i<nEntryCount; i++)
		pEntries[i] = r.pEntries[i];
}

//------------------------------------------------------------------------

ScQueryParam::~ScQueryParam()
{
	delete[] pEntries;
}

//------------------------------------------------------------------------

void ScQueryParam::Clear()
{
	nCol1=nCol2=nDestCol = 0;
	nRow1=nRow2=nDestRow = 0;
	nDestTab = 0;
	nTab = SCTAB_MAX;
	bHasHeader = bCaseSens = bRegExp = bMixedComparison = FALSE;
	bInplace = bByRow = bDuplicate = bDestPers = TRUE;

	Resize( MAXQUERY );
	for (USHORT i=0; i<MAXQUERY; i++)
		pEntries[i].Clear();
}

//------------------------------------------------------------------------

ScQueryParam& ScQueryParam::operator=( const ScQueryParam& r )
{
	nCol1		= r.nCol1;
	nRow1		= r.nRow1;
	nCol2		= r.nCol2;
	nRow2		= r.nRow2;
	nTab		= r.nTab;
	nDestTab	= r.nDestTab;
	nDestCol	= r.nDestCol;
	nDestRow	= r.nDestRow;
	bHasHeader	= r.bHasHeader;
	bInplace	= r.bInplace;
	bCaseSens	= r.bCaseSens;
	bRegExp		= r.bRegExp;
    bMixedComparison = r.bMixedComparison;
	bDuplicate	= r.bDuplicate;
	bByRow		= r.bByRow;
	bDestPers	= r.bDestPers;

	Resize( r.nEntryCount );
	for (USHORT i=0; i<nEntryCount; i++)
		pEntries[i] = r.pEntries[i];

	return *this;
}

//------------------------------------------------------------------------

BOOL ScQueryParam::operator==( const ScQueryParam& rOther ) const
{
	BOOL bEqual = FALSE;

	// Anzahl der Queries gleich?
	USHORT nUsed 	  = 0;
	USHORT nOtherUsed = 0;
	while ( nUsed<nEntryCount && pEntries[nUsed].bDoQuery ) ++nUsed;
	while ( nOtherUsed<rOther.nEntryCount && rOther.pEntries[nOtherUsed].bDoQuery )
		++nOtherUsed;

	if (   (nUsed 		== nOtherUsed)
		&& (nCol1		== rOther.nCol1)
		&& (nRow1		== rOther.nRow1)
		&& (nCol2		== rOther.nCol2)
		&& (nRow2		== rOther.nRow2)
		&& (nTab 		== rOther.nTab)
		&& (bHasHeader	== rOther.bHasHeader)
		&& (bByRow		== rOther.bByRow)
		&& (bInplace	== rOther.bInplace)
		&& (bCaseSens	== rOther.bCaseSens)
		&& (bRegExp		== rOther.bRegExp)
        && (bMixedComparison == rOther.bMixedComparison)
		&& (bDuplicate	== rOther.bDuplicate)
		&& (bDestPers   == rOther.bDestPers)
		&& (nDestTab	== rOther.nDestTab)
		&& (nDestCol	== rOther.nDestCol)
		&& (nDestRow	== rOther.nDestRow) )
	{
		bEqual = TRUE;
		for ( USHORT i=0; i<nUsed && bEqual; i++ )
			bEqual = pEntries[i] == rOther.pEntries[i];
	}
	return bEqual;
}

//------------------------------------------------------------------------

void ScQueryParam::DeleteQuery( SCSIZE nPos )
{
	if (nPos<nEntryCount)
	{
		for (SCSIZE i=nPos; i+1<nEntryCount; i++)
			pEntries[i] = pEntries[i+1];

		pEntries[nEntryCount-1].Clear();
	}
	else
	{
		DBG_ERROR("Falscher Parameter bei ScQueryParam::DeleteQuery");
	}
}

//------------------------------------------------------------------------

void ScQueryParam::Resize(SCSIZE nNew)
{
	if ( nNew < MAXQUERY )
		nNew = MAXQUERY;				// nie weniger als MAXQUERY

	ScQueryEntry* pNewEntries = NULL;
	if ( nNew )
		pNewEntries = new ScQueryEntry[nNew];

	SCSIZE nCopy = Min( nEntryCount, nNew );
	for (SCSIZE i=0; i<nCopy; i++)
		pNewEntries[i] = pEntries[i];

	if ( nEntryCount )
		delete[] pEntries;
	nEntryCount = nNew;
	pEntries = pNewEntries;
}

//------------------------------------------------------------------------

void ScQueryParam::MoveToDest()
{
	if (!bInplace)
	{
		SCsCOL nDifX = ((SCsCOL) nDestCol) - ((SCsCOL) nCol1);
		SCsROW nDifY = ((SCsROW) nDestRow) - ((SCsROW) nRow1);
		SCsTAB nDifZ = ((SCsTAB) nDestTab) - ((SCsTAB) nTab);

        nCol1 = sal::static_int_cast<SCCOL>( nCol1 + nDifX );
        nRow1 = sal::static_int_cast<SCROW>( nRow1 + nDifY );
        nCol2 = sal::static_int_cast<SCCOL>( nCol2 + nDifX );
        nRow2 = sal::static_int_cast<SCROW>( nRow2 + nDifY );
        nTab  = sal::static_int_cast<SCTAB>( nTab  + nDifZ );
		for (USHORT i=0; i<nEntryCount; i++)
			pEntries[i].nField += nDifX;

		bInplace = TRUE;
	}
	else
	{
		DBG_ERROR("MoveToDest, bInplace == TRUE");
	}
}

//------------------------------------------------------------------------

void ScQueryParam::FillInExcelSyntax(String& aCellStr, SCSIZE nIndex)
{
	if (aCellStr.Len() > 0)
	{
		if ( nIndex >= nEntryCount )
			Resize( nIndex+1 );

		ScQueryEntry& rEntry = pEntries[nIndex];

		rEntry.bDoQuery = TRUE;
		// Operatoren herausfiltern
		if (aCellStr.GetChar(0) == '<')
		{
			if (aCellStr.GetChar(1) == '>')
			{
				*rEntry.pStr = aCellStr.Copy(2);
				rEntry.eOp   = SC_NOT_EQUAL;
			}
			else if (aCellStr.GetChar(1) == '=')
			{
				*rEntry.pStr = aCellStr.Copy(2);
				rEntry.eOp   = SC_LESS_EQUAL;
			}
			else
			{
				*rEntry.pStr = aCellStr.Copy(1);
				rEntry.eOp   = SC_LESS;
			}
		}
		else if (aCellStr.GetChar(0) == '>')
		{
			if (aCellStr.GetChar(1) == '=')
			{
				*rEntry.pStr = aCellStr.Copy(2);
				rEntry.eOp   = SC_GREATER_EQUAL;
			}
			else
			{
				*rEntry.pStr = aCellStr.Copy(1);
				rEntry.eOp   = SC_GREATER;
			}
		}
		else
		{
			if (aCellStr.GetChar(0) == '=')
				*rEntry.pStr = aCellStr.Copy(1);
			else
				*rEntry.pStr = aCellStr;
			rEntry.eOp = SC_EQUAL;
		}
	}
}

//------------------------------------------------------------------------
// struct ScSubTotalParam:

ScSubTotalParam::ScSubTotalParam()
{
	for ( USHORT i=0; i<MAXSUBTOTAL; i++ )
	{
		nSubTotals[i] = 0;
		pSubTotals[i] = NULL;
		pFunctions[i] = NULL;
	}

	Clear();
}

//------------------------------------------------------------------------

ScSubTotalParam::ScSubTotalParam( const ScSubTotalParam& r ) :
		nCol1(r.nCol1),nRow1(r.nRow1),nCol2(r.nCol2),nRow2(r.nRow2),
		bRemoveOnly(r.bRemoveOnly),bReplace(r.bReplace),bPagebreak(r.bPagebreak),bCaseSens(r.bCaseSens),
		bDoSort(r.bDoSort),bAscending(r.bAscending),bUserDef(r.bUserDef),nUserIndex(r.nUserIndex),
		bIncludePattern(r.bIncludePattern)
{
	for (USHORT i=0; i<MAXSUBTOTAL; i++)
	{
		bGroupActive[i]	= r.bGroupActive[i];
		nField[i]		= r.nField[i];

		if ( (r.nSubTotals[i] > 0) && r.pSubTotals[i] && r.pFunctions[i] )
		{
			nSubTotals[i] = r.nSubTotals[i];
			pSubTotals[i] = new SCCOL	[r.nSubTotals[i]];
			pFunctions[i] = new ScSubTotalFunc	[r.nSubTotals[i]];

			for (SCCOL j=0; j<r.nSubTotals[i]; j++)
			{
				pSubTotals[i][j] = r.pSubTotals[i][j];
				pFunctions[i][j] = r.pFunctions[i][j];
			}
		}
		else
		{
			nSubTotals[i] = 0;
			pSubTotals[i] = NULL;
			pFunctions[i] = NULL;
		}
	}
}

//------------------------------------------------------------------------

void ScSubTotalParam::Clear()
{
	nCol1=nCol2= 0;
	nRow1=nRow2 = 0;
	nUserIndex = 0;
	bPagebreak=bCaseSens=bUserDef=bIncludePattern=bRemoveOnly = FALSE;
	bAscending=bReplace=bDoSort = TRUE;

	for (USHORT i=0; i<MAXSUBTOTAL; i++)
	{
		bGroupActive[i]	= FALSE;
		nField[i]		= 0;

		if ( (nSubTotals[i] > 0) && pSubTotals[i] && pFunctions[i] )
		{
			for ( SCCOL j=0; j<nSubTotals[i]; j++ ) {
				pSubTotals[i][j] = 0;
				pFunctions[i][j] = SUBTOTAL_FUNC_NONE;
			}
		}
	}
}

//------------------------------------------------------------------------

ScSubTotalParam& ScSubTotalParam::operator=( const ScSubTotalParam& r )
{
	nCol1			= r.nCol1;
	nRow1			= r.nRow1;
	nCol2			= r.nCol2;
	nRow2			= r.nRow2;
	bRemoveOnly		= r.bRemoveOnly;
	bReplace		= r.bReplace;
	bPagebreak		= r.bPagebreak;
	bCaseSens		= r.bCaseSens;
	bDoSort			= r.bDoSort;
	bAscending		= r.bAscending;
	bUserDef		= r.bUserDef;
	nUserIndex		= r.nUserIndex;
	bIncludePattern	= r.bIncludePattern;

	for (USHORT i=0; i<MAXSUBTOTAL; i++)
	{
		bGroupActive[i]	= r.bGroupActive[i];
		nField[i]		= r.nField[i];
		nSubTotals[i]	= r.nSubTotals[i];

		if ( pSubTotals[i] ) delete [] pSubTotals[i];
		if ( pFunctions[i] ) delete [] pFunctions[i];

		if ( r.nSubTotals[i] > 0 )
		{
			pSubTotals[i] = new SCCOL	[r.nSubTotals[i]];
			pFunctions[i] = new ScSubTotalFunc	[r.nSubTotals[i]];

			for (SCCOL j=0; j<r.nSubTotals[i]; j++)
			{
				pSubTotals[i][j] = r.pSubTotals[i][j];
				pFunctions[i][j] = r.pFunctions[i][j];
			}
		}
		else
		{
			nSubTotals[i] = 0;
			pSubTotals[i] = NULL;
			pFunctions[i] = NULL;
		}
	}

	return *this;
}

//------------------------------------------------------------------------

BOOL ScSubTotalParam::operator==( const ScSubTotalParam& rOther ) const
{
	BOOL bEqual =   (nCol1			== rOther.nCol1)
				 && (nRow1			== rOther.nRow1)
				 && (nCol2			== rOther.nCol2)
				 && (nRow2			== rOther.nRow2)
				 && (bRemoveOnly	== rOther.bRemoveOnly)
				 && (bReplace		== rOther.bReplace)
				 && (bPagebreak		== rOther.bPagebreak)
				 && (bDoSort		== rOther.bDoSort)
				 && (bCaseSens		== rOther.bCaseSens)
				 && (bAscending		== rOther.bAscending)
				 && (bUserDef		== rOther.bUserDef)
				 && (nUserIndex		== rOther.nUserIndex)
				 && (bIncludePattern== rOther.bIncludePattern);

	if ( bEqual )
	{
		bEqual = TRUE;
		for ( USHORT i=0; i<MAXSUBTOTAL && bEqual; i++ )
		{
			bEqual =   (bGroupActive[i]	== rOther.bGroupActive[i])
					&& (nField[i]		== rOther.nField[i])
					&& (nSubTotals[i]	== rOther.nSubTotals[i]);

			if ( bEqual && (nSubTotals[i] > 0) )
			{
				bEqual = (pSubTotals != NULL) && (pFunctions != NULL);

				for (SCCOL j=0; (j<nSubTotals[i]) && bEqual; j++)
				{
					bEqual =   bEqual
							&& (pSubTotals[i][j] == rOther.pSubTotals[i][j])
							&& (pFunctions[i][j] == rOther.pFunctions[i][j]);
				}
			}
		}
	}

	return bEqual;
}

//------------------------------------------------------------------------

void ScSubTotalParam::SetSubTotals( USHORT					nGroup,
									const SCCOL*			ptrSubTotals,
									const ScSubTotalFunc*	ptrFunctions,
									USHORT					nCount )
{
	DBG_ASSERT( (nGroup <= MAXSUBTOTAL),
				"ScSubTotalParam::SetSubTotals(): nGroup > MAXSUBTOTAL!" );
	DBG_ASSERT( ptrSubTotals,
				"ScSubTotalParam::SetSubTotals(): ptrSubTotals == NULL!" );
	DBG_ASSERT( ptrFunctions,
				"ScSubTotalParam::SetSubTotals(): ptrFunctions == NULL!" );
	DBG_ASSERT( (nCount > 0),
				"ScSubTotalParam::SetSubTotals(): nCount <= 0!" );

	if ( ptrSubTotals && ptrFunctions && (nCount > 0) && (nGroup <= MAXSUBTOTAL) )
	{
		// 0 wird als 1 aufgefasst, sonst zum Array-Index dekrementieren
		if (nGroup != 0)
			nGroup--;

		delete [] pSubTotals[nGroup];
		delete [] pFunctions[nGroup];

		pSubTotals[nGroup] = new SCCOL		[nCount];
		pFunctions[nGroup] = new ScSubTotalFunc	[nCount];
		nSubTotals[nGroup] = static_cast<SCCOL>(nCount);

		for ( USHORT i=0; i<nCount; i++ )
		{
			pSubTotals[nGroup][i] = ptrSubTotals[i];
			pFunctions[nGroup][i] = ptrFunctions[i];
		}
	}
}

// -----------------------------------------------------------------------

PivotField::PivotField( SCsCOL nNewCol, USHORT nNewFuncMask ) :
    nCol( nNewCol ),
    nFuncMask( nNewFuncMask ),
    nFuncCount( 0 )
{
}

bool PivotField::operator==( const PivotField& r ) const
{
    return (nCol                            == r.nCol)
        && (nFuncMask                       == r.nFuncMask)
        && (nFuncCount                      == r.nFuncCount)
        && (maFieldRef.ReferenceType        == r.maFieldRef.ReferenceType)
        && (maFieldRef.ReferenceField       == r.maFieldRef.ReferenceField)
        && (maFieldRef.ReferenceItemType    == r.maFieldRef.ReferenceItemType)
        && (maFieldRef.ReferenceItemName    == r.maFieldRef.ReferenceItemName);
}

//------------------------------------------------------------------------
// struct ScPivotParam:

ScPivotParam::ScPivotParam()
	:	nCol(0), nRow(0), nTab(0),
		ppLabelArr( NULL ), nLabels(0),
        nPageCount(0), nColCount(0), nRowCount(0), nDataCount(0),
		bIgnoreEmptyRows(FALSE), bDetectCategories(FALSE),
		bMakeTotalCol(TRUE), bMakeTotalRow(TRUE)
{
}

//------------------------------------------------------------------------

ScPivotParam::ScPivotParam( const ScPivotParam& r )
	:	nCol( r.nCol ), nRow( r.nRow ), nTab( r.nTab ),
		ppLabelArr( NULL ), nLabels(0),
        nPageCount(0), nColCount(0), nRowCount(0), nDataCount(0),
		bIgnoreEmptyRows(r.bIgnoreEmptyRows),
		bDetectCategories(r.bDetectCategories),
		bMakeTotalCol(r.bMakeTotalCol),
		bMakeTotalRow(r.bMakeTotalRow)
{
	SetLabelData	( r.ppLabelArr, r.nLabels );
    SetPivotArrays  ( r.aPageArr, r.aColArr, r.aRowArr, r.aDataArr,
                      r.nPageCount, r.nColCount, r.nRowCount, r.nDataCount );
}

//------------------------------------------------------------------------

__EXPORT ScPivotParam::~ScPivotParam()
{
	ClearLabelData();
}

//------------------------------------------------------------------------

//UNUSED2009-05 void __EXPORT ScPivotParam::Clear()
//UNUSED2009-05 {
//UNUSED2009-05     nCol = 0;
//UNUSED2009-05     nRow = 0;
//UNUSED2009-05     nTab = 0;
//UNUSED2009-05     bIgnoreEmptyRows = bDetectCategories = FALSE;
//UNUSED2009-05     bMakeTotalCol = bMakeTotalRow = TRUE;
//UNUSED2009-05     ClearLabelData();
//UNUSED2009-05     ClearPivotArrays();
//UNUSED2009-05 }

//------------------------------------------------------------------------

void __EXPORT ScPivotParam::ClearLabelData()
{
	if ( (nLabels > 0) && ppLabelArr )
	{
		for ( SCSIZE i=0; i<nLabels; i++ )
			delete ppLabelArr[i];
		delete [] ppLabelArr;
		ppLabelArr = NULL;
		nLabels = 0;
	}
}

//------------------------------------------------------------------------

void __EXPORT ScPivotParam::ClearPivotArrays()
{
    memset( aPageArr, 0, PIVOT_MAXPAGEFIELD * sizeof(PivotField) );
	memset( aColArr, 0, PIVOT_MAXFIELD * sizeof(PivotField) );
	memset( aRowArr, 0, PIVOT_MAXFIELD * sizeof(PivotField) );
	memset( aDataArr, 0, PIVOT_MAXFIELD * sizeof(PivotField) );
    nPageCount = 0;
	nColCount = 0;
	nRowCount = 0;
	nDataCount = 0;
}

//------------------------------------------------------------------------

void __EXPORT ScPivotParam::SetLabelData( LabelData**	pLabArr,
										  SCSIZE		nLab )
{
	ClearLabelData();

	if ( (nLab > 0) && pLabArr )
	{
		nLabels = (nLab>MAX_LABELS) ? MAX_LABELS : nLab;
		ppLabelArr = new LabelData*[nLabels];
		for ( SCSIZE i=0; i<nLabels; i++ )
			ppLabelArr[i] = new LabelData( *(pLabArr[i]) );
	}
}

//------------------------------------------------------------------------

void __EXPORT ScPivotParam::SetPivotArrays  ( const PivotField* pPageArr,
                                              const PivotField* pColArr,
											  const PivotField*	pRowArr,
											  const PivotField*	pDataArr,
                                              SCSIZE            nPageCnt,
											  SCSIZE			nColCnt,
											  SCSIZE			nRowCnt,
											  SCSIZE			nDataCnt )
{
	ClearPivotArrays();

    if ( pPageArr && pColArr && pRowArr && pDataArr  )
	{
        nPageCount  = (nPageCnt>PIVOT_MAXPAGEFIELD) ? PIVOT_MAXPAGEFIELD : nPageCnt;
		nColCount	= (nColCnt>PIVOT_MAXFIELD) ? PIVOT_MAXFIELD : nColCnt;
		nRowCount	= (nRowCnt>PIVOT_MAXFIELD) ? PIVOT_MAXFIELD : nRowCnt;
		nDataCount	= (nDataCnt>PIVOT_MAXFIELD) ? PIVOT_MAXFIELD : nDataCnt;

        memcpy( aPageArr, pPageArr, nPageCount * sizeof(PivotField) );
		memcpy( aColArr,  pColArr,	nColCount  * sizeof(PivotField) );
		memcpy( aRowArr,  pRowArr,	nRowCount  * sizeof(PivotField) );
		memcpy( aDataArr, pDataArr, nDataCount * sizeof(PivotField) );
	}
}

//------------------------------------------------------------------------

ScPivotParam& __EXPORT ScPivotParam::operator=( const ScPivotParam& r )
{
	nCol			  = r.nCol;
	nRow			  = r.nRow;
	nTab			  = r.nTab;
	bIgnoreEmptyRows  = r.bIgnoreEmptyRows;
	bDetectCategories = r.bDetectCategories;
	bMakeTotalCol	  = r.bMakeTotalCol;
	bMakeTotalRow	  = r.bMakeTotalRow;

	SetLabelData	( r.ppLabelArr, r.nLabels );
    SetPivotArrays  ( r.aPageArr, r.aColArr, r.aRowArr, r.aDataArr,
                      r.nPageCount, r.nColCount, r.nRowCount, r.nDataCount );

	return *this;
}

//------------------------------------------------------------------------

BOOL __EXPORT ScPivotParam::operator==( const ScPivotParam& r ) const
{
	BOOL bEqual = 	(nCol		== r.nCol)
				 &&	(nRow		== r.nRow)
				 && (nTab		== r.nTab)
				 && (bIgnoreEmptyRows  == r.bIgnoreEmptyRows)
				 && (bDetectCategories == r.bDetectCategories)
				 && (bMakeTotalCol == r.bMakeTotalCol)
				 && (bMakeTotalRow == r.bMakeTotalRow)
				 && (nLabels 	== r.nLabels)
                 && (nPageCount == r.nPageCount)
				 && (nColCount	== r.nColCount)
				 && (nRowCount	== r.nRowCount)
				 && (nDataCount	== r.nDataCount);

	if ( bEqual )
	{
		SCSIZE i;

        for ( i=0; i<nPageCount && bEqual; i++ )
            bEqual = ( aPageArr[i] == r.aPageArr[i] );

		for ( i=0; i<nColCount && bEqual; i++ )
			bEqual = ( aColArr[i] == r.aColArr[i] );

		for ( i=0; i<nRowCount && bEqual; i++ )
			bEqual = ( aRowArr[i] == r.aRowArr[i] );

		for ( i=0; i<nDataCount && bEqual; i++ )
			bEqual = ( aDataArr[i] == r.aDataArr[i] );
	}

	return bEqual;
}

//------------------------------------------------------------------------
// struct ScSolveParam

ScSolveParam::ScSolveParam()
	:	pStrTargetVal( NULL )
{
}

//------------------------------------------------------------------------

ScSolveParam::ScSolveParam( const ScSolveParam& r )
	:	aRefFormulaCell	( r.aRefFormulaCell ),
		aRefVariableCell( r.aRefVariableCell ),
		pStrTargetVal	( r.pStrTargetVal
							? new String(*r.pStrTargetVal)
							: NULL )
{
}

//------------------------------------------------------------------------

ScSolveParam::ScSolveParam( const ScAddress& rFormulaCell,
							const ScAddress& rVariableCell,
							const String& 	rTargetValStr )
	:	aRefFormulaCell	( rFormulaCell ),
		aRefVariableCell( rVariableCell ),
		pStrTargetVal	( new String(rTargetValStr) )
{
}

//------------------------------------------------------------------------

ScSolveParam::~ScSolveParam()
{
	delete pStrTargetVal;
}

//------------------------------------------------------------------------

ScSolveParam& __EXPORT ScSolveParam::operator=( const ScSolveParam& r )
{
	delete pStrTargetVal;

	aRefFormulaCell  = r.aRefFormulaCell;
	aRefVariableCell = r.aRefVariableCell;
	pStrTargetVal    = r.pStrTargetVal
							? new String(*r.pStrTargetVal)
							: NULL;
	return *this;
}

//------------------------------------------------------------------------

BOOL ScSolveParam::operator==( const ScSolveParam& r ) const
{
	BOOL bEqual = 	(aRefFormulaCell  == r.aRefFormulaCell)
				 &&	(aRefVariableCell == r.aRefVariableCell);

	if ( bEqual )
	{
		if ( !pStrTargetVal && !r.pStrTargetVal )
			bEqual = TRUE;
		else if ( !pStrTargetVal || !r.pStrTargetVal )
			bEqual = FALSE;
		else if ( pStrTargetVal && r.pStrTargetVal )
			bEqual = ( *pStrTargetVal == *(r.pStrTargetVal) );
	}

	return bEqual;
}


//------------------------------------------------------------------------
// struct ScTabOpParam

ScTabOpParam::ScTabOpParam( const ScTabOpParam& r )
	:	aRefFormulaCell	( r.aRefFormulaCell ),
		aRefFormulaEnd	( r.aRefFormulaEnd ),
		aRefRowCell		( r.aRefRowCell ),
		aRefColCell		( r.aRefColCell ),
		nMode			( r.nMode )
{
}

//------------------------------------------------------------------------

ScTabOpParam::ScTabOpParam( const ScRefAddress& rFormulaCell,
							const ScRefAddress& rFormulaEnd,
							const ScRefAddress& rRowCell,
							const ScRefAddress& rColCell,
								  BYTE		 nMd)
	:	aRefFormulaCell	( rFormulaCell ),
		aRefFormulaEnd	( rFormulaEnd ),
		aRefRowCell		( rRowCell ),
		aRefColCell		( rColCell ),
		nMode			( nMd )
{
}

//------------------------------------------------------------------------

ScTabOpParam& ScTabOpParam::operator=( const ScTabOpParam& r )
{
	aRefFormulaCell  = r.aRefFormulaCell;
	aRefFormulaEnd   = r.aRefFormulaEnd;
	aRefRowCell 	 = r.aRefRowCell;
	aRefColCell 	 = r.aRefColCell;
	nMode		     = r.nMode;
	return *this;
}

//------------------------------------------------------------------------

BOOL __EXPORT ScTabOpParam::operator==( const ScTabOpParam& r ) const
{
	return (		(aRefFormulaCell == r.aRefFormulaCell)
				 &&	(aRefFormulaEnd	 == r.aRefFormulaEnd)
				 &&	(aRefRowCell	 == r.aRefRowCell)
				 &&	(aRefColCell	 == r.aRefColCell)
				 && (nMode 			 == r.nMode) );
}

String ScGlobal::GetAbsDocName( const String& rFileName,
								SfxObjectShell* pShell )
{
	String aAbsName;
	if ( !pShell->HasName() )
	{	// maybe relative to document path working directory
		INetURLObject aObj;
		SvtPathOptions aPathOpt;
		aObj.SetSmartURL( aPathOpt.GetWorkPath() );
		aObj.setFinalSlash();		// it IS a path
		bool bWasAbs = true;
		aAbsName = aObj.smartRel2Abs( rFileName, bWasAbs ).GetMainURL(INetURLObject::NO_DECODE);
		//	returned string must be encoded because it's used directly to create SfxMedium
	}
	else
	{
		const SfxMedium* pMedium = pShell->GetMedium();
		if ( pMedium )
		{
			bool bWasAbs = true;
			aAbsName = pMedium->GetURLObject().smartRel2Abs( rFileName, bWasAbs ).GetMainURL(INetURLObject::NO_DECODE);
		}
		else
		{	// This can't happen, but ...
			// just to be sure to have the same encoding
			INetURLObject aObj;
			aObj.SetSmartURL( aAbsName );
			aAbsName = aObj.GetMainURL(INetURLObject::NO_DECODE);
		}
	}
	return aAbsName;
}


String ScGlobal::GetDocTabName( const String& rFileName,
								const String& rTabName )
{
	String aDocTab( '\'' );
	aDocTab += rFileName;
	xub_StrLen nPos = 1;
	while( (nPos = aDocTab.Search( '\'', nPos ))
			!= STRING_NOTFOUND )
	{	// escape Quotes
		aDocTab.Insert( '\\', nPos );
		nPos += 2;
	}
	aDocTab += '\'';
	aDocTab += SC_COMPILER_FILE_TAB_SEP;
	aDocTab += rTabName;  	// "'Doc'#Tab"
	return aDocTab;
}

// ============================================================================

ScSimpleSharedString::StringTable::StringTable() :
    mnStrCount(0)
{
    // empty string (ID = 0)
    maSharedStrings.push_back(String());
    maSharedStringIds.insert( SharedStrMap::value_type(String(), mnStrCount++) );
}

ScSimpleSharedString::StringTable::StringTable(const ScSimpleSharedString::StringTable& r) :
    maSharedStrings(r.maSharedStrings),
    maSharedStringIds(r.maSharedStringIds),
    mnStrCount(r.mnStrCount)
{
}

ScSimpleSharedString::StringTable::~StringTable()
{
}

sal_Int32 ScSimpleSharedString::StringTable::insertString(const String& aStr)
{
    SharedStrMap::const_iterator itr = maSharedStringIds.find(aStr),
        itrEnd = maSharedStringIds.end();

    if (itr == itrEnd)
    {
        // new string.
        maSharedStrings.push_back(aStr);
        maSharedStringIds.insert( SharedStrMap::value_type(aStr, mnStrCount) );
        return mnStrCount++;
    }

    // existing string.
    return itr->second;
}

sal_Int32 ScSimpleSharedString::StringTable::getStringId(const String& aStr)
{
    SharedStrMap::const_iterator itr = maSharedStringIds.find(aStr),
        itrEnd = maSharedStringIds.end();
    if (itr == itrEnd)
    {
        // string not found.
        return insertString(aStr);
    }
    return itr->second;
}

const String* ScSimpleSharedString::StringTable::getString(sal_Int32 nId) const
{
    if (nId >= mnStrCount)
        return NULL;

    return &maSharedStrings[nId];
}

// ----------------------------------------------------------------------------

ScSimpleSharedString::ScSimpleSharedString()
{
}

ScSimpleSharedString::ScSimpleSharedString(const ScSimpleSharedString& r) :
    maStringTable(r.maStringTable)
{
}

ScSimpleSharedString::~ScSimpleSharedString()
{
}

sal_Int32 ScSimpleSharedString::insertString(const String& aStr)
{
    return maStringTable.insertString(aStr);
}

const String* ScSimpleSharedString::getString(sal_Int32 nId)
{
    return maStringTable.getString(nId);
}

sal_Int32 ScSimpleSharedString::getStringId(const String& aStr)
{
    return maStringTable.getStringId(aStr);
}
