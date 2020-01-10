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
/*
	WICHTIG:
	Folgende Reihenfolge beim Aufbau der Pivot-Tabelle unbedingt einzuhalten:

	pPivot->SetColFields(aColArr, aColCount)
	pPivot->SetRowFields(aRowArr, aRowCount)
	pPivot->SetDataFields(aDataArr, aDataCount)
	if (pPivot->CreateData())
	{
		pPivotDrawData();
		pPivotReleaseData();
	}

	ausserdem ist sicherzustellen, dass entweder das ColArr oder das RowArr
	einen PivotDataField Eintrag enthalten

*/


#ifndef SC_PIVOT_HXX
#define SC_PIVOT_HXX

#include "global.hxx"
#include "address.hxx"

#include <vector>

class SubTotal;
#include "collect.hxx"

#define PIVOT_DATA_FIELD		(MAXCOLCOUNT)
#define PIVOT_FUNC_REF			(MAXCOLCOUNT)
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/sheet/DataPilotFieldReference.hpp>
#include <com/sun/star/sheet/DataPilotFieldSortInfo.hpp>
#include <com/sun/star/sheet/DataPilotFieldLayoutInfo.hpp>
#include <com/sun/star/sheet/DataPilotFieldAutoShowInfo.hpp>

class SvStream;
class ScDocument;
class ScUserListData;
class ScProgress;
struct LabelData;

// -----------------------------------------------------------------------

struct PivotField
{
    SCsCOL               nCol;
    USHORT              nFuncMask;
    USHORT              nFuncCount;
    ::com::sun::star::sheet::DataPilotFieldReference maFieldRef;

    explicit            PivotField( SCsCOL nNewCol = 0, USHORT nNewFuncMask = PIVOT_FUNC_NONE );

    bool                operator==( const PivotField& r ) const;
};

// -----------------------------------------------------------------------

// implementation still in global2.cxx
struct ScPivotParam
{
    SCCOL           nCol;           // Cursor Position /
    SCROW           nRow;           // bzw. Anfang des Zielbereiches
    SCTAB           nTab;
    LabelData**     ppLabelArr;
    SCSIZE          nLabels;
    PivotField      aPageArr[PIVOT_MAXPAGEFIELD];
    PivotField      aColArr[PIVOT_MAXFIELD];
    PivotField      aRowArr[PIVOT_MAXFIELD];
    PivotField      aDataArr[PIVOT_MAXFIELD];
    SCSIZE          nPageCount;
    SCSIZE          nColCount;
    SCSIZE          nRowCount;
    SCSIZE          nDataCount;
    BOOL            bIgnoreEmptyRows;
    BOOL            bDetectCategories;
    BOOL            bMakeTotalCol;
    BOOL            bMakeTotalRow;

    ScPivotParam();
    ScPivotParam( const ScPivotParam& r );
    ~ScPivotParam();

    ScPivotParam&   operator=       ( const ScPivotParam& r );
    BOOL            operator==      ( const ScPivotParam& r ) const;
//UNUSED2009-05 void            Clear           ();
    void            ClearLabelData  ();
    void            ClearPivotArrays();
    void            SetLabelData    ( LabelData**   ppLabArr,
                                      SCSIZE        nLab );
    void            SetPivotArrays  ( const PivotField* pPageArr,
                                      const PivotField* pColArr,
                                      const PivotField* pRowArr,
                                      const PivotField* pDataArr,
                                      SCSIZE            nPageCnt,
                                      SCSIZE            nColCnt,
                                      SCSIZE            nRowCnt,
                                      SCSIZE            nDataCnt );
};

// -----------------------------------------------------------------------

typedef PivotField			PivotFieldArr[PIVOT_MAXFIELD];
typedef PivotField          PivotPageFieldArr[PIVOT_MAXPAGEFIELD];

//------------------------------------------------------------------------

struct LabelData
{
    String              maName;         /// Visible name of the dimension.
    SCsCOL              mnCol;
    USHORT              mnFuncMask;     /// Page/Column/Row subtotal function.
    sal_Int32           mnUsedHier;     /// Used hierarchy.
    bool                mbShowAll;      /// true = Show all (also empty) results.
    bool                mbIsValue;      /// true = Sum or count in data field.

    ::com::sun::star::uno::Sequence< ::rtl::OUString >  maHiers;        /// Hierarchies.
    ::com::sun::star::uno::Sequence< ::rtl::OUString >  maMembers;      /// Members.
    ::com::sun::star::uno::Sequence< sal_Bool >         maVisible;      /// Visibility of members.
    ::com::sun::star::uno::Sequence< sal_Bool >         maShowDet;      /// Show details of members.
    ::com::sun::star::sheet::DataPilotFieldSortInfo     maSortInfo;     /// Sorting info.
    ::com::sun::star::sheet::DataPilotFieldLayoutInfo   maLayoutInfo;   /// Layout info.
    ::com::sun::star::sheet::DataPilotFieldAutoShowInfo maShowInfo;     /// AutoShow info.

    explicit            LabelData( const String& rName, short nCol, bool bIsValue );
};

// ============================================================================

struct ScDPFuncData
{
    short               mnCol;
    USHORT              mnFuncMask;
    ::com::sun::star::sheet::DataPilotFieldReference maFieldRef;

    explicit            ScDPFuncData( short nNewCol, USHORT nNewFuncMask );
    explicit            ScDPFuncData( short nNewCol, USHORT nNewFuncMask,
                            const ::com::sun::star::sheet::DataPilotFieldReference& rFieldRef );
};

// ============================================================================

typedef LabelData ScDPLabelData;
typedef std::vector< ScDPLabelData > ScDPLabelDataVec;
typedef std::vector< String > ScDPNameVec;

// ============================================================================

#endif
