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

#ifndef SC_UNDOTAB_HXX
#define SC_UNDOTAB_HXX

#include "undobase.hxx"
#include "markdata.hxx"
#include "formula/grammar.hxx"
#include <tools/color.hxx>

#ifndef _SVSTDARR_SHORTS

#define _SVSTDARR_SHORTS
#include <svtools/svstdarr.hxx>

#endif

#ifndef _SVSTDARR_STRINGS

#define _SVSTDARR_STRINGS
#include <svtools/svstdarr.hxx>

#endif

#include <com/sun/star/uno/Sequence.hxx>

#include <memory>

class ScDocShell;
class ScDocument;
class SdrUndoAction;
class ScPrintRangeSaver;
class SdrObject;
class ScDocProtection;
class ScTableProtection;

//----------------------------------------------------------------------------

class ScUndoInsertTab : public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoInsertTab(
							ScDocShell* pNewDocShell,
							SCTAB nTabNum,
							BOOL bApp,
							const String& rNewName);
	virtual			~ScUndoInsertTab();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	String			sNewName;
	SdrUndoAction*	pDrawUndo;
	ULONG			nEndChangeAction;
	SCTAB			nTab;
	BOOL			bAppend;

	void			SetChangeTrack();
};

class ScUndoInsertTables : public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoInsertTables(
							ScDocShell* pNewDocShell,
							SCTAB nTabNum,
							BOOL bApp,
							SvStrings *pNewNameList);
	virtual			~ScUndoInsertTables();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:

	SdrUndoAction*	pDrawUndo;
	SvStrings*		pNameList;
	ULONG			nStartChangeAction;
	ULONG			nEndChangeAction;
	SCTAB			nTab;
	BOOL			bAppend;

	void			SetChangeTrack();
};


class ScUndoDeleteTab: public ScMoveUndo	// Draw vom Move fuer geloeschte Tabelle
{
public:
					TYPEINFO();
					ScUndoDeleteTab(
							ScDocShell* pNewDocShell,
							const SvShorts &theTabs,		//SCTAB nNewTab,
							ScDocument* pUndoDocument,
							ScRefUndoData* pRefData );
	virtual			~ScUndoDeleteTab();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	SvShorts	theTabs;
	ULONG			nStartChangeAction;
	ULONG			nEndChangeAction;

	void			SetChangeTrack();
};


class ScUndoRenameTab: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoRenameTab(
							ScDocShell* pNewDocShell,
							SCTAB nT,
							const String& rOldName,
							const String& rNewName);
	virtual			~ScUndoRenameTab();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	SCTAB	nTab;
	String	sOldName;
	String	sNewName;

	void DoChange( SCTAB nTab, const String& rName ) const;
};


class ScUndoMoveTab: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoMoveTab( ScDocShell* pNewDocShell,
								  const SvShorts &aOldTab,
								  const SvShorts &aNewTab);
	virtual			~ScUndoMoveTab();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	SvShorts	theOldTabs;
	SvShorts	theNewTabs;

	void DoChange( BOOL bUndo ) const;
};


class ScUndoCopyTab: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoCopyTab(ScDocShell* pNewDocShell,
								  const SvShorts &aOldTab,
								  const SvShorts &aNewTab);

	virtual			~ScUndoCopyTab();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	SdrUndoAction*	pDrawUndo;
	SvShorts	theOldTabs;
	SvShorts	theNewTabs;

	void DoChange() const;
};


class ScUndoMakeScenario: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoMakeScenario(
							ScDocShell* pNewDocShell,
							SCTAB nSrc, SCTAB nDest,
							const String& rN, const String& rC,
							const Color& rCol, USHORT nF,
							const ScMarkData& rMark );
	virtual			~ScUndoMakeScenario();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	SCTAB		nSrcTab;
	SCTAB		nDestTab;
	String		aName;
	String		aComment;
	Color		aColor;
	USHORT		nFlags;
	ScMarkData	aMarkData;
    SdrUndoAction* pDrawUndo;
};


class ScUndoImportTab : public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoImportTab(
							ScDocShell* pShell,
							SCTAB nNewTab, SCTAB nNewCount,
							BOOL bNewLink );
	virtual			~ScUndoImportTab();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	SCTAB		nTab;
	SCTAB		nCount;
	BOOL		bLink;
	ScDocument*	pRedoDoc;
	SdrUndoAction*	pDrawUndo;

	void DoChange() const;
};


class ScUndoRemoveLink : public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoRemoveLink(				// vor dem Loeschen aufrufen!
							ScDocShell* pShell,
							const String& rDoc );
	virtual			~ScUndoRemoveLink();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	String	aDocName;
	String	aFltName;
	String	aOptions;
	ULONG	nRefreshDelay;
	USHORT	nCount;
	SCTAB*	pTabs;
	BYTE*	pModes;
	String*	pTabNames;

	void DoChange( BOOL bLink ) const;
};


class ScUndoShowHideTab : public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoShowHideTab(
							ScDocShell* pShell,
							SCTAB nNewTab, BOOL bNewShow );
	virtual			~ScUndoShowHideTab();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	SCTAB	nTab;
	BOOL	bShow;

	void DoChange( BOOL bShow ) const;
};

// ============================================================================

/** This class implements undo & redo of document protect & unprotect
    operations. */
class ScUndoDocProtect : public ScSimpleUndo
{
public:
                    ScUndoDocProtect(ScDocShell* pShell, ::std::auto_ptr<ScDocProtection> pProtectSettings);
	virtual			~ScUndoDocProtect();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
    ::std::auto_ptr<ScDocProtection> mpProtectSettings;

	void	DoProtect(bool bProtect);
};

// ============================================================================

/** This class implements undo & redo of both protect and unprotect of
    sheet. */
class ScUndoTabProtect : public ScSimpleUndo
{
public:
                    ScUndoTabProtect(ScDocShell* pShell, SCTAB nTab, 
                                     ::std::auto_ptr<ScTableProtection> pProtectSettings);
	virtual			~ScUndoTabProtect();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
    SCTAB   mnTab;
    ::std::auto_ptr<ScTableProtection> mpProtectSettings;

	void	DoProtect(bool bProtect);
};


class ScUndoPrintRange : public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoPrintRange( ScDocShell* pShell, SCTAB nNewTab,
										ScPrintRangeSaver* pOld, ScPrintRangeSaver* pNew );
	virtual			~ScUndoPrintRange();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	SCTAB				nTab;
	ScPrintRangeSaver*	pOldRanges;
	ScPrintRangeSaver*	pNewRanges;

	void DoChange( BOOL bUndo );
};


class ScUndoScenarioFlags: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoScenarioFlags(
							ScDocShell* pNewDocShell, SCTAB nT,
							const String& rON, const String& rNN,
							const String& rOC, const String& rNC,
							const Color& rOCol, const Color& rNCol,
							USHORT nOF, USHORT nNF );

	virtual			~ScUndoScenarioFlags();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	SCTAB	nTab;
	String	aOldName;
	String	aNewName;
	String	aOldComment;
	String	aNewComment;
	Color	aOldColor;
	Color	aNewColor;
	USHORT	nOldFlags;
	USHORT	nNewFlags;
};


class ScUndoRenameObject: public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoRenameObject(
							ScDocShell* pNewDocShell, const String& rPN,
							const String& rON, const String& rNN );

	virtual			~ScUndoRenameObject();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	String	aPersistName;		// to find object (works only for OLE objects)
	String	aOldName;
	String	aNewName;

	SdrObject*	GetObject();
};


class ScUndoLayoutRTL : public ScSimpleUndo
{
public:
					TYPEINFO();
					ScUndoLayoutRTL( ScDocShell* pShell, SCTAB nNewTab, BOOL bNewRTL );
	virtual			~ScUndoLayoutRTL();

	virtual void	Undo();
	virtual void	Redo();
	virtual void	Repeat(SfxRepeatTarget& rTarget);
	virtual BOOL	CanRepeat(SfxRepeatTarget& rTarget) const;

	virtual String	GetComment() const;

private:
	SCTAB	nTab;
	BOOL	bRTL;

	void DoChange( BOOL bNew );
};


//UNUSED2009-05 class ScUndoSetGrammar : public ScSimpleUndo
//UNUSED2009-05 {
//UNUSED2009-05 public:
//UNUSED2009-05                     TYPEINFO();
//UNUSED2009-05                     ScUndoSetGrammar( ScDocShell* pShell,
//UNUSED2009-05                                       formula::FormulaGrammar::Grammar eGrammar );
//UNUSED2009-05     virtual         ~ScUndoSetGrammar();
//UNUSED2009-05 
//UNUSED2009-05     virtual void    Undo();
//UNUSED2009-05     virtual void    Redo();
//UNUSED2009-05     virtual void    Repeat(SfxRepeatTarget& rTarget);
//UNUSED2009-05     virtual BOOL    CanRepeat(SfxRepeatTarget& rTarget) const;
//UNUSED2009-05 
//UNUSED2009-05     virtual String  GetComment() const;
//UNUSED2009-05 
//UNUSED2009-05 private:
//UNUSED2009-05     formula::FormulaGrammar::Grammar meNewGrammar, meOldGrammar;
//UNUSED2009-05 
//UNUSED2009-05     void DoChange( formula::FormulaGrammar::Grammar eGrammar );
//UNUSED2009-05 };

#endif

