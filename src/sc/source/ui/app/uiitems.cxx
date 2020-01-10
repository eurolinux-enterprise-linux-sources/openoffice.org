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



#include <svx/editobj.hxx>

#include "userlist.hxx"
#include "uiitems.hxx"
#include "dpsave.hxx"

// STATIC DATA -----------------------------------------------------------

TYPEINIT1(ScInputStatusItem,    SfxPoolItem);
TYPEINIT1(ScSortItem,           SfxPoolItem);
TYPEINIT1(ScQueryItem,          SfxPoolItem);
TYPEINIT1(ScSubTotalItem,       SfxPoolItem);
TYPEINIT1(ScUserListItem,       SfxPoolItem);
TYPEINIT1(ScPivotItem,		    SfxPoolItem);
TYPEINIT1(ScSolveItem,		    SfxPoolItem);
TYPEINIT1(ScTabOpItem,		    SfxPoolItem);
TYPEINIT1(ScCondFrmtItem,	    SfxPoolItem);

TYPEINIT1(ScTablesHint,			SfxHint);
TYPEINIT1(ScEditViewHint,		SfxHint);
TYPEINIT1(ScIndexHint,			SfxHint);

// -----------------------------------------------------------------------
//      ScInputStatusItem - Status-Update fuer Eingabezeile
// -----------------------------------------------------------------------

//UNUSED2008-05  ScInputStatusItem::ScInputStatusItem( USHORT nWhichP,
//UNUSED2008-05                                        SCTAB nTab,
//UNUSED2008-05                                        SCCOL nCol, SCROW nRow,
//UNUSED2008-05                                        SCCOL nStartCol, SCROW nStartRow,
//UNUSED2008-05                                        SCCOL nEndCol,   SCROW nEndRow,
//UNUSED2008-05                                        const String& rString, const EditTextObject* pData )
//UNUSED2008-05  
//UNUSED2008-05      :   SfxPoolItem ( nWhichP ),
//UNUSED2008-05          aCursorPos  ( nCol, nRow, nTab ),
//UNUSED2008-05          aStartPos   ( nStartCol, nStartRow, nTab ),
//UNUSED2008-05          aEndPos     ( nEndCol,   nEndRow,   nTab ),
//UNUSED2008-05          aString     ( rString ),
//UNUSED2008-05          pEditData   ( pData ? pData->Clone() : NULL )
//UNUSED2008-05  {
//UNUSED2008-05  }

ScInputStatusItem::ScInputStatusItem( USHORT nWhichP,
									  const ScAddress& rCurPos,
									  const ScAddress& rStartPos,
									  const ScAddress& rEndPos,
									  const String& rString,
									  const EditTextObject* pData )
    :   SfxPoolItem ( nWhichP ),
		aCursorPos	( rCurPos ),
		aStartPos	( rStartPos ),
		aEndPos		( rEndPos ),
		aString		( rString ),
		pEditData	( pData ? pData->Clone() : NULL )
{
}

ScInputStatusItem::ScInputStatusItem( const ScInputStatusItem& rItem )
	:	SfxPoolItem	( rItem ),
		aCursorPos	( rItem.aCursorPos ),
		aStartPos	( rItem.aStartPos ),
		aEndPos		( rItem.aEndPos ),
		aString		( rItem.aString ),
		pEditData	( rItem.pEditData ? rItem.pEditData->Clone() : NULL )
{
}

__EXPORT ScInputStatusItem::~ScInputStatusItem()
{
	delete pEditData;
}

String __EXPORT ScInputStatusItem::GetValueText() const
{
	return String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("InputStatus"));
}

int __EXPORT ScInputStatusItem::operator==( const SfxPoolItem& rItem ) const
{
	DBG_ASSERT( SfxPoolItem::operator==( rItem ), "unequal Which or Type" );

	return (    (aStartPos 	== ((ScInputStatusItem&)rItem).aStartPos)
			 &&	(aEndPos   	== ((ScInputStatusItem&)rItem).aEndPos)
			 &&	(aCursorPos == ((ScInputStatusItem&)rItem).aCursorPos)
			 && (aString   	== ((ScInputStatusItem&)rItem).aString) );
														//!	Edit-Daten vergleichen!
}

SfxPoolItem* __EXPORT ScInputStatusItem::Clone( SfxItemPool * ) const
{
	return new ScInputStatusItem( *this );
}

//
//	ScPaintHint ist nach schints.cxx verschoben
//

// -----------------------------------------------------------------------
//	ScTablesHint - Views anpassen, wenn Tabellen eingefuegt / geloescht
// -----------------------------------------------------------------------

ScTablesHint::ScTablesHint(USHORT nNewId, SCTAB nTable1, SCTAB nTable2) :
	nId( nNewId ),
	nTab1( nTable1 ),
	nTab2( nTable2 )
{
}

ScTablesHint::~ScTablesHint()
{
}


// -----------------------------------------------------------------------
//	ScIndexHint
// -----------------------------------------------------------------------

ScIndexHint::ScIndexHint(USHORT nNewId, USHORT nIdx) :
	nId( nNewId ),
	nIndex( nIdx )
{
}

ScIndexHint::~ScIndexHint()
{
}


// -----------------------------------------------------------------------
//      ScEditViewHint - neue EditView fuer Cursorposition anlegen
// -----------------------------------------------------------------------

ScEditViewHint::ScEditViewHint( ScEditEngineDefaulter* pEngine, const ScAddress& rCurPos ) :
    pEditEngine( pEngine ),
    aCursorPos( rCurPos )
{
}

ScEditViewHint::~ScEditViewHint()
{
}

// -----------------------------------------------------------------------
//      ScSortItem - Daten fuer den Sortierdialog
// -----------------------------------------------------------------------

ScSortItem::ScSortItem( USHORT              nWhichP,
						ScViewData*         ptrViewData,
						const ScSortParam*  pSortData ) :
        SfxPoolItem ( nWhichP ),
		pViewData   ( ptrViewData )
{
	if ( pSortData ) theSortData = *pSortData;
}

//------------------------------------------------------------------------

ScSortItem::ScSortItem( USHORT              nWhichP,
						const ScSortParam*  pSortData ) :
        SfxPoolItem ( nWhichP ),
		pViewData   ( NULL )
{
	if ( pSortData ) theSortData = *pSortData;
}

//------------------------------------------------------------------------

ScSortItem::ScSortItem( const ScSortItem& rItem ) :
		SfxPoolItem ( rItem ),
		pViewData   ( rItem.pViewData ),
		theSortData ( rItem.theSortData )
{
}

__EXPORT ScSortItem::~ScSortItem()
{
}

//------------------------------------------------------------------------

String __EXPORT ScSortItem::GetValueText() const
{
	return String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("SortItem"));
}

//------------------------------------------------------------------------

int __EXPORT ScSortItem::operator==( const SfxPoolItem& rItem ) const
{
	DBG_ASSERT( SfxPoolItem::operator==( rItem ), "unequal Which or Type" );

	const ScSortItem& rOther = (const ScSortItem&)rItem;

	return (   (pViewData   == rOther.pViewData)
			&& (theSortData == rOther.theSortData) );
}

//------------------------------------------------------------------------

SfxPoolItem* __EXPORT ScSortItem::Clone( SfxItemPool * ) const
{
	return new ScSortItem( *this );
}

//------------------------------------------------------------------------

sal_Bool ScSortItem::QueryValue( com::sun::star::uno::Any& rVal, BYTE /* nMemberUd */ ) const
{
    // Return empty value as there is no useful conversion
    rVal = com::sun::star::uno::Any();
    return sal_True;
}

// -----------------------------------------------------------------------
//      ScQueryItem - Daten fuer den Filterdialog
// -----------------------------------------------------------------------

ScQueryItem::ScQueryItem( USHORT                nWhichP,
						  ScViewData*           ptrViewData,
						  const ScQueryParam*   pQueryData ) :
        SfxPoolItem ( nWhichP ),
        pViewData   ( ptrViewData ),
        bIsAdvanced	( FALSE )
{
	if ( pQueryData ) theQueryData = *pQueryData;
}

//------------------------------------------------------------------------

ScQueryItem::ScQueryItem( USHORT                nWhichP,
						  const ScQueryParam*   pQueryData ) :
        SfxPoolItem ( nWhichP ),
        pViewData   ( NULL ),
        bIsAdvanced	( FALSE )
{
	if ( pQueryData ) theQueryData = *pQueryData;
}

//------------------------------------------------------------------------

ScQueryItem::ScQueryItem( const ScQueryItem& rItem ) :
		SfxPoolItem ( rItem ),
		pViewData   ( rItem.pViewData ),
		theQueryData( rItem.theQueryData ),
		bIsAdvanced	( rItem.bIsAdvanced ),
		aAdvSource	( rItem.aAdvSource )
{
}

__EXPORT ScQueryItem::~ScQueryItem()
{
}

//------------------------------------------------------------------------

void ScQueryItem::SetAdvancedQuerySource(const ScRange* pSource)
{
	if (pSource)
	{
		aAdvSource = *pSource;
		bIsAdvanced = TRUE;
	}
	else
		bIsAdvanced = FALSE;
}

BOOL ScQueryItem::GetAdvancedQuerySource(ScRange& rSource) const
{
	rSource = aAdvSource;
	return bIsAdvanced;
}

//------------------------------------------------------------------------

String __EXPORT ScQueryItem::GetValueText() const
{
	return String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("QueryItem"));
}

//------------------------------------------------------------------------

int __EXPORT ScQueryItem::operator==( const SfxPoolItem& rItem ) const
{
	DBG_ASSERT( SfxPoolItem::operator==( rItem ), "unequal Which or Type" );

	const ScQueryItem& rQueryItem = (const ScQueryItem&)rItem;

	return (   (pViewData    == rQueryItem.pViewData)
			&& (bIsAdvanced  == rQueryItem.bIsAdvanced)
			&& (aAdvSource   == rQueryItem.aAdvSource)
			&& (theQueryData == rQueryItem.theQueryData) );
}

//------------------------------------------------------------------------

SfxPoolItem* __EXPORT ScQueryItem::Clone( SfxItemPool * ) const
{
	return new ScQueryItem( *this );
}

// -----------------------------------------------------------------------
//      ScSubTotalItem - Daten fuer den Zwischenergebnisdialog
// -----------------------------------------------------------------------

ScSubTotalItem::ScSubTotalItem( USHORT                  nWhichP,
								ScViewData*           	ptrViewData,
								const ScSubTotalParam*	pSubTotalData ) :
        SfxPoolItem ( nWhichP ),
		pViewData   ( ptrViewData )
{
	if ( pSubTotalData ) theSubTotalData = *pSubTotalData;
}

//------------------------------------------------------------------------

ScSubTotalItem::ScSubTotalItem( USHORT                  nWhichP,
								const ScSubTotalParam*	pSubTotalData ) :
        SfxPoolItem ( nWhichP ),
		pViewData   ( NULL )
{
	if ( pSubTotalData ) theSubTotalData = *pSubTotalData;
}

//------------------------------------------------------------------------

ScSubTotalItem::ScSubTotalItem( const ScSubTotalItem& rItem ) :
		SfxPoolItem		( rItem ),
		pViewData		( rItem.pViewData ),
		theSubTotalData	( rItem.theSubTotalData )
{
}

__EXPORT ScSubTotalItem::~ScSubTotalItem()
{
}

//------------------------------------------------------------------------

String __EXPORT ScSubTotalItem::GetValueText() const
{
	return String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("SubTotalItem"));
}

//------------------------------------------------------------------------

int __EXPORT ScSubTotalItem::operator==( const SfxPoolItem& rItem ) const
{
	DBG_ASSERT( SfxPoolItem::operator==( rItem ), "unequal Which or Type" );

	const ScSubTotalItem& rSTItem = (const ScSubTotalItem&)rItem;

	return (   (pViewData   	== rSTItem.pViewData)
			&& (theSubTotalData == rSTItem.theSubTotalData) );
}

//------------------------------------------------------------------------

SfxPoolItem* __EXPORT ScSubTotalItem::Clone( SfxItemPool * ) const
{
	return new ScSubTotalItem( *this );
}

//------------------------------------------------------------------------

sal_Bool ScSubTotalItem::QueryValue( com::sun::star::uno::Any& rVal, BYTE /* nMemberUd */ ) const
{
    // Return empty value as there is no useful conversion
    rVal = com::sun::star::uno::Any();
    return sal_True;
}

// -----------------------------------------------------------------------
//      ScUserListItem - Transporter fuer den Benutzerlisten-TabPage
// -----------------------------------------------------------------------

ScUserListItem::ScUserListItem( USHORT nWhichP )
    :   SfxPoolItem ( nWhichP ),
		pUserList	( NULL )
{
}

//------------------------------------------------------------------------

ScUserListItem::ScUserListItem( const ScUserListItem& rItem )
	:	SfxPoolItem	( rItem )
{
	if ( rItem.pUserList )
		pUserList = new ScUserList( *(rItem.pUserList) );
	else
		pUserList = NULL;
}

__EXPORT ScUserListItem::~ScUserListItem()
{
	delete pUserList;
}

//------------------------------------------------------------------------

String __EXPORT ScUserListItem::GetValueText() const
{
	return String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("ScUserListItem"));
}

//------------------------------------------------------------------------

int __EXPORT ScUserListItem::operator==( const SfxPoolItem& rItem ) const
{
	DBG_ASSERT( SfxPoolItem::operator==( rItem ), "unequal Which or Type" );

	const ScUserListItem& r = (const ScUserListItem&)rItem;
	BOOL bEqual = FALSE;

	if ( !pUserList || !(r.pUserList) )
		bEqual = ( !pUserList && !(r.pUserList) );
	else
		bEqual = ( *pUserList == *(r.pUserList) );

	return bEqual;
}


//------------------------------------------------------------------------

SfxPoolItem* __EXPORT ScUserListItem::Clone( SfxItemPool * ) const
{
	return new ScUserListItem( *this );
}

//------------------------------------------------------------------------

void ScUserListItem::SetUserList( const ScUserList& rUserList )
{
	delete pUserList;
	pUserList = new ScUserList( rUserList );
}

// -----------------------------------------------------------------------
//      ScPivotItem - Daten fuer den Pivot-Dialog
// -----------------------------------------------------------------------

ScPivotItem::ScPivotItem( USHORT nWhichP, const ScDPSaveData* pData,
							 const ScRange* pRange, BOOL bNew ) :
        SfxPoolItem ( nWhichP )
{
	//	pSaveData must always exist
	if ( pData )
		pSaveData = new ScDPSaveData(*pData);
	else
		pSaveData = new ScDPSaveData;
	if ( pRange ) aDestRange = *pRange;
	bNewSheet = bNew;
}

//------------------------------------------------------------------------

ScPivotItem::ScPivotItem( const ScPivotItem& rItem ) :
		SfxPoolItem	( rItem ),
		aDestRange	( rItem.aDestRange ),
		bNewSheet	( rItem.bNewSheet )
{
	DBG_ASSERT(rItem.pSaveData, "pSaveData");
	pSaveData = new ScDPSaveData(*rItem.pSaveData);
}

//------------------------------------------------------------------------

__EXPORT ScPivotItem::~ScPivotItem()
{
	delete pSaveData;
}

//------------------------------------------------------------------------

String __EXPORT ScPivotItem::GetValueText() const
{
	return String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("ScPivotItem"));
}

//------------------------------------------------------------------------

int __EXPORT ScPivotItem::operator==( const SfxPoolItem& rItem ) const
{
	DBG_ASSERT( SfxPoolItem::operator==( rItem ), "unequal Which or Type" );

	const ScPivotItem& rPItem = (const ScPivotItem&)rItem;
	DBG_ASSERT( pSaveData && rPItem.pSaveData, "pSaveData" );
	return ( *pSaveData == *rPItem.pSaveData &&
			 aDestRange == rPItem.aDestRange &&
			 bNewSheet  == rPItem.bNewSheet );
}

//------------------------------------------------------------------------

SfxPoolItem* __EXPORT ScPivotItem::Clone( SfxItemPool * ) const
{
	return new ScPivotItem( *this );
}


// -----------------------------------------------------------------------
//      ScSolveItem - Daten fuer den Solver-Dialog
// -----------------------------------------------------------------------

ScSolveItem::ScSolveItem( USHORT                nWhichP,
						  const ScSolveParam*	pSolveData )
    :   SfxPoolItem ( nWhichP )
{
	if ( pSolveData ) theSolveData = *pSolveData;
}

//------------------------------------------------------------------------

ScSolveItem::ScSolveItem( const ScSolveItem& rItem )
	:   SfxPoolItem		( rItem ),
		theSolveData	( rItem.theSolveData )
{
}

//------------------------------------------------------------------------

__EXPORT ScSolveItem::~ScSolveItem()
{
}

//------------------------------------------------------------------------

String __EXPORT ScSolveItem::GetValueText() const
{
	return String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("ScSolveItem"));
}

//------------------------------------------------------------------------

int __EXPORT ScSolveItem::operator==( const SfxPoolItem& rItem ) const
{
	DBG_ASSERT( SfxPoolItem::operator==( rItem ), "unequal Which or Type" );

	const ScSolveItem& rPItem = (const ScSolveItem&)rItem;

	return ( theSolveData == rPItem.theSolveData );
}

//------------------------------------------------------------------------

SfxPoolItem* __EXPORT ScSolveItem::Clone( SfxItemPool * ) const
{
	return new ScSolveItem( *this );
}

// -----------------------------------------------------------------------
//      ScTabOpItem - Daten fuer den TabOp-Dialog
// -----------------------------------------------------------------------

ScTabOpItem::ScTabOpItem( USHORT                nWhichP,
						  const ScTabOpParam*	pTabOpData )
    :   SfxPoolItem ( nWhichP )
{
	if ( pTabOpData ) theTabOpData = *pTabOpData;
}

//------------------------------------------------------------------------

ScTabOpItem::ScTabOpItem( const ScTabOpItem& rItem )
	:   SfxPoolItem		( rItem ),
		theTabOpData	( rItem.theTabOpData )
{
}

//------------------------------------------------------------------------

__EXPORT ScTabOpItem::~ScTabOpItem()
{
}

//------------------------------------------------------------------------

String __EXPORT ScTabOpItem::GetValueText() const
{
	return String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("ScTabOpItem"));
}

//------------------------------------------------------------------------

int __EXPORT ScTabOpItem::operator==( const SfxPoolItem& rItem ) const
{
	DBG_ASSERT( SfxPoolItem::operator==( rItem ), "unequal Which or Type" );

	const ScTabOpItem& rPItem = (const ScTabOpItem&)rItem;

	return ( theTabOpData == rPItem.theTabOpData );
}

//------------------------------------------------------------------------

SfxPoolItem* __EXPORT ScTabOpItem::Clone( SfxItemPool * ) const
{
	return new ScTabOpItem( *this );
}


// -----------------------------------------------------------------------
//      ScCondFrmtItem - Daten fuer den Dialog bedingte Formatierung
// -----------------------------------------------------------------------

ScCondFrmtItem::ScCondFrmtItem( USHORT nWhichP,
//!								const ScConditionalFormat* pCondFrmt )
								const ScConditionalFormat& rCondFrmt )
    :   SfxPoolItem ( nWhichP ),
		theCondFrmtData ( rCondFrmt )	//!
{
//!	if ( pCondFrmt ) theCondFrmtData = *pCondFrmt;
}

//------------------------------------------------------------------------

ScCondFrmtItem::ScCondFrmtItem( const ScCondFrmtItem& rItem )
	:   SfxPoolItem		( rItem ),
		theCondFrmtData ( rItem.theCondFrmtData )
{
}

//------------------------------------------------------------------------

__EXPORT ScCondFrmtItem::~ScCondFrmtItem()
{
}

//------------------------------------------------------------------------

String __EXPORT ScCondFrmtItem::GetValueText() const
{
	return String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("ScCondFrmtItem"));
}

//------------------------------------------------------------------------

int __EXPORT ScCondFrmtItem::operator==( const SfxPoolItem& rItem ) const
{
	DBG_ASSERT( SfxPoolItem::operator==( rItem ), "unequal Which or Type" );

	const ScCondFrmtItem& rPItem = (const ScCondFrmtItem&)rItem;

	return ( theCondFrmtData == rPItem.theCondFrmtData );
}

//------------------------------------------------------------------------

SfxPoolItem* __EXPORT ScCondFrmtItem::Clone( SfxItemPool * ) const
{
	return new ScCondFrmtItem( *this );
}
