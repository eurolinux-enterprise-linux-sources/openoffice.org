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
#ifndef SC_UNDOBLK_HXX
#define SC_UNDOBLK_HXX

#include "undobase.hxx"
#include "markdata.hxx"
#include "viewutil.hxx"
#include "spellparam.hxx"

class ScDocShell;
class ScBaseCell;
class ScDocument;
class ScOutlineTable;
class ScRangeName;
class ScRangeList;
class ScDBCollection;
class ScPatternAttr;
class SvxBoxItem;
class SvxBoxInfoItem;
class SvxSearchItem;
class SdrUndoAction;

//----------------------------------------------------------------------------

class ScUndoInsertCells: public ScMoveUndo
{
public:
					TYPEINFO();
					ScUndoInsertCells( ScDocShell* pNewDocShell,
									   const ScRange& rRange, SCTAB nNewCount, SCTAB* pNewTabs, SCTAB* pNewScenarios,
                                       InsCellCmd eNewCmd, ScDocument* pUndoDocument, ScRefUndoData* pRefData,
									   BOOL bNewPartOfPaste );
	virtual			~ScUndoInsertCells();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat( SfxRepeatTarget& rTarget );
	virtual BOOL	CanRepeat( SfxRepeatTarget& rTarget ) const;

	virtual String	GetComment() const;

	virtual BOOL	Merge( SfxUndoAction *pNextAction );

private:
	ScRange			aEffRange;
    SCTAB           nCount;
    SCTAB*          pTabs;
    SCTAB*          pScenarios;
	ULONG			nEndChangeAction;
	InsCellCmd		eCmd;
	BOOL			bPartOfPaste;
	SfxUndoAction*	pPasteUndo;

	void			DoChange ( const BOOL bUndo );
	void			SetChangeTrack();
};


class ScUndoDeleteCells: public ScMoveUndo
{
public:
					TYPEINFO();
					ScUndoDeleteCells( ScDocShell* pNewDocShell,
									   const ScRange& rRange, SCTAB nNewCount, SCTAB* pNewTabs, SCTAB* pNewScenarios, 
                                       DelCellCmd eNewCmd, ScDocument* pUndoDocument, ScRefUndoData* pRefData );
	virtual 		~ScUndoDeleteCells();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScRange			aEffRange;
    SCTAB           nCount;
    SCTAB*          pTabs;
    SCTAB*          pScenarios;
	ULONG			nStartChangeAction;
	ULONG			nEndChangeAction;
	DelCellCmd		eCmd;

	void			DoChange ( const BOOL bUndo );
	void			SetChangeTrack();
};


class ScUndoDeleteMulti: public ScMoveUndo
{
public:
					TYPEINFO();
					ScUndoDeleteMulti( ScDocShell* pNewDocShell,
                                       BOOL bNewRows, BOOL bNeedsRefresh, SCTAB nNewTab,
									   const SCCOLROW* pRng, SCCOLROW nRngCnt,
									   ScDocument* pUndoDocument, ScRefUndoData* pRefData );
	virtual			~ScUndoDeleteMulti();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	BOOL			bRows;
    BOOL            bRefresh;
	SCTAB			nTab;
	SCCOLROW*		pRanges;
	SCCOLROW		nRangeCnt;
	ULONG			nStartChangeAction;
	ULONG			nEndChangeAction;

	void			DoChange() const;
	void			SetChangeTrack();
};


class ScUndoCut: public ScBlockUndo
{
public:
					TYPEINFO();
					ScUndoCut( ScDocShell* pNewDocShell,
                               ScRange aRange,              // adjusted for merged cells
                               ScAddress aOldEnd,           // end position without adjustment
                               const ScMarkData& rMark,     // selected sheets
							   ScDocument* pNewUndoDoc );
	virtual			~ScUndoCut();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
    ScMarkData      aMarkData;
	ScDocument*		pUndoDoc;
	ScRange			aExtendedRange;
	ULONG			nStartChangeAction;
	ULONG			nEndChangeAction;

	void			DoChange( const BOOL bUndo );
	void			SetChangeTrack();
};


struct ScUndoPasteOptions
{
	USHORT nFunction;
	BOOL bSkipEmpty;
	BOOL bTranspose;
	BOOL bAsLink;
	InsCellCmd eMoveMode;

	ScUndoPasteOptions() :
		nFunction( PASTE_NOFUNC ),
		bSkipEmpty( FALSE ),
		bTranspose( FALSE ),
		bAsLink( FALSE ),
		eMoveMode( INS_NONE )
	{}
};

class ScUndoPaste: public ScBlockUndo
{
public:
					TYPEINFO();
					ScUndoPaste( ScDocShell* pNewDocShell,
								 SCCOL nStartX, SCROW nStartY, SCTAB nStartZ,
								 SCCOL nEndX, SCROW nEndY, SCTAB nEndZ,
								 const ScMarkData& rMark,
								 ScDocument* pNewUndoDoc, ScDocument* pNewRedoDoc,
								 USHORT nNewFlags,
								 ScRefUndoData* pRefData, void* pFill1, void* pFill2, void* pFill3,
								 BOOL bRedoIsFilled = TRUE,
								 const ScUndoPasteOptions* pOptions = NULL );
	virtual 		~ScUndoPaste();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScMarkData		aMarkData;
	ScDocument*		pUndoDoc;
	ScDocument*		pRedoDoc;
	USHORT			nFlags;
	ScRefUndoData*	pRefUndoData;
	ScRefUndoData*	pRefRedoData;
	ULONG			nStartChangeAction;
	ULONG			nEndChangeAction;
	BOOL			bRedoFilled;
	ScUndoPasteOptions aPasteOptions;

	void			DoChange( const BOOL bUndo );
	void			SetChangeTrack();
};


class ScUndoDragDrop: public ScMoveUndo
{
public:
					TYPEINFO();
					ScUndoDragDrop( ScDocShell* pNewDocShell,
									const ScRange& rRange, ScAddress aNewDestPos, BOOL bNewCut,
									ScDocument* pUndoDocument, ScRefUndoData* pRefData,
									BOOL bScenario );
	virtual 		~ScUndoDragDrop();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScRange			aSrcRange;
	ScRange			aDestRange;
	ULONG			nStartChangeAction;
	ULONG			nEndChangeAction;
	BOOL			bCut;
	BOOL			bKeepScenarioFlags;

	void			PaintArea( ScRange aRange, USHORT nExtFlags ) const;
	void			DoUndo( ScRange aRange ) const;

	void			SetChangeTrack();
};


class ScUndoDeleteContents: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoDeleteContents( ScDocShell* pNewDocShell,
										  const ScMarkData& rMark,
										  const ScRange& rRange,
										  ScDocument* pNewUndoDoc, BOOL bNewMulti,
										  USHORT nNewFlags, BOOL bObjects );
	virtual 		~ScUndoDeleteContents();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScRange			aRange;
	ScMarkData		aMarkData;
	ScDocument*		pUndoDoc;		// Blockmarkierung und geloeschte Daten
	SdrUndoAction*	pDrawUndo;		// geloeschte Objekte
	ULONG			nStartChangeAction;
	ULONG			nEndChangeAction;
	USHORT			nFlags;
	BOOL			bMulti;			// Mehrfachselektion

	void			DoChange( const BOOL bUndo );
	void			SetChangeTrack();
};


class ScUndoFillTable: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoFillTable( ScDocShell* pNewDocShell,
									 const ScMarkData& rMark,
									 SCCOL nStartX, SCROW nStartY, SCTAB nStartZ,
									 SCCOL nEndX, SCROW nEndY, SCTAB nEndZ,
									 ScDocument* pNewUndoDoc, BOOL bNewMulti, SCTAB nSrc,
									 USHORT nFlg, USHORT nFunc, BOOL bSkip, BOOL bLink );
	virtual			~ScUndoFillTable();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScRange			aRange;
	ScMarkData		aMarkData;
	ScDocument*		pUndoDoc;		// Blockmarkierung und geloeschte Daten
	ULONG			nStartChangeAction;
	ULONG			nEndChangeAction;
	USHORT			nFlags;
	USHORT			nFunction;
	SCTAB			nSrcTab;
	BOOL			bMulti;			// Mehrfachselektion
	BOOL			bSkipEmpty;
	BOOL			bAsLink;

	void			DoChange( const BOOL bUndo );
	void			SetChangeTrack();
};


class ScUndoSelectionAttr: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoSelectionAttr( ScDocShell* pNewDocShell,
										 const ScMarkData& rMark,
										 SCCOL nStartX, SCROW nStartY, SCTAB nStartZ,
										 SCCOL nEndX, SCROW nEndY, SCTAB nEndZ,
										 ScDocument* pNewUndoDoc, BOOL bNewMulti,
										 const ScPatternAttr* pNewApply,
										 const SvxBoxItem* pNewOuter = NULL,
										 const SvxBoxInfoItem* pNewInner = NULL );
	virtual 		~ScUndoSelectionAttr();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScMarkData		aMarkData;
	ScRange			aRange;
	ScDocument*		pUndoDoc;
	BOOL			bMulti;
	ScPatternAttr*	pApplyPattern;
	SvxBoxItem*		pLineOuter;
	SvxBoxInfoItem*	pLineInner;

	void			DoChange( const BOOL bUndo );
};


class ScUndoWidthOrHeight: public ScSimpleUndo
{
public:
							TYPEINFO();
							ScUndoWidthOrHeight( ScDocShell* pNewDocShell,
									const ScMarkData& rMark,
									SCCOLROW nNewStart, SCTAB nNewStartTab,
									SCCOLROW nNewEnd, SCTAB nNewEndTab,
									ScDocument* pNewUndoDoc,
									SCCOLROW nNewCnt, SCCOLROW* pNewRanges,
									ScOutlineTable* pNewUndoTab,
									ScSizeMode eNewMode, USHORT nNewSizeTwips,
									BOOL bNewWidth );
	virtual 				~ScUndoWidthOrHeight();

	virtual void			Undo();
	virtual void			Redo();
	virtual void			Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL			CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String          GetComment() const;

private:
	ScMarkData		aMarkData;
	SCCOLROW		nStart;
	SCCOLROW		nEnd;
	SCTAB			nStartTab;
	SCTAB			nEndTab;
	ScDocument*		pUndoDoc;
	ScOutlineTable*	pUndoTab;
	SCCOLROW		nRangeCnt;
	SCCOLROW*		pRanges;
	USHORT			nNewSize;
	BOOL			bWidth;
	ScSizeMode		eMode;
	SdrUndoAction*	pDrawUndo;
};


class ScUndoAutoFill: public ScBlockUndo
{
public:
					TYPEINFO();
					ScUndoAutoFill( ScDocShell* pNewDocShell,
									const ScRange& rRange, const ScRange& rSourceArea,
									ScDocument* pNewUndoDoc, const ScMarkData& rMark,
									FillDir eNewFillDir,
									FillCmd eNewFillCmd, FillDateCmd eNewFillDateCmd,
									double fNewStartValue, double fNewStepValue, double fNewMaxValue,
									USHORT nMaxShIndex );
	virtual 		~ScUndoAutoFill();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScRange			aSource;
	ScMarkData		aMarkData;
	ScDocument*		pUndoDoc;
	FillDir			eFillDir;
	FillCmd			eFillCmd;
	FillDateCmd		eFillDateCmd;
	double			fStartValue;
	double			fStepValue;
	double			fMaxValue;
	ULONG			nStartChangeAction;
	ULONG			nEndChangeAction;
	USHORT			nMaxSharedIndex;

	void			SetChangeTrack();
};


class ScUndoMerge: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoMerge( ScDocShell* pNewDocShell,
								 SCCOL nStartX, SCROW nStartY, SCTAB nStartZ,
								 SCCOL nEndX,   SCROW nEndY,   SCTAB nEndZ,
                                 bool bMergeContents, ScDocument* pUndoDoc, SdrUndoAction* pDrawUndo );
	virtual 		~ScUndoMerge();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
    ScRange         maRange;
    bool            mbMergeContents;        // Merge contents in Redo().
    ScDocument*		mpUndoDoc;              // wenn Daten zusammengefasst
    SdrUndoAction*  mpDrawUndo;

	void			DoChange( bool bUndo ) const;
};


class ScUndoAutoFormat: public ScBlockUndo
{
public:
					TYPEINFO();
					ScUndoAutoFormat( ScDocShell* pNewDocShell,
									  const ScRange& rRange, ScDocument* pNewUndoDoc,
									  const ScMarkData& rMark,
									  BOOL bNewSize, USHORT nNewFormatNo );
	virtual 		~ScUndoAutoFormat();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScDocument*		pUndoDoc;		// geloeschte Daten
	ScMarkData		aMarkData;
	BOOL			bSize;
	USHORT			nFormatNo;
};


class ScUndoReplace: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoReplace( ScDocShell* pNewDocShell,
								   const ScMarkData& rMark,
								   SCCOL nCurX, SCROW nCurY, SCTAB nCurZ,
								   const String& rNewUndoStr, ScDocument* pNewUndoDoc,
								   const SvxSearchItem* pItem );
	virtual 		~ScUndoReplace();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScAddress		aCursorPos;
	ScMarkData		aMarkData;
	String			aUndoStr;			// Daten bei Einfachmarkierung
	ScDocument*		pUndoDoc;			// Blockmarkierung und geloeschte Daten
	SvxSearchItem*	pSearchItem;
	ULONG			nStartChangeAction;
	ULONG			nEndChangeAction;

			void	SetChangeTrack();
};


class ScUndoTabOp: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoTabOp( ScDocShell* pNewDocShell,
								 SCCOL nStartX, SCROW nStartY, SCTAB nStartZ,
								 SCCOL nEndX,   SCROW nEndY,   SCTAB nEndZ,
								 ScDocument* pNewUndoDoc,
								 const ScRefAddress& rFormulaCell,
								 const ScRefAddress& rFormulaEnd,
								 const ScRefAddress& rRowCell,
								 const ScRefAddress& rColCell,
								 BYTE nMode );
	virtual			~ScUndoTabOp();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScRange			aRange;
	ScDocument*		pUndoDoc;		// geloeschte Daten
	ScRefAddress	theFormulaCell;
	ScRefAddress	theFormulaEnd;
	ScRefAddress	theRowCell;
	ScRefAddress	theColCell;
	BYTE			nMode;
};


class ScUndoConversion : public ScSimpleUndo
{
public:
                            TYPEINFO();

                            ScUndoConversion(
                                ScDocShell* pNewDocShell, const ScMarkData& rMark,
                                SCCOL nCurX, SCROW nCurY, SCTAB nCurZ, ScDocument* pNewUndoDoc,
                                SCCOL nNewX, SCROW nNewY, SCTAB nNewZ, ScDocument* pNewRedoDoc,
                                const ScConversionParam& rConvParam );
    virtual                 ~ScUndoConversion();

    virtual void            Undo();
    virtual void            Redo();
    virtual void            Repeat(SfxRepeatTarget& rTarget);
    virtual BOOL            CanRepeat(SfxRepeatTarget& rTarget) const;

    virtual String          GetComment() const;

private:
    ScMarkData              aMarkData;
    ScAddress               aCursorPos;
    ScDocument*             pUndoDoc;           // Blockmarkierung und geloeschte Daten
    ScAddress               aNewCursorPos;
    ScDocument*             pRedoDoc;           // Blockmarkierung und neue Daten
    ULONG                   nStartChangeAction;
    ULONG                   nEndChangeAction;
    ScConversionParam       maConvParam;        /// Conversion type and parameters.

    void                    DoChange( ScDocument* pRefDoc, const ScAddress& rCursorPos );
    void                    SetChangeTrack();
};

class ScUndoRefConversion: public ScSimpleUndo
{
public:
                        TYPEINFO();
                        ScUndoRefConversion( ScDocShell* pNewDocShell,
                            const ScRange& aMarkRange, const ScMarkData& rMark,
                            ScDocument* pNewUndoDoc, ScDocument* pNewRedoDoc, BOOL bNewMulti, USHORT nNewFlag);
    virtual             ~ScUndoRefConversion();

    virtual void        Undo();
    virtual void        Redo();
    virtual void        Repeat(SfxRepeatTarget& rTarget);
    virtual BOOL        CanRepeat(SfxRepeatTarget& rTarget) const;

    virtual String      GetComment() const;

private:
    ScMarkData          aMarkData;
    ScDocument*         pUndoDoc;
    ScDocument*         pRedoDoc;
    ScRange             aRange;
    BOOL                bMulti;
    USHORT              nFlags;
    ULONG               nStartChangeAction;
    ULONG               nEndChangeAction;

    void                DoChange( ScDocument* pRefDoc);
    void                SetChangeTrack();
};

class ScUndoListNames: public ScBlockUndo
{
public:
					TYPEINFO();
					ScUndoListNames( ScDocShell* pNewDocShell,
									 const ScRange& rRange,
									 ScDocument* pNewUndoDoc, ScDocument* pNewRedoDoc );
	virtual 		~ScUndoListNames();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScDocument*		pUndoDoc;
	ScDocument*		pRedoDoc;

	void			DoChange( ScDocument* pSrcDoc ) const;
};


class ScUndoUseScenario: public ScSimpleUndo
{
public:
							TYPEINFO();
							ScUndoUseScenario( ScDocShell* pNewDocShell,
									const ScMarkData& rMark,
									const ScArea& rDestArea, ScDocument* pNewUndoDoc,
									const String& rNewName );
	virtual 				~ScUndoUseScenario();

	virtual void			Undo();
	virtual void			Redo();
	virtual void			Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL			CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String          GetComment() const;

private:
	ScDocument*		pUndoDoc;
	ScRange			aRange;
	ScMarkData		aMarkData;
	String			aName;
};


class ScUndoSelectionStyle: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoSelectionStyle( ScDocShell* pNewDocShell,
										  const ScMarkData& rMark,
										  const ScRange& rRange,
										  const String& rName,
										  ScDocument* pNewUndoDoc );
	virtual 		~ScUndoSelectionStyle();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;
	virtual USHORT	GetId() const;

private:
	ScMarkData		aMarkData;
	ScDocument*		pUndoDoc;
	String			aStyleName;
	ScRange			aRange;

	void			DoChange( const BOOL bUndo );
};


class ScUndoRefreshLink: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoRefreshLink( ScDocShell* pNewDocShell,
									   ScDocument* pNewUndoDoc );
	virtual 		~ScUndoRefreshLink();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScDocument*		pUndoDoc;
	ScDocument*		pRedoDoc;
};


class ScUndoEnterMatrix: public ScBlockUndo
{
public:
					TYPEINFO();
					ScUndoEnterMatrix( ScDocShell* pNewDocShell,
									   const ScRange& rArea,
									   ScDocument* pNewUndoDoc,
									   const String& rForm );
	virtual 		~ScUndoEnterMatrix();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScDocument*		pUndoDoc;
	String			aFormula;
	formula::FormulaGrammar::AddressConvention eConv;
	ULONG			nStartChangeAction;
	ULONG			nEndChangeAction;

	void			SetChangeTrack();
};


class ScUndoInsertAreaLink : public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoInsertAreaLink( ScDocShell* pShell,
										  const String& rDoc,
										  const String& rFlt, const String& rOpt,
										  const String& rArea, const ScRange& rDestRange,
										  ULONG nRefreshDelay );
	virtual			~ScUndoInsertAreaLink();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	String			aDocName;
	String			aFltName;
	String			aOptions;
	String			aAreaName;
	ScRange			aRange;
	ULONG			nRefreshDelay;
};


class ScUndoRemoveAreaLink : public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoRemoveAreaLink( ScDocShell* pShell,
										  const String& rDoc,
										  const String& rFlt, const String& rOpt,
										  const String& rArea, const ScRange& rDestRange,
										  ULONG nRefreshDelay );
	virtual			~ScUndoRemoveAreaLink();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	String			aDocName;
	String			aFltName;
	String			aOptions;
	String			aAreaName;
	ScRange			aRange;
	ULONG			nRefreshDelay;
};


class ScUndoUpdateAreaLink : public ScSimpleUndo		//! auch BlockUndo umstellen?
{
public:
					TYPEINFO();
					ScUndoUpdateAreaLink( ScDocShell* pShell,
										  const String& rOldD,
										  const String& rOldF, const String& rOldO,
										  const String& rOldA, const ScRange& rOldR,
										  ULONG nOldRD,
										  const String& rNewD,
										  const String& rNewF, const String& rNewO,
										  const String& rNewA, const ScRange& rNewR,
										  ULONG nNewRD,
										  ScDocument* pUndo, ScDocument* pRedo,
										  BOOL bDoInsert );
	virtual			~ScUndoUpdateAreaLink();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	String			aOldDoc;
	String			aOldFlt;
	String			aOldOpt;
	String			aOldArea;
	ScRange			aOldRange;
	String			aNewDoc;
	String			aNewFlt;
	String			aNewOpt;
	String			aNewArea;
	ScRange			aNewRange;
	ScDocument*		pUndoDoc;
	ScDocument*		pRedoDoc;
	ULONG			nOldRefresh;
	ULONG			nNewRefresh;
	BOOL			bWithInsert;

	void			DoChange( const BOOL bUndo ) const;
};


class ScUndoIndent: public ScBlockUndo
{
public:
					TYPEINFO();
					ScUndoIndent( ScDocShell* pNewDocShell, const ScMarkData& rMark,
									ScDocument* pNewUndoDoc, BOOL bIncrement );
	virtual 		~ScUndoIndent();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScMarkData		aMarkData;
	ScDocument*		pUndoDoc;
	BOOL			bIsIncrement;
};


class ScUndoTransliterate: public ScBlockUndo
{
public:
					TYPEINFO();
					ScUndoTransliterate( ScDocShell* pNewDocShell, const ScMarkData& rMark,
										ScDocument* pNewUndoDoc, sal_Int32 nType );
	virtual 		~ScUndoTransliterate();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScMarkData		aMarkData;
	ScDocument*		pUndoDoc;
	sal_Int32		nTransliterationType;
};


class ScUndoClearItems: public ScBlockUndo
{
public:
					TYPEINFO();
					ScUndoClearItems( ScDocShell* pNewDocShell, const ScMarkData& rMark,
										ScDocument* pNewUndoDoc, const USHORT* pW );
	virtual 		~ScUndoClearItems();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScMarkData		aMarkData;
	ScDocument*		pUndoDoc;
	USHORT*			pWhich;
};


class ScUndoRemoveBreaks: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoRemoveBreaks( ScDocShell* pNewDocShell,
									SCTAB nNewTab, ScDocument* pNewUndoDoc );
	virtual 		~ScUndoRemoveBreaks();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	SCTAB			nTab;
	ScDocument*		pUndoDoc;
};


class ScUndoRemoveMerge: public ScBlockUndo
{
public:
					TYPEINFO();
					ScUndoRemoveMerge( ScDocShell* pNewDocShell,
									   const ScRange& rArea,
									   ScDocument* pNewUndoDoc );
	virtual 		~ScUndoRemoveMerge();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScDocument*		pUndoDoc;
};


class ScUndoBorder: public ScBlockUndo
{
public:
					TYPEINFO();
					ScUndoBorder( ScDocShell* pNewDocShell,
									const ScRangeList& rRangeList,
									ScDocument* pNewUndoDoc,
									const SvxBoxItem& rNewOuter,
									const SvxBoxInfoItem& rNewInner );
	virtual 		~ScUndoBorder();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	ScDocument*		pUndoDoc;
	ScRangeList*	pRanges;
	SvxBoxItem*		pOuter;
	SvxBoxInfoItem*	pInner;
};




#endif

