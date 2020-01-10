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

#ifndef _TEXTEDIT_HXX
#define _TEXTEDIT_HXX

class AppEdit;
class TextEngine;
class TextView;
class TextEdit;
class BreakpointWindow;

#include <vcl/timer.hxx>
#include <tools/table.hxx>
#include <tools/debug.hxx>
#include <svtools/lstner.hxx>
#include <svtools/svmedit.hxx>

#include "dataedit.hxx"

//#include <xtextedt.hxx>

class TextEditImp : public Window, public SfxListener
{
using Window::Notify;

protected:
	void			DoSyntaxHighlight( ULONG nPara );


private:
	AppEdit *pAppEdit;
	Link ModifyHdl;

	Timer			aSyntaxIdleTimer;
	Timer			aImplSyntaxIdleTimer;
	DECL_LINK( SyntaxTimerHdl, Timer * );
	Table			aSyntaxLineTable;

	void Notify( SfxBroadcaster& rBC, const SfxHint& rHint );

	void			ImpDoHighlight( const String& rSource, ULONG nLineOff );
	BOOL			bHighlightning;
	BOOL			bDoSyntaxHighlight;
	BOOL			bDelayHighlight;


	SbxBase* GetSbxAtMousePos( String &aWord );
	virtual void MouseMove( const MouseEvent& rMEvt );
	DECL_LINK( HideVarContents, void* );
	DECL_LINK( ShowVarContents, void* );
	Point aTipPos;
	String aTipWord;
	ULONG nTipId;

	Timer HideTipTimer;
	Timer ShowTipTimer;

    BOOL bViewMoved;

public:
	TextEditImp( AppEdit *pParent, const WinBits& aBits );
	~TextEditImp();

	TextEngine *pTextEngine;
	TextView *pTextView;

	void SetFont( const Font& rNewFont );
	BOOL IsModified();
	void SetModifyHdl( Link l ){ ModifyHdl = l; }

	void				KeyInput( const KeyEvent& rKeyEvent );
	void				Paint( const Rectangle& rRect );
	void				MouseButtonUp( const MouseEvent& rMouseEvent );
	void				MouseButtonDown( const MouseEvent& rMouseEvent );
//	void				MouseMove( const MouseEvent& rMouseEvent );
	void				Command( const CommandEvent& rCEvt );
	//BOOL				Drop( const DropEvent& rEvt );
	//BOOL				QueryDrop( DropEvent& rEvt );

    BOOL				ViewMoved();

	void DoDelayedSyntaxHighlight( xub_StrLen nPara );
	void InvalidateSyntaxHighlight();
	void SyntaxHighlight( BOOL bNew );
	void BuildKontextMenu( PopupMenu *&pMenu );
};



DBG_NAMEEX(TextEdit)
class TextEdit : public DataEdit {

	BreakpointWindow	*pBreakpointWindow;
	BOOL bFileWasUTF8;
	BOOL bSaveAsUTF8;

public:
	TextEdit( AppEdit*, const WinBits& );
	~TextEdit();
	void Highlight( ULONG nLine, xub_StrLen nCol1, xub_StrLen nCol2 );
	TextEditImp& GetTextEditImp() { return aEdit; }

	void				SetBreakpointWindow( BreakpointWindow *pBPWindow ){ pBreakpointWindow = pBPWindow; }
	BreakpointWindow	*GetBreakpointWindow(){ return pBreakpointWindow; }

	DATA_FUNC_DEF( aEdit, TextEditImp )

	virtual void BuildKontextMenu( PopupMenu *&pMenu );

	void SaveAsUTF8( BOOL bUTF8 ) { bSaveAsUTF8 = bUTF8; }
};

#endif
