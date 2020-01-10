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

// System - Includes ---------------------------------------------------------



// INCLUDE -------------------------------------------------------------------

#include <vcl/msgbox.hxx>
#include <sfx2/app.hxx>

#include "reffact.hxx"
#include "document.hxx"
#include "scresid.hxx"
#include "globstr.hrc"
#include "simpref.hrc"
#include "rangenam.hxx"		// IsNameValid
#include "simpref.hxx"
#include "scmod.hxx"

//============================================================================

#define ABS_SREF		  SCA_VALID \
						| SCA_COL_ABSOLUTE | SCA_ROW_ABSOLUTE | SCA_TAB_ABSOLUTE
#define ABS_DREF		  ABS_SREF \
						| SCA_COL2_ABSOLUTE | SCA_ROW2_ABSOLUTE | SCA_TAB2_ABSOLUTE
#define ABS_SREF3D		ABS_SREF | SCA_TAB_3D
#define ABS_DREF3D		ABS_DREF | SCA_TAB_3D

//----------------------------------------------------------------------------

#define ERRORBOX(s) ErrorBox(this,WinBits(WB_OK|WB_DEF_OK),s).Execute()
#define QUERYBOX(m) QueryBox(this,WinBits(WB_YES_NO|WB_DEF_YES),m).Execute()

//============================================================================
//	class ScSimpleRefDlg

//----------------------------------------------------------------------------
ScSimpleRefDlg::ScSimpleRefDlg( SfxBindings* pB, SfxChildWindow* pCW, Window* pParent,
						  ScViewData*	ptrViewData )

	:	ScAnyRefDlg	( pB, pCW, pParent, RID_SCDLG_SIMPLEREF ),
		//
		aFtAssign		( this, ScResId( FT_ASSIGN ) ),
        aEdAssign       ( this, this, ScResId( ED_ASSIGN ) ),
		aRbAssign		( this, ScResId( RB_ASSIGN ), &aEdAssign, this ),

		aBtnOk			( this, ScResId( BTN_OK ) ),
		aBtnCancel		( this, ScResId( BTN_CANCEL ) ),
		aBtnHelp		( this, ScResId( BTN_HELP ) ),

		//
		pViewData		( ptrViewData ),
		pDoc			( ptrViewData->GetDocument() ),
		bRefInputMode	( FALSE ),
		bAutoReOpen		( TRUE ),
		bCloseOnButtonUp( FALSE ),
        bSingleCell     ( FALSE ),
        bMultiSelection ( FALSE )
{
	//	damit die Strings in der Resource bei den FixedTexten bleiben koennen:
	Init();
	FreeResource();
	SetDispatcherLock( TRUE ); // Modal-Modus einschalten
}

//----------------------------------------------------------------------------
__EXPORT ScSimpleRefDlg::~ScSimpleRefDlg()
{
	SetDispatcherLock( FALSE ); // Modal-Modus einschalten
}

//----------------------------------------------------------------------------
void ScSimpleRefDlg::FillInfo(SfxChildWinInfo& rWinInfo) const
{
	ScAnyRefDlg::FillInfo(rWinInfo);
	rWinInfo.bVisible=bAutoReOpen;
}

//----------------------------------------------------------------------------
void ScSimpleRefDlg::SetRefString(const String &rStr)
{
	aEdAssign.SetText(rStr);
}

//----------------------------------------------------------------------------
void ScSimpleRefDlg::Init()
{
	aBtnOk.SetClickHdl		( LINK( this, ScSimpleRefDlg, OkBtnHdl ) );
	aBtnCancel.SetClickHdl	( LINK( this, ScSimpleRefDlg, CancelBtnHdl ) );
	bCloseFlag=FALSE;
}

//----------------------------------------------------------------------------
// Uebergabe eines mit der Maus selektierten Tabellenbereiches, der dann als
//  neue Selektion im Referenz-Fenster angezeigt wird.
void ScSimpleRefDlg::SetReference( const ScRange& rRef, ScDocument* pDocP )
{
	if ( aEdAssign.IsEnabled() )
	{
		if ( rRef.aStart != rRef.aEnd )
			RefInputStart( &aEdAssign );

		theCurArea = rRef;
		String aRefStr;
		if ( bSingleCell )
		{
			ScAddress aAdr = rRef.aStart;
            aAdr.Format( aRefStr, SCA_ABS_3D, pDocP, pDocP->GetAddressConvention() );
		}
		else
            theCurArea.Format( aRefStr, ABS_DREF3D, pDocP, pDocP->GetAddressConvention() );

        if ( bMultiSelection )
        {
            String aVal = aEdAssign.GetText();
            Selection aSel = aEdAssign.GetSelection();
            aSel.Justify();
            aVal.Erase( (xub_StrLen)aSel.Min(), (xub_StrLen)aSel.Len() );
            aVal.Insert( aRefStr, (xub_StrLen)aSel.Min() );
            Selection aNewSel( aSel.Min(), aSel.Min()+aRefStr.Len() );
            aEdAssign.SetRefString( aVal );
            aEdAssign.SetSelection( aNewSel );
        }
        else
            aEdAssign.SetRefString( aRefStr );

		aChangeHdl.Call( &aRefStr );
	}
}


//----------------------------------------------------------------------------
BOOL __EXPORT ScSimpleRefDlg::Close()
{
	CancelBtnHdl(&aBtnCancel);
	return TRUE;
}

//------------------------------------------------------------------------
void ScSimpleRefDlg::SetActive()
{
	aEdAssign.GrabFocus();

	//	kein NameModifyHdl, weil sonst Bereiche nicht geaendert werden koennen
	//	(nach dem Aufziehen der Referenz wuerde der alte Inhalt wieder angezeigt)
	//	(der ausgewaehlte DB-Name hat sich auch nicht veraendert)

	RefInputDone();
}
//------------------------------------------------------------------------
BOOL ScSimpleRefDlg::IsRefInputMode() const
{
	return TRUE;
}

String ScSimpleRefDlg::GetRefString() const
{
	return aEdAssign.GetText();
}

void ScSimpleRefDlg::SetCloseHdl( const Link& rLink )
{
	aCloseHdl=rLink;
}

void ScSimpleRefDlg::SetUnoLinks( const Link& rDone, const Link& rAbort,
									const Link& rChange )
{
	aDoneHdl	= rDone;
	aAbortedHdl	= rAbort;
	aChangeHdl	= rChange;
}

void ScSimpleRefDlg::SetFlags( BOOL bSetCloseOnButtonUp, BOOL bSetSingleCell, BOOL bSetMultiSelection )
{
	bCloseOnButtonUp = bSetCloseOnButtonUp;
	bSingleCell = bSetSingleCell;
	bMultiSelection = bSetMultiSelection;
}

void ScSimpleRefDlg::StartRefInput()
{
    if ( bMultiSelection )
    {
        // initially select the whole string, so it gets replaced by default
        aEdAssign.SetSelection( Selection( 0, aEdAssign.GetText().Len() ) );
    }

	aRbAssign.DoRef();
	bCloseFlag=TRUE;
}

void ScSimpleRefDlg::RefInputDone( BOOL bForced)
{
	ScAnyRefDlg::RefInputDone(bForced);
	if ( (bForced || bCloseOnButtonUp) && bCloseFlag )
		OkBtnHdl(&aBtnOk);
}
//------------------------------------------------------------------------
// Handler:
// ========
IMPL_LINK( ScSimpleRefDlg, OkBtnHdl, void *, EMPTYARG )
{
	bAutoReOpen=FALSE;
	String aResult=aEdAssign.GetText();
	aCloseHdl.Call(&aResult);
	Link aUnoLink = aDoneHdl;		// stack var because this is deleted in DoClose
	DoClose( ScSimpleRefDlgWrapper::GetChildWindowId() );
	aUnoLink.Call( &aResult );
	return 0;
}

//------------------------------------------------------------------------
IMPL_LINK( ScSimpleRefDlg, CancelBtnHdl, void *, EMPTYARG )
{
	bAutoReOpen=FALSE;
	String aResult=aEdAssign.GetText();
	aCloseHdl.Call(NULL);
	Link aUnoLink = aAbortedHdl;	// stack var because this is deleted in DoClose
	DoClose( ScSimpleRefDlgWrapper::GetChildWindowId() );
	aUnoLink.Call( &aResult );
	return 0;
}



//------------------------------------------------------------------------

