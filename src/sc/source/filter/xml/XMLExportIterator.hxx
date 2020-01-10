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

#ifndef SC_XMLEXPORTITERATOR_HXX
#define SC_XMLEXPORTITERATOR_HXX

#include <vector>
#include <list>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/table/CellAddress.hpp>
#include <com/sun/star/table/CellRangeAddress.hpp>
#include <com/sun/star/table/XCell.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/sheet/XSheetAnnotation.hpp>
#include <com/sun/star/drawing/XShape.hpp>
#include "global.hxx"
#include "detfunc.hxx"
#include "detdata.hxx"

class	ScHorizontalCellIterator;
struct	ScMyCell;
class	ScXMLExport;
class	ScFormatRangeStyles;
class   ScBaseCell;

//==============================================================================

class ScMyIteratorBase
{
protected:
	virtual sal_Bool			GetFirstAddress( ::com::sun::star::table::CellAddress& rCellAddress ) = 0;

public:
								ScMyIteratorBase();
	virtual						~ScMyIteratorBase();

	virtual void				SetCellData( ScMyCell& rMyCell ) = 0;
	virtual void				Sort() = 0;

	virtual void				UpdateAddress( ::com::sun::star::table::CellAddress& rCellAddress );
};

//==============================================================================

struct ScMyShape
{
	ScAddress	aAddress;
	ScAddress	aEndAddress;
	com::sun::star::uno::Reference<com::sun::star::drawing::XShape> xShape;

	sal_Bool operator<(const ScMyShape& aShape) const;
};

typedef std::list<ScMyShape>	ScMyShapeList;

class ScMyShapesContainer : ScMyIteratorBase
{
private:
	ScMyShapeList				aShapeList;
protected:
	virtual sal_Bool			GetFirstAddress( ::com::sun::star::table::CellAddress& rCellAddress );
public:
								ScMyShapesContainer();
	virtual						~ScMyShapesContainer();

                                using ScMyIteratorBase::UpdateAddress;
	void						AddNewShape(const ScMyShape& aShape);
	sal_Bool					HasShapes() { return !aShapeList.empty(); }
	const ScMyShapeList*		GetShapes() { return &aShapeList; }
	virtual void				SetCellData( ScMyCell& rMyCell );
	virtual void				Sort();
    void                        SkipTable(SCTAB nSkip);
};

struct ScMyNoteShape
{
    com::sun::star::uno::Reference<com::sun::star::drawing::XShape> xShape;
    ScAddress aPos;

    sal_Bool operator<(const ScMyNoteShape& aNote) const;
};

typedef std::list<ScMyNoteShape>	ScMyNoteShapeList;

class ScMyNoteShapesContainer : ScMyIteratorBase
{
private:
	ScMyNoteShapeList			aNoteShapeList;
protected:
	virtual sal_Bool			GetFirstAddress( ::com::sun::star::table::CellAddress& rCellAddress );
public:
								ScMyNoteShapesContainer();
	virtual						~ScMyNoteShapesContainer();

                                using ScMyIteratorBase::UpdateAddress;
	void						AddNewNote(const ScMyNoteShape& aNote);
	sal_Bool					HasNotes() { return !aNoteShapeList.empty(); }
	const ScMyNoteShapeList*	GetNotes() { return &aNoteShapeList; }
	virtual void				SetCellData( ScMyCell& rMyCell );
	virtual void				Sort();
    void                        SkipTable(SCTAB nSkip);
};

//==============================================================================

struct ScMyMergedRange
{
	com::sun::star::table::CellRangeAddress aCellRange;
	sal_Int32					nRows;
	sal_Bool			   		bIsFirst;
	sal_Bool					operator<(const ScMyMergedRange& aRange) const;
};

typedef std::list<ScMyMergedRange>	ScMyMergedRangeList;

class ScMyMergedRangesContainer : ScMyIteratorBase
{
private:
	ScMyMergedRangeList			aRangeList;
protected:
	virtual sal_Bool			GetFirstAddress( ::com::sun::star::table::CellAddress& rCellAddress );
public:
								ScMyMergedRangesContainer();
	virtual						~ScMyMergedRangesContainer();
	void						AddRange(const com::sun::star::table::CellRangeAddress aMergedRange);

                                using ScMyIteratorBase::UpdateAddress;
	virtual void				SetCellData( ScMyCell& rMyCell );
	virtual void				Sort();	// + remove doublets
    void                        SkipTable(SCTAB nSkip);
};

//==============================================================================

struct ScMyAreaLink
{
	::rtl::OUString				sFilter;
	::rtl::OUString				sFilterOptions;
	::rtl::OUString				sURL;
	::rtl::OUString				sSourceStr;
	::com::sun::star::table::CellRangeAddress aDestRange;
	sal_Int32					nRefresh;

	inline						ScMyAreaLink() : nRefresh( 0 ) {}

	inline sal_Int32			GetColCount() const	{ return aDestRange.EndColumn - aDestRange.StartColumn + 1; }
	inline sal_Int32			GetRowCount() const	{ return aDestRange.EndRow - aDestRange.StartRow + 1; }

	sal_Bool					Compare( const ScMyAreaLink& rAreaLink ) const;
	sal_Bool 					operator<(const ScMyAreaLink& rAreaLink ) const;
};

typedef ::std::list< ScMyAreaLink > ScMyAreaLinkList;

class ScMyAreaLinksContainer : ScMyIteratorBase
{
private:
	ScMyAreaLinkList			aAreaLinkList;
protected:
	virtual sal_Bool			GetFirstAddress( ::com::sun::star::table::CellAddress& rCellAddress );
public:
								ScMyAreaLinksContainer();
	virtual						~ScMyAreaLinksContainer();

	inline void					AddNewAreaLink( const ScMyAreaLink& rAreaLink )
									{ aAreaLinkList.push_back( rAreaLink ); }

                                using ScMyIteratorBase::UpdateAddress;
	virtual void				SetCellData( ScMyCell& rMyCell );
	virtual void				Sort();
    void                        SkipTable(SCTAB nSkip);
};

//==============================================================================

struct ScMyCellRangeAddress : com::sun::star::table::CellRangeAddress
{
	ScMyCellRangeAddress(const com::sun::star::table::CellRangeAddress& rRange);
	sal_Bool 					operator<(const ScMyCellRangeAddress& rCellRangeAddress ) const;
};

typedef std::list<ScMyCellRangeAddress> ScMyEmptyDatabaseRangeList;

class ScMyEmptyDatabaseRangesContainer : ScMyIteratorBase
{
private:
	ScMyEmptyDatabaseRangeList	aDatabaseList;
protected:
	virtual sal_Bool			GetFirstAddress( ::com::sun::star::table::CellAddress& rCellAddress );
public:
								ScMyEmptyDatabaseRangesContainer();
	virtual						~ScMyEmptyDatabaseRangesContainer();
	void						AddNewEmptyDatabaseRange(const com::sun::star::table::CellRangeAddress& aCellRangeAddress);

                                using ScMyIteratorBase::UpdateAddress;
	virtual void				SetCellData( ScMyCell& rMyCell );
	virtual void				Sort();
    void                        SkipTable(SCTAB nSkip);
};

//==============================================================================

struct ScMyDetectiveObj
{
	::com::sun::star::table::CellAddress		aPosition;
	::com::sun::star::table::CellRangeAddress	aSourceRange;
	ScDetectiveObjType							eObjType;
	sal_Bool									bHasError;
	sal_Bool operator<(const ScMyDetectiveObj& rDetObj) const;
};

typedef ::std::list< ScMyDetectiveObj > ScMyDetectiveObjList;
typedef ::std::vector< ScMyDetectiveObj > ScMyDetectiveObjVec;

class ScMyDetectiveObjContainer : ScMyIteratorBase
{
private:
	ScMyDetectiveObjList			aDetectiveObjList;
protected:
	virtual sal_Bool			GetFirstAddress( ::com::sun::star::table::CellAddress& rCellAddress );
public:
								ScMyDetectiveObjContainer();
	virtual						~ScMyDetectiveObjContainer();

	void						AddObject(
									ScDetectiveObjType eObjType,
                                    const SCTAB nSheet,
									const ScAddress& rPosition,
									const ScRange& rSourceRange,
									sal_Bool bHasError );

                                using ScMyIteratorBase::UpdateAddress;
	virtual void				SetCellData( ScMyCell& rMyCell );
	virtual void				Sort();
    void                        SkipTable(SCTAB nSkip);
};

//==============================================================================

struct ScMyDetectiveOp
{
	::com::sun::star::table::CellAddress	aPosition;
	ScDetOpType								eOpType;
	sal_Int32								nIndex;
	sal_Bool operator<(const ScMyDetectiveOp& rDetOp) const;
};

typedef ::std::list< ScMyDetectiveOp > ScMyDetectiveOpList;
typedef ::std::vector< ScMyDetectiveOp > ScMyDetectiveOpVec;

class ScMyDetectiveOpContainer : ScMyIteratorBase
{
private:
	ScMyDetectiveOpList			aDetectiveOpList;
protected:
	virtual sal_Bool			GetFirstAddress( ::com::sun::star::table::CellAddress& rCellAddress );
public:
								ScMyDetectiveOpContainer();
	virtual						~ScMyDetectiveOpContainer();

	void						AddOperation( ScDetOpType eOpType, const ScAddress& rPosition, sal_uInt32 nIndex );

                                using ScMyIteratorBase::UpdateAddress;
	virtual void				SetCellData( ScMyCell& rMyCell );
	virtual void				Sort();
    void                        SkipTable(SCTAB nSkip);
};

//==============================================================================

// contains data to export for the current cell position
struct ScMyCell
{
//	com::sun::star::uno::Reference<com::sun::star::table::XCell> xCell;
//	com::sun::star::uno::Reference<com::sun::star::text::XText> xText;
	com::sun::star::uno::Reference<com::sun::star::sheet::XSheetAnnotation> xAnnotation;
	com::sun::star::uno::Reference<com::sun::star::drawing::XShape> xNoteShape;
	com::sun::star::table::CellAddress		aCellAddress;
	com::sun::star::table::CellRangeAddress	aMergeRange;
	com::sun::star::table::CellRangeAddress	aMatrixRange;

	rtl::OUString				sStringValue;
	rtl::OUString				sAnnotationText;

	ScMyAreaLink				aAreaLink;
	ScMyShapeList				aShapeList;
	ScMyDetectiveObjVec			aDetectiveObjVec;
	ScMyDetectiveOpVec			aDetectiveOpVec;

	double						fValue;
	sal_Int32					nValidationIndex;
	sal_Int32					nStyleIndex;
	sal_Int32					nNumberFormat;
	com::sun::star::table::CellContentType	nType;

    ScBaseCell*                 pBaseCell;

	sal_Bool					bIsAutoStyle;

	sal_Bool					bHasShape;
	sal_Bool					bIsMergedBase;
	sal_Bool					bIsCovered;
	sal_Bool					bHasAreaLink;
	sal_Bool					bHasEmptyDatabase;
	sal_Bool					bHasDetectiveObj;
	sal_Bool					bHasDetectiveOp;

	sal_Bool					bIsEditCell;
	sal_Bool					bKnowWhetherIsEditCell;
	sal_Bool					bHasStringValue;
	sal_Bool					bHasDoubleValue;
	sal_Bool					bHasXText;

	sal_Bool					bIsMatrixBase;
	sal_Bool					bIsMatrixCovered;
	sal_Bool					bHasAnnotation;

								ScMyCell();
								~ScMyCell();
};

//==============================================================================

struct ScMyExportAnnotation
{
	com::sun::star::uno::Reference<com::sun::star::sheet::XSheetAnnotation> xAnnotation;
	com::sun::star::table::CellAddress		aCellAddress;
	sal_Bool operator<(const ScMyExportAnnotation& rAnno) const;
};

typedef ::std::list< ScMyExportAnnotation > ScMyExportAnnotationList;

class ScMyNotEmptyCellsIterator
{
	com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet> xTable;
	com::sun::star::uno::Reference<com::sun::star::table::XCellRange> xCellRange;
	com::sun::star::table::CellAddress	aLastAddress;
	ScMyExportAnnotationList			aAnnotations;

	ScMyShapesContainer*				pShapes;
	ScMyNoteShapesContainer*			pNoteShapes;
	ScMyEmptyDatabaseRangesContainer*	pEmptyDatabaseRanges;
	ScMyMergedRangesContainer*			pMergedRanges;
	ScMyAreaLinksContainer*				pAreaLinks;
	ScMyDetectiveObjContainer*			pDetectiveObj;
	ScMyDetectiveOpContainer*			pDetectiveOp;

	ScXMLExport&				rExport;
	ScHorizontalCellIterator*	pCellItr;

	SCCOL					nCellCol;
	SCROW					nCellRow;
	SCTAB					nCurrentTable;

	void						UpdateAddress( ::com::sun::star::table::CellAddress& rAddress );
	void						SetCellData( ScMyCell& rMyCell, ::com::sun::star::table::CellAddress& rAddress );

	void						SetMatrixCellData( ScMyCell& rMyCell );
	void						HasAnnotation( ScMyCell& aCell );
public:
								ScMyNotEmptyCellsIterator(ScXMLExport& rExport);
								~ScMyNotEmptyCellsIterator();

	void						Clear();

	inline void					SetShapes(ScMyShapesContainer* pNewShapes)
									{ pShapes = pNewShapes; }
	inline void					SetNoteShapes(ScMyNoteShapesContainer* pNewNoteShapes)
									{ pNoteShapes = pNewNoteShapes; }
	inline void					SetEmptyDatabaseRanges(ScMyEmptyDatabaseRangesContainer* pNewEmptyDatabaseRanges)
									{ pEmptyDatabaseRanges = pNewEmptyDatabaseRanges; }
	inline void					SetMergedRanges(ScMyMergedRangesContainer* pNewMergedRanges)
									{ pMergedRanges = pNewMergedRanges; }
	inline void					SetAreaLinks(ScMyAreaLinksContainer* pNewAreaLinks)
									{ pAreaLinks = pNewAreaLinks; }
	inline void					SetDetectiveObj(ScMyDetectiveObjContainer* pNewDetectiveObj)
									{ pDetectiveObj = pNewDetectiveObj; }
	inline void					SetDetectiveOp(ScMyDetectiveOpContainer* pNewDetectiveOp)
									{ pDetectiveOp = pNewDetectiveOp; }

	void						SetCurrentTable(const SCTAB nTable,
									com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheet>& rxTable);
    void                        SkipTable(SCTAB nSkip);

	sal_Bool					GetNext(ScMyCell& aCell, ScFormatRangeStyles* pCellStyles);
};

#endif

