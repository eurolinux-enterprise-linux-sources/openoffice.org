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

#ifndef SC_DOCFUNC_HXX
#define SC_DOCFUNC_HXX

#include <tools/link.hxx>
#include "global.hxx"
#include "formula/grammar.hxx"

class ScEditEngineDefaulter;
class SdrUndoAction;
class ScAddress;
class ScDocShell;
class ScMarkData;
class ScPatternAttr;
class ScRange;
class ScRangeName;
class ScBaseCell;
class ScTokenArray;
struct ScTabOpParam;
class ScTableProtection;

// ---------------------------------------------------------------------------

class ScDocFunc
{
private:
	ScDocShell&		rDocShell;

	BOOL			AdjustRowHeight( const ScRange& rRange, BOOL bPaint = TRUE );
	void			CreateOneName( ScRangeName& rList,
									SCCOL nPosX, SCROW nPosY, SCTAB nTab,
									SCCOL nX1, SCROW nY1, SCCOL nX2, SCROW nY2,
									BOOL& rCancel, BOOL bApi );
	void			NotifyInputHandler( const ScAddress& rPos );

public:
					ScDocFunc( ScDocShell& rDocSh ): rDocShell(rDocSh) {}
					~ScDocFunc() {}

    DECL_LINK( NotifyDrawUndo, SdrUndoAction* );

	BOOL			DetectiveAddPred(const ScAddress& rPos);
	BOOL			DetectiveDelPred(const ScAddress& rPos);
	BOOL			DetectiveAddSucc(const ScAddress& rPos);
	BOOL			DetectiveDelSucc(const ScAddress& rPos);
	BOOL			DetectiveAddError(const ScAddress& rPos);
	BOOL			DetectiveMarkInvalid(SCTAB nTab);
	BOOL			DetectiveDelAll(SCTAB nTab);
	BOOL			DetectiveRefresh(BOOL bAutomatic = FALSE);

	BOOL			DeleteContents( const ScMarkData& rMark, USHORT nFlags,
									BOOL bRecord, BOOL bApi );

	BOOL			TransliterateText( const ScMarkData& rMark, sal_Int32 nType,
									BOOL bRecord, BOOL bApi );

	BOOL			SetNormalString( const ScAddress& rPos, const String& rText, BOOL bApi );
	BOOL			PutCell( const ScAddress& rPos, ScBaseCell* pNewCell, BOOL bApi );
	BOOL			PutData( const ScAddress& rPos, ScEditEngineDefaulter& rEngine,
								BOOL bInterpret, BOOL bApi );
	BOOL			SetCellText( const ScAddress& rPos, const String& rText,
									BOOL bInterpret, BOOL bEnglish, BOOL bApi,
                                    const String& rFormulaNmsp,
                                    const formula::FormulaGrammar::Grammar eGrammar );

					// creates a new cell for use with PutCell
	ScBaseCell*		InterpretEnglishString( const ScAddress& rPos, const String& rText,
                        const String& rFormulaNmsp, const formula::FormulaGrammar::Grammar eGrammar );

	bool			ShowNote( const ScAddress& rPos, bool bShow = true );
	inline bool		HideNote( const ScAddress& rPos ) { return ShowNote( rPos, false ); }

    bool            SetNoteText( const ScAddress& rPos, const String& rNoteText, BOOL bApi );
    bool            ReplaceNote( const ScAddress& rPos, const String& rNoteText, const String* pAuthor, const String* pDate, BOOL bApi );

	BOOL			ApplyAttributes( const ScMarkData& rMark, const ScPatternAttr& rPattern,
									BOOL bRecord, BOOL bApi );
	BOOL			ApplyStyle( const ScMarkData& rMark, const String& rStyleName,
									BOOL bRecord, BOOL bApi );

	BOOL			InsertCells( const ScRange& rRange,const ScMarkData* pTabMark,
                                 InsCellCmd eCmd, BOOL bRecord, BOOL bApi,
									BOOL bPartOfPaste = FALSE );
	BOOL			DeleteCells( const ScRange& rRange, const ScMarkData* pTabMark,
                                 DelCellCmd eCmd, BOOL bRecord, BOOL bApi );

	BOOL			MoveBlock( const ScRange& rSource, const ScAddress& rDestPos,
								BOOL bCut, BOOL bRecord, BOOL bPaint, BOOL bApi );

	BOOL			InsertTable( SCTAB nTab, const String& rName, BOOL bRecord, BOOL bApi );
	BOOL			RenameTable( SCTAB nTab, const String& rName, BOOL bRecord, BOOL bApi );
	BOOL			DeleteTable( SCTAB nTab, BOOL bRecord, BOOL bApi );

	BOOL			SetTableVisible( SCTAB nTab, BOOL bVisible, BOOL bApi );

	BOOL			SetLayoutRTL( SCTAB nTab, BOOL bRTL, BOOL bApi );

//UNUSED2009-05 BOOL	 	    SetGrammar( formula::FormulaGrammar::Grammar eGrammar );

	SC_DLLPUBLIC BOOL			SetWidthOrHeight( BOOL bWidth, SCCOLROW nRangeCnt, SCCOLROW* pRanges,
									SCTAB nTab, ScSizeMode eMode, USHORT nSizeTwips,
									BOOL bRecord, BOOL bApi );

	BOOL			InsertPageBreak( BOOL bColumn, const ScAddress& rPos,
									BOOL bRecord, BOOL bSetModified, BOOL bApi );
	BOOL			RemovePageBreak( BOOL bColumn, const ScAddress& rPos,
									BOOL bRecord, BOOL bSetModified, BOOL bApi );

    void            ProtectSheet( SCTAB nTab, const ScTableProtection& rProtect );

	BOOL			Protect( SCTAB nTab, const String& rPassword, BOOL bApi );
	BOOL			Unprotect( SCTAB nTab, const String& rPassword, BOOL bApi );

	BOOL			ClearItems( const ScMarkData& rMark, const USHORT* pWhich, BOOL bApi );
	BOOL			ChangeIndent( const ScMarkData& rMark, BOOL bIncrement, BOOL bApi );
	BOOL			AutoFormat( const ScRange& rRange, const ScMarkData* pTabMark,
									USHORT nFormatNo, BOOL bRecord, BOOL bApi );

	BOOL			EnterMatrix( const ScRange& rRange, const ScMarkData* pTabMark,
                                    const ScTokenArray* pTokenArray,
									const String& rString, BOOL bApi, BOOL bEnglish,
                                    const String& rFormulaNmsp,
                                    const formula::FormulaGrammar::Grammar );

	BOOL			TabOp( const ScRange& rRange, const ScMarkData* pTabMark,
							const ScTabOpParam& rParam, BOOL bRecord, BOOL bApi );

	BOOL			FillSimple( const ScRange& rRange, const ScMarkData* pTabMark,
								FillDir eDir, BOOL bRecord, BOOL bApi );
	BOOL			FillSeries( const ScRange& rRange, const ScMarkData* pTabMark,
								FillDir	eDir, FillCmd eCmd, FillDateCmd	eDateCmd,
								double fStart, double fStep, double fMax,
								BOOL bRecord, BOOL bApi );

	BOOL			ResizeMatrix( const ScRange& rOldRange, const ScAddress& rNewEnd, BOOL bApi );

	BOOL			MergeCells( const ScRange& rRange, BOOL bContents,
								BOOL bRecord, BOOL bApi );
	BOOL			UnmergeCells( const ScRange& rRange, BOOL bRecord, BOOL bApi );

    BOOL            SetNewRangeNames( ScRangeName* pNewRanges, BOOL bApi );     // takes ownership of pNewRanges
	BOOL			ModifyRangeNames( const ScRangeName& rNewRanges, BOOL bApi );

	BOOL			CreateNames( const ScRange& rRange, USHORT nFlags, BOOL bApi );
	BOOL			InsertNameList( const ScAddress& rStartPos, BOOL bApi );

	BOOL			InsertAreaLink( const String& rFile, const String& rFilter,
									const String& rOptions, const String& rSource,
									const ScRange& rDestRange, ULONG nRefresh,
									BOOL bFitBlock, BOOL bApi );
};



#endif

