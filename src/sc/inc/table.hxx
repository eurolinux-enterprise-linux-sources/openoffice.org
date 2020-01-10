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

#ifndef SC_TABLE_HXX
#define SC_TABLE_HXX

#include <vector>
#include <memory>
#include <utility>
#include <tools/gen.hxx>
#include <tools/color.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include "column.hxx"
#include "sortparam.hxx"
#include "compressedarray.hxx"

#include <memory>

namespace utl {
	class SearchParam;
	class TextSearch;
}

class SfxItemSet;
class SfxStyleSheetBase;
class SvxBoxInfoItem;
class SvxBoxItem;
class SvxSearchItem;

class ScAutoFormat;
class ScAutoFormatData;
class ScBaseCell;
class ScDocument;
class ScDrawLayer;
class ScFormulaCell;
class ScOutlineTable;
class ScPostIt;
class ScPrintSaverTab;
class ScProgress;
class ScProgress;
class ScRangeList;
class ScSortInfoArray;
class ScStyleSheet;
class ScTableLink;
class ScTableProtection;
class ScUserListData;
struct RowInfo;
struct ScFunctionData;
struct ScLineFlags;
class CollatorWrapper;


class ScTable
{
private:
    typedef ::std::vector< ScRange > ScRangeVec;
    typedef ::std::pair< SCCOL, SCROW > ScAddress2D;
    typedef ::std::vector< ScAddress2D > ScAddress2DVec;
    typedef ::std::auto_ptr< ScAddress2DVec > ScAddress2DVecPtr;

											//	Daten pro Tabelle	------------------
	ScColumn		aCol[MAXCOLCOUNT];

	String			aName;
	String			aComment;
	BOOL			bScenario;
	BOOL			bLayoutRTL;
    BOOL            bLoadingRTL;

	String			aLinkDoc;
	String			aLinkFlt;
	String			aLinkOpt;
	String			aLinkTab;
	ULONG			nLinkRefreshDelay;
	BYTE			nLinkMode;

	// Seitenformatvorlage
	String			aPageStyle;
	BOOL			bPageSizeValid;
	Size			aPageSizeTwips;					// Groesse der Druck-Seite
	SCCOL			nRepeatStartX;					// Wiederholungszeilen/Spalten
	SCCOL			nRepeatEndX;					// REPEAT_NONE, wenn nicht benutzt
	SCROW			nRepeatStartY;
	SCROW			nRepeatEndY;

    ::std::auto_ptr<ScTableProtection> pTabProtection;

	USHORT*			pColWidth;
	ScSummableCompressedArray< SCROW, USHORT>*  pRowHeight;

	BYTE*			pColFlags;
	ScBitMaskCompressedArray< SCROW, BYTE>*     pRowFlags;

	ScOutlineTable*	pOutlineTable;

	SCCOL			nTableAreaX;
	SCROW			nTableAreaY;
	BOOL			bTableAreaValid;

											//	interne Verwaltung	------------------
	BOOL			bVisible;
    BOOL            bStreamValid;
    BOOL            bPendingRowHeights;

	SCTAB			nTab;
	USHORT			nRecalcLvl;				// Rekursionslevel Size-Recalc
	ScDocument*		pDocument;
	utl::SearchParam*	pSearchParam;
	utl::TextSearch*	pSearchText;

    mutable String  aUpperName;             // #i62977# filled only on demand, reset in SetName

    ScAddress2DVecPtr mxUninitNotes;

	// SortierParameter um den Stackbedarf von Quicksort zu Minimieren
	ScSortParam		aSortParam;
	CollatorWrapper*	pSortCollator;
	BOOL			bGlobalKeepQuery;
	BOOL			bSharedNameInserted;

    ScRangeVec      aPrintRanges;
    BOOL            bPrintEntireSheet;

	ScRange*		pRepeatColRange;
	ScRange*		pRepeatRowRange;

	USHORT			nLockCount;

	ScRangeList*	pScenarioRanges;
	Color			aScenarioColor;
	USHORT			nScenarioFlags;
	BOOL			bActiveScenario;

friend class ScDocument;					// fuer FillInfo
friend class ScDocumentIterator;
friend class ScValueIterator;
friend class ScQueryValueIterator;
friend class ScCellIterator;
friend class ScQueryCellIterator;
friend class ScHorizontalCellIterator;
friend class ScHorizontalAttrIterator;
friend class ScDocAttrIterator;
friend class ScAttrRectIterator;


public:
				ScTable( ScDocument* pDoc, SCTAB nNewTab, const String& rNewName,
							BOOL bColInfo = TRUE, BOOL bRowInfo = TRUE );
				~ScTable();

	ScOutlineTable*	GetOutlineTable()				{ return pOutlineTable; }

	ULONG		GetCellCount() const;
	ULONG		GetWeightedCount() const;
	ULONG		GetCodeCount() const;		// RPN-Code in Formeln

	BOOL		SetOutlineTable( const ScOutlineTable* pNewOutline );
	void		StartOutlineTable();

	void		DoAutoOutline( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow );

	BOOL		TestRemoveSubTotals( const ScSubTotalParam& rParam );
	void		RemoveSubTotals( ScSubTotalParam& rParam );
	BOOL		DoSubTotals( ScSubTotalParam& rParam );

	BOOL		IsVisible() const							 { return bVisible; }
	void		SetVisible( BOOL bVis );

    BOOL        IsStreamValid() const                        { return bStreamValid; }
    void        SetStreamValid( BOOL bSet, BOOL bIgnoreLock = FALSE );

    BOOL        IsPendingRowHeights() const                  { return bPendingRowHeights; }
    void        SetPendingRowHeights( BOOL bSet );

    BOOL        IsLayoutRTL() const                          { return bLayoutRTL; }
    BOOL        IsLoadingRTL() const                         { return bLoadingRTL; }
    void        SetLayoutRTL( BOOL bSet );
    void        SetLoadingRTL( BOOL bSet );

	BOOL		IsScenario() const							 { return bScenario; }
	void		SetScenario( BOOL bFlag );
	void 		GetScenarioComment( String& rComment) const	 { rComment = aComment; }
	void		SetScenarioComment( const String& rComment ) { aComment = rComment; }
	const Color& GetScenarioColor() const					 { return aScenarioColor; }
	void		SetScenarioColor(const Color& rNew)			 { aScenarioColor = rNew; }
	USHORT		GetScenarioFlags() const					 { return nScenarioFlags; }
	void		SetScenarioFlags(USHORT nNew)				 { nScenarioFlags = nNew; }
	void		SetActiveScenario(BOOL bSet)				 { bActiveScenario = bSet; }
	BOOL		IsActiveScenario() const					 { return bActiveScenario; }

	BYTE		GetLinkMode() const							{ return nLinkMode; }
	BOOL		IsLinked() const							{ return nLinkMode != SC_LINK_NONE; }
	const String& GetLinkDoc() const						{ return aLinkDoc; }
	const String& GetLinkFlt() const						{ return aLinkFlt; }
	const String& GetLinkOpt() const						{ return aLinkOpt; }
	const String& GetLinkTab() const						{ return aLinkTab; }
	ULONG		GetLinkRefreshDelay() const					{ return nLinkRefreshDelay; }

	void		SetLink( BYTE nMode, const String& rDoc, const String& rFlt,
						const String& rOpt, const String& rTab, ULONG nRefreshDelay );

	void		GetName( String& rName ) const;
	void		SetName( const String& rNewName );

    const String&   GetUpperName() const;

	const String&	GetPageStyle() const					{ return aPageStyle; }
	void			SetPageStyle( const String& rName );
	void			PageStyleModified( const String& rNewName );

    BOOL            IsProtected() const;
    void            SetProtection(const ScTableProtection* pProtect);
    ScTableProtection* GetProtection();

	Size			GetPageSize() const;
	void			SetPageSize( const Size& rSize );
	void			SetRepeatArea( SCCOL nStartCol, SCCOL nEndCol, SCROW nStartRow, SCROW nEndRow );

	void		RemoveAutoSpellObj();

	void		LockTable();
	void		UnlockTable();

	BOOL		IsBlockEditable( SCCOL nCol1, SCROW nRow1, SCCOL nCol2,
						SCROW nRow2, BOOL* pOnlyNotBecauseOfMatrix = NULL ) const;
	BOOL		IsSelectionEditable( const ScMarkData& rMark,
						BOOL* pOnlyNotBecauseOfMatrix = NULL ) const;

	BOOL		HasBlockMatrixFragment( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2 ) const;
	BOOL		HasSelectionMatrixFragment( const ScMarkData& rMark ) const;

	BOOL		IsBlockEmpty( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, bool bIgnoreNotes = false ) const;

	void		PutCell( const ScAddress&, ScBaseCell* pCell );
//UNUSED2009-05 void		PutCell( const ScAddress&, ULONG nFormatIndex, ScBaseCell* pCell);
	void		PutCell( SCCOL nCol, SCROW nRow, ScBaseCell* pCell );
	void		PutCell(SCCOL nCol, SCROW nRow, ULONG nFormatIndex, ScBaseCell* pCell);
				//	TRUE = Zahlformat gesetzt
	BOOL		SetString( SCCOL nCol, SCROW nRow, SCTAB nTab, const String& rString );
	void		SetValue( SCCOL nCol, SCROW nRow, const double& rVal );
	void 		SetError( SCCOL nCol, SCROW nRow, USHORT nError);

	void		GetString( SCCOL nCol, SCROW nRow, String& rString );
	void		GetInputString( SCCOL nCol, SCROW nRow, String& rString );
	double		GetValue( const ScAddress& rPos ) const
					{
                        return ValidColRow(rPos.Col(),rPos.Row()) ? 
                            aCol[rPos.Col()].GetValue( rPos.Row() ) :
                            0.0;
                    }
	double		GetValue( SCCOL nCol, SCROW nRow );
	void		GetFormula( SCCOL nCol, SCROW nRow, String& rFormula,
							BOOL bAsciiExport = FALSE );

	CellType	GetCellType( const ScAddress& rPos ) const
					{
                        return ValidColRow(rPos.Col(),rPos.Row()) ? 
                            aCol[rPos.Col()].GetCellType( rPos.Row() ) : 
                            CELLTYPE_NONE;
                    }
	CellType	GetCellType( SCCOL nCol, SCROW nRow ) const;
	ScBaseCell*	GetCell( const ScAddress& rPos ) const
					{
                        return ValidColRow(rPos.Col(),rPos.Row()) ? 
                            aCol[rPos.Col()].GetCell( rPos.Row() ) :
                            NULL;
                    }
	ScBaseCell*	GetCell( SCCOL nCol, SCROW nRow ) const;

	void		GetLastDataPos(SCCOL& rCol, SCROW& rRow) const;

    /** Returns the pointer to a cell note object at the passed cell address. */
    ScPostIt*   GetNote( SCCOL nCol, SCROW nRow );
    /** Sets the passed cell note object at the passed cell address. Takes ownership! */
    void        TakeNote( SCCOL nCol, SCROW nRow, ScPostIt*& rpNote );
    /** Returns and forgets the cell note object at the passed cell address. */
    ScPostIt*   ReleaseNote( SCCOL nCol, SCROW nRow );
    /** Deletes the note at the passed cell address. */
    void        DeleteNote( SCCOL nCol, SCROW nRow );
    /** Creates the captions of all uninitialized cell notes.
        @param bForced  True = always create all captions, false = skip when Undo is disabled. */
    void        InitializeNoteCaptions( bool bForced = false );

	BOOL		TestInsertRow( SCCOL nStartCol, SCCOL nEndCol, SCSIZE nSize );
	void		InsertRow( SCCOL nStartCol, SCCOL nEndCol, SCROW nStartRow, SCSIZE nSize );
	void		DeleteRow( SCCOL nStartCol, SCCOL nEndCol, SCROW nStartRow, SCSIZE nSize,
							BOOL* pUndoOutline = NULL );

	BOOL		TestInsertCol( SCROW nStartRow, SCROW nEndRow, SCSIZE nSize );
	void		InsertCol( SCCOL nStartCol, SCROW nStartRow, SCROW nEndRow, SCSIZE nSize );
	void		DeleteCol( SCCOL nStartCol, SCROW nStartRow, SCROW nEndRow, SCSIZE nSize,
							BOOL* pUndoOutline = NULL );

	void		DeleteArea(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, USHORT nDelFlag);
	void		CopyToClip(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, ScTable* pTable,
                            BOOL bKeepScenarioFlags, BOOL bCloneNoteCaptions);
    void        CopyToClip(const ScRangeList& rRanges, ScTable* pTable, 
                           bool bKeepScenarioFlags, bool bCloneNoteCaptions);
	void		CopyFromClip(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, SCsCOL nDx, SCsROW nDy,
								USHORT nInsFlag, BOOL bAsLink, BOOL bSkipAttrForEmpty, ScTable* pTable);
	void		StartListeningInArea( SCCOL nCol1, SCROW nRow1,
										SCCOL nCol2, SCROW nRow2 );
	void		BroadcastInArea( SCCOL nCol1, SCROW nRow1,
									SCCOL nCol2, SCROW nRow2 );

	void		CopyToTable(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
							USHORT nFlags, BOOL bMarked, ScTable* pDestTab,
							const ScMarkData* pMarkData = NULL,
							BOOL bAsLink = FALSE, BOOL bColRowFlags = TRUE);
	void		UndoToTable(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
							USHORT nFlags, BOOL bMarked, ScTable* pDestTab,
							const ScMarkData* pMarkData = NULL);

	void		TransposeClip( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
								ScTable* pTransClip, USHORT nFlags, BOOL bAsLink );

				//	Markierung von diesem Dokument
	void		MixMarked( const ScMarkData& rMark, USHORT nFunction,
							BOOL bSkipEmpty, ScTable* pSrcTab );
	void		MixData( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
							USHORT nFunction, BOOL bSkipEmpty, ScTable* pSrcTab );

	void		CopyData( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
							SCCOL nDestCol, SCROW nDestRow, SCTAB nDestTab );

	void		CopyScenarioFrom( const ScTable* pSrcTab );
	void		CopyScenarioTo( ScTable* pDestTab ) const;
	BOOL		TestCopyScenarioTo( const ScTable* pDestTab ) const;
	void		MarkScenarioIn( ScMarkData& rMark, USHORT nNeededBits ) const;
	BOOL		HasScenarioRange( const ScRange& rRange ) const;
	void		InvalidateScenarioRanges();
	const ScRangeList* GetScenarioRanges() const;

	void		CopyUpdated( const ScTable* pPosTab, ScTable* pDestTab ) const;

	void		InvalidateTableArea()						{ bTableAreaValid = FALSE; }

	BOOL		GetCellArea( SCCOL& rEndCol, SCROW& rEndRow ) const;			// FALSE = leer
	BOOL		GetTableArea( SCCOL& rEndCol, SCROW& rEndRow ) const;
	BOOL		GetPrintArea( SCCOL& rEndCol, SCROW& rEndRow, BOOL bNotes ) const;
	BOOL		GetPrintAreaHor( SCROW nStartRow, SCROW nEndRow,
								SCCOL& rEndCol, BOOL bNotes ) const;
	BOOL		GetPrintAreaVer( SCCOL nStartCol, SCCOL nEndCol,
								SCROW& rEndRow, BOOL bNotes ) const;

	BOOL		GetDataStart( SCCOL& rStartCol, SCROW& rStartRow ) const;

	void		ExtendPrintArea( OutputDevice* pDev,
						SCCOL nStartCol, SCROW nStartRow, SCCOL& rEndCol, SCROW nEndRow );

	void		GetDataArea( SCCOL& rStartCol, SCROW& rStartRow, SCCOL& rEndCol, SCROW& rEndRow,
								BOOL bIncludeOld );

	SCSIZE	    GetEmptyLinesInBlock( SCCOL nStartCol, SCROW nStartRow,
										SCCOL nEndCol, SCROW nEndRow, ScDirection eDir );

	void		FindAreaPos( SCCOL& rCol, SCROW& rRow, SCsCOL nMovX, SCsROW nMovY );
	void		GetNextPos( SCCOL& rCol, SCROW& rRow, SCsCOL nMovX, SCsROW nMovY,
								BOOL bMarked, BOOL bUnprotected, const ScMarkData& rMark );

	void		LimitChartArea( SCCOL& rStartCol, SCROW& rStartRow, SCCOL& rEndCol, SCROW& rEndRow );

	BOOL		HasData( SCCOL nCol, SCROW nRow );
	BOOL		HasStringData( SCCOL nCol, SCROW nRow );
	BOOL		HasValueData( SCCOL nCol, SCROW nRow );
//UNUSED2008-05  USHORT		GetErrorData(SCCOL nCol, SCROW nRow) const;
	BOOL		HasStringCells( SCCOL nStartCol, SCROW nStartRow,
								SCCOL nEndCol, SCROW nEndRow ) const;

	USHORT		GetErrCode( const ScAddress& rPos ) const
					{
                        return ValidColRow(rPos.Col(),rPos.Row()) ? 
                            aCol[rPos.Col()].GetErrCode( rPos.Row() ) :
                            0;
                    }
//UNUSED2008-05  USHORT		GetErrCode( SCCOL nCol, SCROW nRow ) const;

	void		ResetChanged( const ScRange& rRange );

	void		SetDirty();
	void		SetDirty( const ScRange& );
	void		SetDirtyAfterLoad();
	void		SetDirtyVar();
	void		SetTableOpDirty( const ScRange& );
	void		CalcAll();
	void		CalcAfterLoad();
	void		CompileAll();
	void		CompileXML( ScProgress& rProgress );
    bool        MarkUsedExternalReferences();

	void		UpdateReference( UpdateRefMode eUpdateRefMode, SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
									SCCOL nCol2, SCROW nRow2, SCTAB nTab2,
									SCsCOL nDx, SCsROW nDy, SCsTAB nDz,
									ScDocument* pUndoDoc = NULL, BOOL bIncludeDraw = TRUE, bool bUpdateNoteCaptionPos = true );

	void		UpdateDrawRef( UpdateRefMode eUpdateRefMode, SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
									SCCOL nCol2, SCROW nRow2, SCTAB nTab2,
									SCsCOL nDx, SCsROW nDy, SCsTAB nDz, bool bUpdateNoteCaptionPos = true );

	void		UpdateTranspose( const ScRange& rSource, const ScAddress& rDest,
									ScDocument* pUndoDoc );

	void		UpdateGrow( const ScRange& rArea, SCCOL nGrowX, SCROW nGrowY );

	void		UpdateInsertTab(SCTAB nTable);
//UNUSED2008-05  void        UpdateInsertTabOnlyCells(SCTAB nTable);
	void 		UpdateDeleteTab( SCTAB nTable, BOOL bIsMove, ScTable* pRefUndo = NULL );
	void		UpdateMoveTab(SCTAB nOldPos, SCTAB nNewPos, SCTAB nTabNo, ScProgress& );
	void		UpdateCompile( BOOL bForceIfNameInUse = FALSE );
	void		SetTabNo(SCTAB nNewTab);
	BOOL		IsRangeNameInUse(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
								 USHORT nIndex) const;
    void        FindRangeNamesInUse(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
                                 std::set<USHORT>& rIndexes) const;
	void 		ReplaceRangeNamesInUse(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
									  const ScRangeData::IndexMap& rMap );
	void		Fill( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
						ULONG nFillCount, FillDir eFillDir, FillCmd eFillCmd, FillDateCmd eFillDateCmd,
						double nStepValue, double nMaxValue);
	String		GetAutoFillPreview( const ScRange& rSource, SCCOL nEndX, SCROW nEndY );

	void		UpdateSelectionFunction( ScFunctionData& rData,
						SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
						const ScMarkData& rMark );

	void		AutoFormat( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
									USHORT nFormatNo );
	void		GetAutoFormatData(SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow, ScAutoFormatData& rData);
	void 		ScReplaceTabsStr( String& rStr, const String& rSrch, const String& rRepl ); // aus sw
	BOOL		SearchAndReplace(const SvxSearchItem& rSearchItem,
								SCCOL& rCol, SCROW& rRow, ScMarkData& rMark,
								String& rUndoStr, ScDocument* pUndoDoc);

	void		FindMaxRotCol( RowInfo* pRowInfo, SCSIZE nArrCount, SCCOL nX1, SCCOL nX2 ) const;

	void		GetBorderLines( SCCOL nCol, SCROW nRow,
								const SvxBorderLine** ppLeft, const SvxBorderLine** ppTop,
								const SvxBorderLine** ppRight, const SvxBorderLine** ppBottom ) const;

//UNUSED2009-05 BOOL		HasLines( const ScRange& rRange, Rectangle& rSizes ) const;
	BOOL		HasAttrib( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, USHORT nMask ) const;
	BOOL		HasAttribSelection( const ScMarkData& rMark, USHORT nMask ) const;
	BOOL		ExtendMerge( SCCOL nStartCol, SCROW nStartRow,
								SCCOL& rEndCol, SCROW& rEndRow,
								BOOL bRefresh, BOOL bAttrs );
	const SfxPoolItem*		GetAttr( SCCOL nCol, SCROW nRow, USHORT nWhich ) const;
	const ScPatternAttr*	GetPattern( SCCOL nCol, SCROW nRow ) const;
    const ScPatternAttr*    GetMostUsedPattern( SCCOL nCol, SCROW nStartRow, SCROW nEndRow ) const;

	ULONG					GetNumberFormat( const ScAddress& rPos ) const
								{
                                    return ValidColRow(rPos.Col(),rPos.Row()) ? 
                                        aCol[rPos.Col()].GetNumberFormat( rPos.Row() ) :
                                        0;
                                }
	ULONG					GetNumberFormat( SCCOL nCol, SCROW nRow ) const;
	void					MergeSelectionPattern( ScMergePatternState& rState,
												const ScMarkData& rMark, BOOL bDeep ) const;
	void					MergePatternArea( ScMergePatternState& rState, SCCOL nCol1, SCROW nRow1,
												SCCOL nCol2, SCROW nRow2, BOOL bDeep ) const;
	void					MergeBlockFrame( SvxBoxItem* pLineOuter, SvxBoxInfoItem* pLineInner,
											ScLineFlags& rFlags,
											SCCOL nStartCol, SCROW nStartRow,
											SCCOL nEndCol, SCROW nEndRow ) const;
	void					ApplyBlockFrame( const SvxBoxItem* pLineOuter,
											const SvxBoxInfoItem* pLineInner,
											SCCOL nStartCol, SCROW nStartRow,
											SCCOL nEndCol, SCROW nEndRow );

	void		ApplyAttr( SCCOL nCol, SCROW nRow, const SfxPoolItem& rAttr );
	void		ApplyPattern( SCCOL nCol, SCROW nRow, const ScPatternAttr& rAttr );
	void		ApplyPatternArea( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow, const ScPatternAttr& rAttr );
	void		SetPattern( const ScAddress& rPos, const ScPatternAttr& rAttr, BOOL bPutToPool = FALSE )
					{
                        if (ValidColRow(rPos.Col(),rPos.Row()))
                            aCol[rPos.Col()].SetPattern( rPos.Row(), rAttr, bPutToPool );
                    }
	void		SetPattern( SCCOL nCol, SCROW nRow, const ScPatternAttr& rAttr, BOOL bPutToPool = FALSE );
	void		ApplyPatternIfNumberformatIncompatible( const ScRange& rRange,
							const ScPatternAttr& rPattern, short nNewType );

	void		ApplyStyle( SCCOL nCol, SCROW nRow, const ScStyleSheet& rStyle );
	void		ApplyStyleArea( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow, const ScStyleSheet& rStyle );
	void		ApplySelectionStyle(const ScStyleSheet& rStyle, const ScMarkData& rMark);
	void		ApplySelectionLineStyle( const ScMarkData& rMark,
									const SvxBorderLine* pLine, BOOL bColorOnly );

	const ScStyleSheet*	GetStyle( SCCOL nCol, SCROW nRow ) const;
	const ScStyleSheet*	GetSelectionStyle( const ScMarkData& rMark, BOOL& rFound ) const;
	const ScStyleSheet*	GetAreaStyle( BOOL& rFound, SCCOL nCol1, SCROW nRow1,
													SCCOL nCol2, SCROW nRow2 ) const;

	void		StyleSheetChanged( const SfxStyleSheetBase* pStyleSheet, BOOL bRemoved,
									OutputDevice* pDev,
									double nPPTX, double nPPTY,
									const Fraction& rZoomX, const Fraction& rZoomY );

	BOOL		IsStyleSheetUsed( const ScStyleSheet& rStyle, BOOL bGatherAllStyles ) const;

	BOOL		ApplyFlags( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow, INT16 nFlags );
	BOOL		RemoveFlags( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow, INT16 nFlags );

	void		ApplySelectionCache( SfxItemPoolCache* pCache, const ScMarkData& rMark );
    void        DeleteSelection( USHORT nDelFlag, const ScMarkData& rMark );

	void		ClearSelectionItems( const USHORT* pWhich, const ScMarkData& rMark );
	void		ChangeSelectionIndent( BOOL bIncrement, const ScMarkData& rMark );

	const ScRange*	GetRepeatColRange() const	{ return pRepeatColRange; }
	const ScRange*	GetRepeatRowRange() const	{ return pRepeatRowRange; }
	void			SetRepeatColRange( const ScRange* pNew );
	void			SetRepeatRowRange( const ScRange* pNew );

    USHORT          GetPrintRangeCount() const          { return static_cast< USHORT >( aPrintRanges.size() ); }
	const ScRange*	GetPrintRange(USHORT nPos) const;
    /** Returns true, if the sheet is always printed. */
    BOOL            IsPrintEntireSheet() const          { return bPrintEntireSheet; }

    /** Removes all print ranges. */
    void            ClearPrintRanges();
    /** Adds a new print ranges. */
    void            AddPrintRange( const ScRange& rNew );
//UNUSED2009-05 /** Removes all old print ranges and sets the passed print ranges. */
//UNUSED2009-05 void            SetPrintRange( const ScRange& rNew );
    /** Marks the specified sheet to be printed completely. Deletes old print ranges! */
    void            SetPrintEntireSheet();

	void			FillPrintSaver( ScPrintSaverTab& rSaveTab ) const;
	void			RestorePrintRanges( const ScPrintSaverTab& rSaveTab );

	USHORT		GetOptimalColWidth( SCCOL nCol, OutputDevice* pDev,
									double nPPTX, double nPPTY,
									const Fraction& rZoomX, const Fraction& rZoomY,
									BOOL bFormula, const ScMarkData* pMarkData,
									BOOL bSimpleTextImport );
	BOOL		SetOptimalHeight( SCROW nStartRow, SCROW nEndRow, USHORT nExtra,
									OutputDevice* pDev,
									double nPPTX, double nPPTY,
									const Fraction& rZoomX, const Fraction& rZoomY,
                                    BOOL bForce,
                                    ScProgress* pOuterProgress = NULL, ULONG nProgressStart = 0 );
	long		GetNeededSize( SCCOL nCol, SCROW nRow,
									OutputDevice* pDev,
									double nPPTX, double nPPTY,
									const Fraction& rZoomX, const Fraction& rZoomY,
									BOOL bWidth, BOOL bTotalSize );
	void		SetColWidth( SCCOL nCol, USHORT nNewWidth );
	void		SetRowHeight( SCROW nRow, USHORT nNewHeight );
	BOOL		SetRowHeightRange( SCROW nStartRow, SCROW nEndRow, USHORT nNewHeight,
									double nPPTX, double nPPTY );
						// nPPT fuer Test auf Veraenderung
	void		SetManualHeight( SCROW nStartRow, SCROW nEndRow, BOOL bManual );

	USHORT		GetColWidth( SCCOL nCol ) const;
	USHORT		GetRowHeight( SCROW nRow ) const;
	ULONG		GetRowHeight( SCROW nStartRow, SCROW nEndRow ) const;
	ULONG		GetScaledRowHeight( SCROW nStartRow, SCROW nEndRow, double fScale ) const;
	ULONG		GetColOffset( SCCOL nCol ) const;
	ULONG		GetRowOffset( SCROW nRow ) const;

	USHORT		GetOriginalWidth( SCCOL nCol ) const;
	USHORT		GetOriginalHeight( SCROW nRow ) const;

	USHORT		GetCommonWidth( SCCOL nEndCol ) const;

	SCROW		GetHiddenRowCount( SCROW nRow ) const;

	void		ShowCol(SCCOL nCol, BOOL bShow);
	void		ShowRow(SCROW nRow, BOOL bShow);
	void		DBShowRow(SCROW nRow, BOOL bShow);

	void		ShowRows(SCROW nRow1, SCROW nRow2, BOOL bShow);
	void		DBShowRows(SCROW nRow1, SCROW nRow2, BOOL bShow);

	void		SetColFlags( SCCOL nCol, BYTE nNewFlags );
	void		SetRowFlags( SCROW nRow, BYTE nNewFlags );
	void		SetRowFlags( SCROW nStartRow, SCROW nEndRow, BYTE nNewFlags );

                /// @return  the index of the last row with any set flags (auto-pagebreak is ignored).
    SCROW      GetLastFlaggedRow() const;

                /// @return  the index of the last changed column (flags and column width, auto pagebreak is ignored).
    SCCOL      GetLastChangedCol() const;
                /// @return  the index of the last changed row (flags and row height, auto pagebreak is ignored).
    SCROW      GetLastChangedRow() const;

	BOOL		IsFiltered(SCROW nRow) const;

	BYTE		GetColFlags( SCCOL nCol ) const;
	BYTE		GetRowFlags( SCROW nRow ) const;

    const ScBitMaskCompressedArray< SCROW, BYTE> * GetRowFlagsArray() const
                    { return pRowFlags; }
    const ScSummableCompressedArray< SCROW, USHORT> * GetRowHeightArray() const
                    { return pRowHeight; }

	BOOL		UpdateOutlineCol( SCCOL nStartCol, SCCOL nEndCol, BOOL bShow );
	BOOL		UpdateOutlineRow( SCROW nStartRow, SCROW nEndRow, BOOL bShow );

	void		UpdatePageBreaks( const ScRange* pUserArea );
	void		RemoveManualBreaks();
	BOOL		HasManualBreaks() const;

	void		StripHidden( SCCOL& rX1, SCROW& rY1, SCCOL& rX2, SCROW& rY2 );
	void		ExtendHidden( SCCOL& rX1, SCROW& rY1, SCCOL& rX2, SCROW& rY2 );

	void		Sort(const ScSortParam& rSortParam, BOOL bKeepQuery);
    BOOL        ValidQuery(SCROW nRow, const ScQueryParam& rQueryParam,
                    BOOL* pSpecial = NULL, ScBaseCell* pCell = NULL,
                    BOOL* pbTestEqualCondition = NULL );
	void		TopTenQuery( ScQueryParam& );
	SCSIZE		Query(ScQueryParam& rQueryParam, BOOL bKeepSub);
	BOOL		CreateQueryParam(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, ScQueryParam& rQueryParam);

	void		GetFilterEntries(SCCOL nCol, SCROW nRow1, SCROW nRow2, TypedScStrCollection& rStrings);
    void        GetFilteredFilterEntries( SCCOL nCol, SCROW nRow1, SCROW nRow2, const ScQueryParam& rParam, TypedScStrCollection& rStrings );
	BOOL		GetDataEntries(SCCOL nCol, SCROW nRow, TypedScStrCollection& rStrings, BOOL bLimit);

	BOOL		HasColHeader( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow );
	BOOL		HasRowHeader( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow );

	void		DoColResize( SCCOL nCol1, SCCOL nCol2, SCSIZE nAdd );

	sal_Int32	GetMaxStringLen( SCCOL nCol,
									SCROW nRowStart, SCROW nRowEnd, CharSet eCharSet ) const;
	xub_StrLen	GetMaxNumberStringLen( USHORT& nPrecision,
									SCCOL nCol,
									SCROW nRowStart, SCROW nRowEnd ) const;

	void		FindConditionalFormat( ULONG nKey, ScRangeList& rRanges );

	void		IncRecalcLevel() { ++nRecalcLvl; }
	void		DecRecalcLevel( bool bUpdateNoteCaptionPos = true ) { if (!--nRecalcLvl) SetDrawPageSize(true, bUpdateNoteCaptionPos); }

	BOOL		IsSortCollatorGlobal() const;
	void		InitSortCollator( const ScSortParam& rPar );
	void		DestroySortCollator();

private:
	void		FillSeries( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
								ULONG nFillCount, FillDir eFillDir, FillCmd eFillCmd,
								FillDateCmd eFillDateCmd,
								double nStepValue, double nMaxValue, USHORT nMinDigits,
								BOOL bAttribs, ScProgress& rProgress );
	void		FillAnalyse( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
								FillCmd& rCmd, FillDateCmd& rDateCmd,
								double& rInc, USHORT& rMinDigits,
								ScUserListData*& rListData, USHORT& rListIndex);

	BOOL		ValidNextPos( SCCOL nCol, SCROW nRow, const ScMarkData& rMark,
								BOOL bMarked, BOOL bUnprotected );

	void		AutoFormatArea(SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
								const ScPatternAttr& rAttr, USHORT nFormatNo);
	void 		GetAutoFormatAttr(SCCOL nCol, SCROW nRow, USHORT nIndex, ScAutoFormatData& rData);
	void		GetAutoFormatFrame(SCCOL nCol, SCROW nRow, USHORT nFlags, USHORT nIndex, ScAutoFormatData& rData);
	BOOL 		SearchCell(const SvxSearchItem& rSearchItem, SCCOL nCol, SCROW nRow,
							const ScMarkData& rMark, String& rUndoStr, ScDocument* pUndoDoc);
	BOOL		Search(const SvxSearchItem& rSearchItem, SCCOL& rCol, SCROW& rRow,
						const ScMarkData& rMark, String& rUndoStr, ScDocument* pUndoDoc);
	BOOL		SearchAll(const SvxSearchItem& rSearchItem, ScMarkData& rMark,
						String& rUndoStr, ScDocument* pUndoDoc);
	BOOL		Replace(const SvxSearchItem& rSearchItem, SCCOL& rCol, SCROW& rRow,
						const ScMarkData& rMark, String& rUndoStr, ScDocument* pUndoDoc);
	BOOL		ReplaceAll(const SvxSearchItem& rSearchItem, ScMarkData& rMark,
							String& rUndoStr, ScDocument* pUndoDoc);

	BOOL		SearchStyle(const SvxSearchItem& rSearchItem, SCCOL& rCol, SCROW& rRow,
								ScMarkData& rMark);
	BOOL		ReplaceStyle(const SvxSearchItem& rSearchItem, SCCOL& rCol, SCROW& rRow,
								ScMarkData& rMark, BOOL bIsUndo);
	BOOL		SearchAllStyle(const SvxSearchItem& rSearchItem, ScMarkData& rMark);
	BOOL		ReplaceAllStyle(const SvxSearchItem& rSearchItem, ScMarkData& rMark,
								ScDocument* pUndoDoc);

								// benutzen globalen SortParam:
	BOOL		IsSorted(SCCOLROW nStart, SCCOLROW nEnd);
	void		DecoladeRow( ScSortInfoArray*, SCROW nRow1, SCROW nRow2 );
	void		SwapCol(SCCOL nCol1, SCCOL nCol2);
	void		SwapRow(SCROW nRow1, SCROW nRow2);
	short 		CompareCell( USHORT nSort,
					ScBaseCell* pCell1, SCCOL nCell1Col, SCROW nCell1Row,
					ScBaseCell* pCell2, SCCOL nCell2Col, SCROW nCell2Row );
	short		Compare(SCCOLROW nIndex1, SCCOLROW nIndex2);
	short		Compare( ScSortInfoArray*, SCCOLROW nIndex1, SCCOLROW nIndex2);
	ScSortInfoArray*	CreateSortInfoArray( SCCOLROW nInd1, SCCOLROW nInd2 );
	void		QuickSort( ScSortInfoArray*, SCsCOLROW nLo, SCsCOLROW nHi);
	void		SortReorder( ScSortInfoArray*, ScProgress& );

	BOOL 		CreateExcelQuery(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, ScQueryParam& rQueryParam);
	BOOL 		CreateStarQuery(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, ScQueryParam& rQueryParam);
	void 		GetUpperCellString(SCCOL nCol, SCROW nRow, String& rStr);

	BOOL		RefVisible(ScFormulaCell* pCell);

	BOOL		IsEmptyLine(SCROW nRow, SCCOL nStartCol, SCCOL nEndCol);

	void 		IncDate(double& rVal, USHORT& nDayOfMonth, double nStep, FillDateCmd eCmd);
	void		FillFormula(ULONG& nFormulaCounter, BOOL bFirst, ScFormulaCell* pSrcCell,
							SCCOL nDestCol, SCROW nDestRow, BOOL bLast );
	void		UpdateInsertTabAbs(SCTAB nNewPos);
	BOOL 		GetNextSpellingCell(SCCOL& rCol, SCROW& rRow, BOOL bInSel,
									const ScMarkData& rMark) const;
	BOOL		GetNextMarkedCell( SCCOL& rCol, SCROW& rRow, const ScMarkData& rMark );
    void        SetDrawPageSize( bool bResetStreamValid = true, bool bUpdateNoteCaptionPos = true );
	BOOL		TestTabRefAbs(SCTAB nTable);
	void 		CompileDBFormula();
	void 		CompileDBFormula( BOOL bCreateFormulaString );
	void 		CompileNameFormula( BOOL bCreateFormulaString );
	void 		CompileColRowNameFormula();

	void		StartListening( const ScAddress& rAddress, SvtListener* pListener );
	void		EndListening( const ScAddress& rAddress, SvtListener* pListener );
	void		StartAllListeners();
    void        StartNeededListeners(); // only for cells where NeedsListening()==TRUE
	void		SetRelNameDirty();

	SCSIZE		FillMaxRot( RowInfo* pRowInfo, SCSIZE nArrCount, SCCOL nX1, SCCOL nX2,
							SCCOL nCol, SCROW nAttrRow1, SCROW nAttrRow2, SCSIZE nArrY,
							const ScPatternAttr* pPattern, const SfxItemSet* pCondSet ) const;

    // idle calculation of OutputDevice text width for cell
    // also invalidates script type, broadcasts for "calc as shown"
    void        InvalidateTextWidth( const ScAddress* pAdrFrom, const ScAddress* pAdrTo,
                                     BOOL bNumFormatChanged, BOOL bBroadcast );
};


#endif


