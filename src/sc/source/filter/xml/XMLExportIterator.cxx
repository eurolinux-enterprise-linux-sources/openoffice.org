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
#include "XMLExportIterator.hxx"
#include <com/sun/star/text/XSimpleText.hpp>
#include <com/sun/star/sheet/XCellAddressable.hpp>
#include <com/sun/star/sheet/CellFlags.hpp>
#include <com/sun/star/sheet/XSheetAnnotationsSupplier.hpp>
#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <tools/debug.hxx>
#include <xmloff/xmlnmspe.hxx>
#include "dociter.hxx"
#include "convuno.hxx"
#include "xmlexprt.hxx"
#include "XMLExportSharedData.hxx"
#include "XMLStylesExportHelper.hxx"
#include "document.hxx"

#include <algorithm>

using ::rtl::OUString;
using namespace ::com::sun::star;

//==============================================================================

ScMyIteratorBase::ScMyIteratorBase()
{
}

ScMyIteratorBase::~ScMyIteratorBase()
{
}

void ScMyIteratorBase::UpdateAddress( table::CellAddress& rCellAddress )
{
	table::CellAddress aNewAddr( rCellAddress );
	if( GetFirstAddress( aNewAddr ) )
	{
		if( (aNewAddr.Sheet == rCellAddress.Sheet) &&
			((aNewAddr.Row < rCellAddress.Row) ||
			((aNewAddr.Row == rCellAddress.Row) && (aNewAddr.Column < rCellAddress.Column))) )
			rCellAddress = aNewAddr;
	}
}


//==============================================================================

sal_Bool ScMyShape::operator<(const ScMyShape& aShape) const
{
	if( aAddress.Tab() != aShape.aAddress.Tab() )
		return (aAddress.Tab() < aShape.aAddress.Tab());
	else if( aAddress.Row() != aShape.aAddress.Row() )
		return (aAddress.Row() < aShape.aAddress.Row());
	else
		return (aAddress.Col() < aShape.aAddress.Col());
}

ScMyShapesContainer::ScMyShapesContainer()
	: aShapeList()
{
}

ScMyShapesContainer::~ScMyShapesContainer()
{
}

void ScMyShapesContainer::AddNewShape( const ScMyShape& aShape )
{
	aShapeList.push_back(aShape);
}

sal_Bool ScMyShapesContainer::GetFirstAddress( table::CellAddress& rCellAddress )
{
	sal_Int32 nTable(rCellAddress.Sheet);
	if( !aShapeList.empty() )
	{
		ScUnoConversion::FillApiAddress( rCellAddress, aShapeList.begin()->aAddress );
		return (nTable == rCellAddress.Sheet);
	}
	return sal_False;
}

void ScMyShapesContainer::SetCellData( ScMyCell& rMyCell )
{
	rMyCell.aShapeList.clear();
	ScAddress aAddress;
	ScUnoConversion::FillScAddress( aAddress, rMyCell.aCellAddress );

	ScMyShapeList::iterator aItr(aShapeList.begin());
	ScMyShapeList::iterator aEndItr(aShapeList.end());
	while( (aItr != aEndItr) && (aItr->aAddress == aAddress) )
	{
		rMyCell.aShapeList.push_back(*aItr);
		aItr = aShapeList.erase(aItr);
	}
	rMyCell.bHasShape = !rMyCell.aShapeList.empty();
}

void ScMyShapesContainer::SkipTable(SCTAB nSkip)
{
    ScMyShapeList::iterator aItr = aShapeList.begin();
    while( (aItr != aShapeList.end()) && (aItr->aAddress.Tab() == nSkip) )
        aItr = aShapeList.erase(aItr);
}

void ScMyShapesContainer::Sort()
{
	aShapeList.sort();
}

sal_Bool ScMyNoteShape::operator<(const ScMyNoteShape& aNote) const
{
	if( aPos.Tab() != aNote.aPos.Tab() )
		return (aPos.Tab() < aNote.aPos.Tab());
	else if( aPos.Row() != aNote.aPos.Row() )
		return (aPos.Row() < aNote.aPos.Row());
	else
		return (aPos.Col() < aNote.aPos.Col());
}

ScMyNoteShapesContainer::ScMyNoteShapesContainer()
	: aNoteShapeList()
{
}

ScMyNoteShapesContainer::~ScMyNoteShapesContainer()
{
}

void ScMyNoteShapesContainer::AddNewNote( const ScMyNoteShape& aNote )
{
	aNoteShapeList.push_back(aNote);
}

sal_Bool ScMyNoteShapesContainer::GetFirstAddress( table::CellAddress& rCellAddress )
{
	sal_Int16 nTable = rCellAddress.Sheet;
	if( !aNoteShapeList.empty() )
	{
		ScUnoConversion::FillApiAddress( rCellAddress, aNoteShapeList.begin()->aPos );
		return (nTable == rCellAddress.Sheet);
	}
	return sal_False;
}

void ScMyNoteShapesContainer::SetCellData( ScMyCell& rMyCell )
{
    rMyCell.xNoteShape.clear();
	ScAddress aAddress;
	ScUnoConversion::FillScAddress( aAddress, rMyCell.aCellAddress );

	ScMyNoteShapeList::iterator aItr = aNoteShapeList.begin();
	while( (aItr != aNoteShapeList.end()) && (aItr->aPos == aAddress) )
	{
		rMyCell.xNoteShape = aItr->xShape;
		aItr = aNoteShapeList.erase(aItr);
	}
}

void ScMyNoteShapesContainer::SkipTable(SCTAB nSkip)
{
    ScMyNoteShapeList::iterator aItr = aNoteShapeList.begin();
    while( (aItr != aNoteShapeList.end()) && (aItr->aPos.Tab() == nSkip) )
        aItr = aNoteShapeList.erase(aItr);
}

void ScMyNoteShapesContainer::Sort()
{
	aNoteShapeList.sort();
}

//==============================================================================

sal_Bool ScMyMergedRange::operator<(const ScMyMergedRange& aRange) const
{
	if( aCellRange.Sheet != aRange.aCellRange.Sheet )
		return (aCellRange.Sheet < aRange.aCellRange.Sheet);
	else if( aCellRange.StartRow != aRange.aCellRange.StartRow )
		return (aCellRange.StartRow < aRange.aCellRange.StartRow);
	else
		return (aCellRange.StartColumn < aRange.aCellRange.StartColumn);
}


ScMyMergedRangesContainer::ScMyMergedRangesContainer()
	: aRangeList()
{
}

ScMyMergedRangesContainer::~ScMyMergedRangesContainer()
{
}

void ScMyMergedRangesContainer::AddRange(const table::CellRangeAddress aMergedRange)
{
	sal_Int32 nStartRow(aMergedRange.StartRow);
	sal_Int32 nEndRow(aMergedRange.EndRow);

	ScMyMergedRange aRange;
	aRange.bIsFirst = sal_True;
	aRange.aCellRange = aMergedRange;
	aRange.aCellRange.EndRow = nStartRow;
	aRange.nRows = nEndRow - nStartRow + 1;
	aRangeList.push_back( aRange );

	aRange.bIsFirst = sal_False;
	aRange.nRows = 0;
	for( sal_Int32 nRow = nStartRow + 1; nRow <= nEndRow; ++nRow )
	{
		aRange.aCellRange.StartRow = aRange.aCellRange.EndRow = nRow;
		aRangeList.push_back(aRange);
	}
}

sal_Bool ScMyMergedRangesContainer::GetFirstAddress( table::CellAddress& rCellAddress )
{
	sal_Int32 nTable(rCellAddress.Sheet);
	if( !aRangeList.empty() )
	{
		ScUnoConversion::FillApiStartAddress( rCellAddress, aRangeList.begin()->aCellRange );
		return (nTable == rCellAddress.Sheet);
	}
	return sal_False;
}

void ScMyMergedRangesContainer::SetCellData( ScMyCell& rMyCell )
{
	rMyCell.bIsMergedBase = rMyCell.bIsCovered = sal_False;
	ScMyMergedRangeList::iterator aItr(aRangeList.begin());
	if( aItr != aRangeList.end() )
	{
		table::CellAddress aFirstAddress;
		ScUnoConversion::FillApiStartAddress( aFirstAddress, aItr->aCellRange );
		if( aFirstAddress == rMyCell.aCellAddress )
		{
			rMyCell.aMergeRange = aItr->aCellRange;
			if (aItr->bIsFirst)
				rMyCell.aMergeRange.EndRow = rMyCell.aMergeRange.StartRow + aItr->nRows - 1;
			rMyCell.bIsMergedBase = aItr->bIsFirst;
			rMyCell.bIsCovered = !aItr->bIsFirst;
			if( aItr->aCellRange.StartColumn < aItr->aCellRange.EndColumn )
			{
				++(aItr->aCellRange.StartColumn);
				aItr->bIsFirst = sal_False;
			}
			else
				aRangeList.erase(aItr);
		}
	}
}

void ScMyMergedRangesContainer::SkipTable(SCTAB nSkip)
{
    ScMyMergedRangeList::iterator aItr = aRangeList.begin();
    while( (aItr != aRangeList.end()) && (aItr->aCellRange.Sheet == nSkip) )
        aItr = aRangeList.erase(aItr);
}

void ScMyMergedRangesContainer::Sort()
{
	aRangeList.sort();
}

//==============================================================================

sal_Bool ScMyAreaLink::Compare( const ScMyAreaLink& rAreaLink ) const
{
	return	(GetRowCount() == rAreaLink.GetRowCount()) &&
			(sFilter == rAreaLink.sFilter) &&
			(sFilterOptions == rAreaLink.sFilterOptions) &&
			(sURL == rAreaLink.sURL) &&
			(sSourceStr == rAreaLink.sSourceStr);
}

sal_Bool ScMyAreaLink::operator<(const ScMyAreaLink& rAreaLink ) const
{
	if( aDestRange.Sheet != rAreaLink.aDestRange.Sheet )
		return (aDestRange.Sheet < rAreaLink.aDestRange.Sheet);
	else if( aDestRange.StartRow != rAreaLink.aDestRange.StartRow )
		return (aDestRange.StartRow < rAreaLink.aDestRange.StartRow);
	else
		return (aDestRange.StartColumn < rAreaLink.aDestRange.StartColumn);
}

ScMyAreaLinksContainer::ScMyAreaLinksContainer() :
	aAreaLinkList()
{
}

ScMyAreaLinksContainer::~ScMyAreaLinksContainer()
{
}

sal_Bool ScMyAreaLinksContainer::GetFirstAddress( table::CellAddress& rCellAddress )
{
	sal_Int32 nTable(rCellAddress.Sheet);
	if( !aAreaLinkList.empty() )
	{
		ScUnoConversion::FillApiStartAddress( rCellAddress, aAreaLinkList.begin()->aDestRange );
		return (nTable == rCellAddress.Sheet);
	}
	return sal_False;
}

void ScMyAreaLinksContainer::SetCellData( ScMyCell& rMyCell )
{
	rMyCell.bHasAreaLink = sal_False;
	ScMyAreaLinkList::iterator aItr(aAreaLinkList.begin());
	if( aItr != aAreaLinkList.end() )
	{
		table::CellAddress aAddress;
		ScUnoConversion::FillApiStartAddress( aAddress, aItr->aDestRange );
		if( aAddress == rMyCell.aCellAddress )
		{
			rMyCell.bHasAreaLink = sal_True;
			rMyCell.aAreaLink = *aItr;
			aItr = aAreaLinkList.erase( aItr );
            sal_Bool bFound = sal_True;
            while (aItr != aAreaLinkList.end() && bFound)
            {
        		ScUnoConversion::FillApiStartAddress( aAddress, aItr->aDestRange );
                if (aAddress == rMyCell.aCellAddress)
                {
                    DBG_ERROR("more than one linked range on one cell");
                    aItr = aAreaLinkList.erase( aItr );
                }
                else
                    bFound = sal_False;
            }
		}
	}
}

void ScMyAreaLinksContainer::SkipTable(SCTAB nSkip)
{
    ScMyAreaLinkList::iterator aItr = aAreaLinkList.begin();
    while( (aItr != aAreaLinkList.end()) && (aItr->aDestRange.Sheet == nSkip) )
        aItr = aAreaLinkList.erase(aItr);
}

void ScMyAreaLinksContainer::Sort()
{
	aAreaLinkList.sort();
}

//==============================================================================

ScMyCellRangeAddress::ScMyCellRangeAddress(const table::CellRangeAddress& rRange)
	: table::CellRangeAddress(rRange)
{
}

sal_Bool ScMyCellRangeAddress::operator<(const ScMyCellRangeAddress& rRange ) const
{
	if( Sheet != rRange.Sheet )
		return (Sheet < rRange.Sheet);
	else if( StartRow != rRange.StartRow )
		return (StartRow < rRange.StartRow);
	else
		return (StartColumn < rRange.StartColumn);
}

ScMyEmptyDatabaseRangesContainer::ScMyEmptyDatabaseRangesContainer()
	: aDatabaseList()
{
}

ScMyEmptyDatabaseRangesContainer::~ScMyEmptyDatabaseRangesContainer()
{
}

void ScMyEmptyDatabaseRangesContainer::AddNewEmptyDatabaseRange(const table::CellRangeAddress& aCellRange)
{
	sal_Int32 nStartRow(aCellRange.StartRow);
	sal_Int32 nEndRow(aCellRange.EndRow);
	ScMyCellRangeAddress aRange( aCellRange );
	for( sal_Int32 nRow = nStartRow; nRow <= nEndRow; ++nRow )
	{
		aRange.StartRow = aRange.EndRow = nRow;
		aDatabaseList.push_back( aRange );
	}
}

sal_Bool ScMyEmptyDatabaseRangesContainer::GetFirstAddress( table::CellAddress& rCellAddress )
{
	sal_Int32 nTable(rCellAddress.Sheet);
	if( !aDatabaseList.empty() )
	{
		ScUnoConversion::FillApiStartAddress( rCellAddress, *(aDatabaseList.begin()) );
		return (nTable == rCellAddress.Sheet);
	}
	return sal_False;
}

void ScMyEmptyDatabaseRangesContainer::SetCellData( ScMyCell& rMyCell )
{
	rMyCell.bHasEmptyDatabase = sal_False;
	ScMyEmptyDatabaseRangeList::iterator aItr(aDatabaseList.begin());
	if( aItr != aDatabaseList.end() )
	{
		table::CellAddress aFirstAddress;
		ScUnoConversion::FillApiStartAddress( aFirstAddress, *aItr );
		if( aFirstAddress == rMyCell.aCellAddress )
		{
			rMyCell.bHasEmptyDatabase = sal_True;
			if( aItr->StartColumn < aItr->EndColumn )
				++(aItr->StartColumn);
			else
				aDatabaseList.erase(aItr);
		}
	}
}

void ScMyEmptyDatabaseRangesContainer::SkipTable(SCTAB nSkip)
{
    ScMyEmptyDatabaseRangeList::iterator aItr = aDatabaseList.begin();
    while( (aItr != aDatabaseList.end()) && (aItr->Sheet == nSkip) )
        aItr = aDatabaseList.erase(aItr);
}

void ScMyEmptyDatabaseRangesContainer::Sort()
{
	aDatabaseList.sort();
}

//==============================================================================

sal_Bool ScMyDetectiveObj::operator<( const ScMyDetectiveObj& rDetObj) const
{
	if( aPosition.Sheet != rDetObj.aPosition.Sheet )
		return (aPosition.Sheet < rDetObj.aPosition.Sheet);
	else if( aPosition.Row != rDetObj.aPosition.Row )
		return (aPosition.Row < rDetObj.aPosition.Row);
	else
		return (aPosition.Column < rDetObj.aPosition.Column);
}

ScMyDetectiveObjContainer::ScMyDetectiveObjContainer() :
	aDetectiveObjList()
{
}

ScMyDetectiveObjContainer::~ScMyDetectiveObjContainer()
{
}

void ScMyDetectiveObjContainer::AddObject( ScDetectiveObjType eObjType, const SCTAB nSheet,
                                            const ScAddress& rPosition, const ScRange& rSourceRange,
                                            sal_Bool bHasError )
{
	if( (eObjType == SC_DETOBJ_ARROW) ||
		(eObjType == SC_DETOBJ_FROMOTHERTAB) ||
		(eObjType == SC_DETOBJ_TOOTHERTAB) ||
		(eObjType == SC_DETOBJ_CIRCLE) )
	{
		ScMyDetectiveObj aDetObj;
		aDetObj.eObjType = eObjType;
		if( eObjType == SC_DETOBJ_TOOTHERTAB )
			ScUnoConversion::FillApiAddress( aDetObj.aPosition, rSourceRange.aStart );
		else
			ScUnoConversion::FillApiAddress( aDetObj.aPosition, rPosition );
		ScUnoConversion::FillApiRange( aDetObj.aSourceRange, rSourceRange );

        // #111064#; take the sheet where the object is found and not the sheet given in the ranges, because they are not always true
        if (eObjType != SC_DETOBJ_FROMOTHERTAB)
        {
            // if the ObjType == SC_DETOBJ_FROMOTHERTAB then the SourceRange is not used and so it has not to be tested and changed
            DBG_ASSERT(aDetObj.aPosition.Sheet == aDetObj.aSourceRange.Sheet, "It seems to be possible to have different sheets");
            aDetObj.aSourceRange.Sheet = nSheet;
        }
        aDetObj.aPosition.Sheet = nSheet;

		aDetObj.bHasError = bHasError;
		aDetectiveObjList.push_back( aDetObj );
	}
}

sal_Bool ScMyDetectiveObjContainer::GetFirstAddress( table::CellAddress& rCellAddress )
{
	sal_Int32 nTable(rCellAddress.Sheet);
	if( !aDetectiveObjList.empty() )
	{
		rCellAddress = aDetectiveObjList.begin()->aPosition;
		return (nTable == rCellAddress.Sheet);
	}
	return sal_False;
}

void ScMyDetectiveObjContainer::SetCellData( ScMyCell& rMyCell )
{
	rMyCell.aDetectiveObjVec.clear();
	ScMyDetectiveObjList::iterator aItr(aDetectiveObjList.begin());
	ScMyDetectiveObjList::iterator aEndItr(aDetectiveObjList.end());
	while( (aItr != aEndItr) && (aItr->aPosition == rMyCell.aCellAddress) )
	{
		rMyCell.aDetectiveObjVec.push_back( *aItr );
		aItr = aDetectiveObjList.erase( aItr );
	}
	rMyCell.bHasDetectiveObj = (rMyCell.aDetectiveObjVec.size() != 0);
}

void ScMyDetectiveObjContainer::SkipTable(SCTAB nSkip)
{
    ScMyDetectiveObjList::iterator aItr = aDetectiveObjList.begin();
    while( (aItr != aDetectiveObjList.end()) && (aItr->aPosition.Sheet == nSkip) )
        aItr = aDetectiveObjList.erase(aItr);
}

void ScMyDetectiveObjContainer::Sort()
{
	aDetectiveObjList.sort();
}

//==============================================================================

sal_Bool ScMyDetectiveOp::operator<( const ScMyDetectiveOp& rDetOp) const
{
	if( aPosition.Sheet != rDetOp.aPosition.Sheet )
		return (aPosition.Sheet < rDetOp.aPosition.Sheet);
	else if( aPosition.Row != rDetOp.aPosition.Row )
		return (aPosition.Row < rDetOp.aPosition.Row);
	else
		return (aPosition.Column < rDetOp.aPosition.Column);
}

ScMyDetectiveOpContainer::ScMyDetectiveOpContainer() :
	aDetectiveOpList()
{
}

ScMyDetectiveOpContainer::~ScMyDetectiveOpContainer()
{
}

void ScMyDetectiveOpContainer::AddOperation( ScDetOpType eOpType, const ScAddress& rPosition, sal_uInt32 nIndex )
{
	ScMyDetectiveOp aDetOp;
	aDetOp.eOpType = eOpType;
	ScUnoConversion::FillApiAddress( aDetOp.aPosition, rPosition );
	aDetOp.nIndex = nIndex;
	aDetectiveOpList.push_back( aDetOp );
}

sal_Bool ScMyDetectiveOpContainer::GetFirstAddress( table::CellAddress& rCellAddress )
{
	sal_Int32 nTable(rCellAddress.Sheet);
	if( !aDetectiveOpList.empty() )
	{
		rCellAddress = aDetectiveOpList.begin()->aPosition;
		return (nTable == rCellAddress.Sheet);
	}
	return sal_False;
}

void ScMyDetectiveOpContainer::SetCellData( ScMyCell& rMyCell )
{
	rMyCell.aDetectiveOpVec.clear();
	ScMyDetectiveOpList::iterator aItr(aDetectiveOpList.begin());
	ScMyDetectiveOpList::iterator aEndItr(aDetectiveOpList.end());
	while( (aItr != aEndItr) && (aItr->aPosition == rMyCell.aCellAddress) )
	{
		rMyCell.aDetectiveOpVec.push_back( *aItr );
		aItr = aDetectiveOpList.erase( aItr );
	}
	rMyCell.bHasDetectiveOp = (rMyCell.aDetectiveOpVec.size() != 0);
}

void ScMyDetectiveOpContainer::SkipTable(SCTAB nSkip)
{
    ScMyDetectiveOpList::iterator aItr = aDetectiveOpList.begin();
    while( (aItr != aDetectiveOpList.end()) && (aItr->aPosition.Sheet == nSkip) )
        aItr = aDetectiveOpList.erase(aItr);
}

void ScMyDetectiveOpContainer::Sort()
{
	aDetectiveOpList.sort();
}

//==============================================================================

ScMyCell::ScMyCell() :
	aShapeList(),
	aDetectiveObjVec(),
    nValidationIndex(-1),
    pBaseCell(NULL),
	bIsAutoStyle( sal_False ),
	bHasShape( sal_False ),
	bIsMergedBase( sal_False ),
	bIsCovered( sal_False ),
	bHasAreaLink( sal_False ),
	bHasEmptyDatabase( sal_False ),
	bHasDetectiveObj( sal_False ),
	bHasDetectiveOp( sal_False ),
	bIsEditCell( sal_False ),
	bKnowWhetherIsEditCell( sal_False ),
	bHasStringValue( sal_False ),
	bHasDoubleValue( sal_False ),
	bHasXText( sal_False ),
	bIsMatrixBase( sal_False ),
	bIsMatrixCovered( sal_False ),
	bHasAnnotation( sal_False )
{
}

ScMyCell::~ScMyCell()
{
}

//==============================================================================

sal_Bool ScMyExportAnnotation::operator<(const ScMyExportAnnotation& rAnno) const
{
	if( aCellAddress.Row != rAnno.aCellAddress.Row )
		return (aCellAddress.Row < rAnno.aCellAddress.Row);
	else
		return (aCellAddress.Column < rAnno.aCellAddress.Column);
}


ScMyNotEmptyCellsIterator::ScMyNotEmptyCellsIterator(ScXMLExport& rTempXMLExport)
    : pShapes(NULL),
    pNoteShapes(NULL),
	pEmptyDatabaseRanges(NULL),
	pMergedRanges(NULL),
	pAreaLinks(NULL),
	pDetectiveObj(NULL),
	pDetectiveOp(NULL),
    rExport(rTempXMLExport),
	pCellItr(NULL),
	nCurrentTable(SCTAB_MAX)
{
}

ScMyNotEmptyCellsIterator::~ScMyNotEmptyCellsIterator()
{
	Clear();
}

void ScMyNotEmptyCellsIterator::Clear()
{
	if (pCellItr)
		delete pCellItr;
	if (!aAnnotations.empty())
	{
		DBG_ERROR("not all Annotations saved");
		aAnnotations.clear();
	}
	pCellItr = NULL;
	pShapes = NULL;
    pNoteShapes = NULL;
	pMergedRanges = NULL;
	pAreaLinks = NULL;
	pEmptyDatabaseRanges = NULL;
	pDetectiveObj = NULL;
	pDetectiveOp = NULL;
	nCurrentTable = SCTAB_MAX;
}

void ScMyNotEmptyCellsIterator::UpdateAddress( table::CellAddress& rAddress )
{
	if( pCellItr->ReturnNext( nCellCol, nCellRow ) )
	{
		rAddress.Column = nCellCol;
		rAddress.Row = nCellRow;
	}
}

void ScMyNotEmptyCellsIterator::SetCellData( ScMyCell& rMyCell, table::CellAddress& rAddress )
{
	rMyCell.aCellAddress = rAddress;
	rMyCell.bHasStringValue = sal_False;
	rMyCell.bHasDoubleValue = sal_False;
	rMyCell.bHasXText = sal_False;
	rMyCell.bKnowWhetherIsEditCell = sal_False;
	rMyCell.bIsEditCell = sal_False;
	if( (nCellCol == rAddress.Column) && (nCellRow == rAddress.Row) )
		pCellItr->GetNext( nCellCol, nCellRow );
}

void ScMyNotEmptyCellsIterator::SetMatrixCellData( ScMyCell& rMyCell )
{
	rMyCell.bIsMatrixCovered = sal_False;
	rMyCell.bIsMatrixBase = sal_False;

	sal_Bool bIsMatrixBase(sal_False);

	ScAddress aScAddress;
	ScUnoConversion::FillScAddress( aScAddress, rMyCell.aCellAddress );
    CellType eCalcType = rExport.GetDocument()->GetCellType( aScAddress );
	switch (eCalcType)
	{
		case CELLTYPE_VALUE:
			rMyCell.nType = table::CellContentType_VALUE;
			break;
		case CELLTYPE_STRING:
		case CELLTYPE_EDIT:
			rMyCell.nType = table::CellContentType_TEXT;
			break;
		case CELLTYPE_FORMULA:
			rMyCell.nType = table::CellContentType_FORMULA;
			break;
		default:
			rMyCell.nType = table::CellContentType_EMPTY; 
    }

	if (rMyCell.nType == table::CellContentType_FORMULA)
		if( rExport.IsMatrix( aScAddress, rMyCell.aMatrixRange, bIsMatrixBase ) )
		{
			rMyCell.bIsMatrixBase = bIsMatrixBase;
			rMyCell.bIsMatrixCovered = !bIsMatrixBase;
		}
}

void ScMyNotEmptyCellsIterator::HasAnnotation(ScMyCell& aCell)
{
	aCell.bHasAnnotation = sal_False;
	if (!aAnnotations.empty())
	{
		ScMyExportAnnotationList::iterator aItr(aAnnotations.begin());
		if ((aCell.aCellAddress.Column == aItr->aCellAddress.Column) &&
			(aCell.aCellAddress.Row == aItr->aCellAddress.Row))
		{
			aCell.xAnnotation.set(aItr->xAnnotation);
			uno::Reference<text::XSimpleText> xSimpleText(aCell.xAnnotation, uno::UNO_QUERY);
			if (aCell.xAnnotation.is() && xSimpleText.is())
			{
				aCell.sAnnotationText = xSimpleText->getString();
				if (aCell.sAnnotationText.getLength())
					aCell.bHasAnnotation = sal_True;
			}
			aAnnotations.erase(aItr);
		}
	}

    // test - bypass the API
    // if (xCellRange.is())
	// 	aCell.xCell.set(xCellRange->getCellByPosition(aCell.aCellAddress.Column, aCell.aCellAddress.Row));
}

void ScMyNotEmptyCellsIterator::SetCurrentTable(const SCTAB nTable,
	uno::Reference<sheet::XSpreadsheet>& rxTable)
{
	DBG_ASSERT(aAnnotations.empty(), "not all Annotations saved");
	aLastAddress.Row = 0;
	aLastAddress.Column = 0;
	aLastAddress.Sheet = nTable;
	if (nCurrentTable != nTable)
	{
		nCurrentTable = nTable;
		if (pCellItr)
			delete pCellItr;
		pCellItr = new ScHorizontalCellIterator(rExport.GetDocument(), nCurrentTable, 0, 0,
			static_cast<SCCOL>(rExport.GetSharedData()->GetLastColumn(nCurrentTable)), static_cast<SCROW>(rExport.GetSharedData()->GetLastRow(nCurrentTable)));
		xTable.set(rxTable);
		xCellRange.set(xTable, uno::UNO_QUERY);
		uno::Reference<sheet::XSheetAnnotationsSupplier> xSheetAnnotationsSupplier (xTable, uno::UNO_QUERY);
		if (xSheetAnnotationsSupplier.is())
		{
			uno::Reference<container::XEnumerationAccess> xAnnotationAccess ( xSheetAnnotationsSupplier->getAnnotations(), uno::UNO_QUERY);
			if (xAnnotationAccess.is())
			{
				uno::Reference<container::XEnumeration> xAnnotations(xAnnotationAccess->createEnumeration());
				if (xAnnotations.is())
				{
					while (xAnnotations->hasMoreElements())
					{
						ScMyExportAnnotation aAnnotation;
                        aAnnotation.xAnnotation.set(xAnnotations->nextElement(), uno::UNO_QUERY);
						if (aAnnotation.xAnnotation.is())
						{
							aAnnotation.aCellAddress = aAnnotation.xAnnotation->getPosition();
							aAnnotations.push_back(aAnnotation);
						}
					}
					if (!aAnnotations.empty())
						aAnnotations.sort();
				}
			}
		}
	}
}

void ScMyNotEmptyCellsIterator::SkipTable(SCTAB nSkip)
{
    // Skip entries for a sheet that is copied instead of saving normally.
    // Cells (including aAnnotations) are handled separately in SetCurrentTable.
    
    if( pShapes )
        pShapes->SkipTable(nSkip);
    if( pNoteShapes )
        pNoteShapes->SkipTable(nSkip);
    if( pEmptyDatabaseRanges )
        pEmptyDatabaseRanges->SkipTable(nSkip);
    if( pMergedRanges )
        pMergedRanges->SkipTable(nSkip);
    if( pAreaLinks )
        pAreaLinks->SkipTable(nSkip);
    if( pDetectiveObj )
        pDetectiveObj->SkipTable(nSkip);
    if( pDetectiveOp )
        pDetectiveOp->SkipTable(nSkip);
}

sal_Bool ScMyNotEmptyCellsIterator::GetNext(ScMyCell& aCell, ScFormatRangeStyles* pCellStyles)
{
	table::CellAddress	aAddress( nCurrentTable, MAXCOL + 1, MAXROW + 1 );

	UpdateAddress( aAddress );
	if( pShapes )
		pShapes->UpdateAddress( aAddress );
	if( pNoteShapes )
		pNoteShapes->UpdateAddress( aAddress );
	if( pEmptyDatabaseRanges )
		pEmptyDatabaseRanges->UpdateAddress( aAddress );
	if( pMergedRanges )
		pMergedRanges->UpdateAddress( aAddress );
	if( pAreaLinks )
		pAreaLinks->UpdateAddress( aAddress );
	if( pDetectiveObj )
		pDetectiveObj->UpdateAddress( aAddress );
	if( pDetectiveOp )
		pDetectiveOp->UpdateAddress( aAddress );

	sal_Bool bFoundCell((aAddress.Column <= MAXCOL) && (aAddress.Row <= MAXROW));
	if( bFoundCell )
	{
		SetCellData( aCell, aAddress );
		if( pShapes )
			pShapes->SetCellData( aCell );
		if( pNoteShapes )
			pNoteShapes->SetCellData( aCell );
		if( pEmptyDatabaseRanges )
			pEmptyDatabaseRanges->SetCellData( aCell );
		if( pMergedRanges )
			pMergedRanges->SetCellData( aCell );
		if( pAreaLinks )
			pAreaLinks->SetCellData( aCell );
		if( pDetectiveObj )
			pDetectiveObj->SetCellData( aCell );
		if( pDetectiveOp )
			pDetectiveOp->SetCellData( aCell );

		HasAnnotation( aCell );
		SetMatrixCellData( aCell );
		sal_Bool bIsAutoStyle;
        // Ranges before the previous cell are not needed by ExportFormatRanges anymore and can be removed
        sal_Int32 nRemoveBeforeRow = aLastAddress.Row;
		aCell.nStyleIndex = pCellStyles->GetStyleNameIndex(aCell.aCellAddress.Sheet,
			aCell.aCellAddress.Column, aCell.aCellAddress.Row,
            bIsAutoStyle, aCell.nValidationIndex, aCell.nNumberFormat, nRemoveBeforeRow);
		aLastAddress = aCell.aCellAddress;
		aCell.bIsAutoStyle = bIsAutoStyle;

        //#102799#; if the cell is in a DatabaseRange which should saved empty, the cell should have the type empty
        if (aCell.bHasEmptyDatabase)
            aCell.nType = table::CellContentType_EMPTY;
	}
	return bFoundCell;
}

