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

#include "global.hxx"
#include "reffact.hxx"
#include "document.hxx"
#include "docfunc.hxx"
#include "scresid.hxx"
#include "globstr.hrc"
#include "namedlg.hrc"

#define _NAMEDLG_CXX
#include "namedlg.hxx"
#undef _NAMEDLG_CXX
#include <vcl/msgbox.hxx>



// defines -------------------------------------------------------------------

#define ABS_SREF		  SCA_VALID \
						| SCA_COL_ABSOLUTE | SCA_ROW_ABSOLUTE | SCA_TAB_ABSOLUTE
#define ABS_DREF		  ABS_SREF \
						| SCA_COL2_ABSOLUTE | SCA_ROW2_ABSOLUTE | SCA_TAB2_ABSOLUTE
#define ABS_SREF3D		ABS_SREF | SCA_TAB_3D
#define ABS_DREF3D		ABS_DREF | SCA_TAB_3D


//============================================================================
// Hilfsklasse: Merken der aktuellen Bereichsoptionen,
// wenn ein Name in der ComboBox gefunden wird.

struct SaveData
{
	SaveData()
		: bCriteria(FALSE),bPrintArea(FALSE),
		  bColHeader(FALSE),bRowHeader(FALSE),
		  bDirty(FALSE) {}

	void Clear()
		{
			aStrSymbol.Erase();
			bCriteria  = bPrintArea =
			bColHeader = bRowHeader = FALSE;
			bDirty = TRUE;
		}

	String	aStrSymbol;
	BOOL	bCriteria:1;
	BOOL	bPrintArea:1;
	BOOL	bColHeader:1;
	BOOL	bRowHeader:1;
	BOOL	bDirty:1;
};

static SaveData* pSaveObj = NULL;

#define SAVE_DATA() \
	pSaveObj->aStrSymbol = aEdAssign.GetText(); 		\
	pSaveObj->bCriteria  = aBtnCriteria.IsChecked();	\
	pSaveObj->bPrintArea = aBtnPrintArea.IsChecked();	\
	pSaveObj->bColHeader = aBtnColHeader.IsChecked();	\
	pSaveObj->bRowHeader = aBtnRowHeader.IsChecked();	\
	pSaveObj->bDirty	 = TRUE;

#define RESTORE_DATA() \
	if ( pSaveObj->bDirty ) 							\
	{													\
		aEdAssign.SetText( pSaveObj->aStrSymbol );		\
		aBtnCriteria.Check( pSaveObj->bCriteria );		\
		aBtnPrintArea.Check( pSaveObj->bPrintArea );	\
		aBtnColHeader.Check( pSaveObj->bColHeader );	\
		aBtnRowHeader.Check( pSaveObj->bRowHeader );	\
		pSaveObj->bDirty = FALSE;						\
	}

#define ERRORBOX(s) ErrorBox(this,WinBits(WB_OK|WB_DEF_OK),s).Execute();


//============================================================================
//	class ScNameDlg

//----------------------------------------------------------------------------

ScNameDlg::ScNameDlg( SfxBindings* pB, SfxChildWindow* pCW, Window* pParent,
					  ScViewData*		ptrViewData,
					  const ScAddress&	aCursorPos )

	:	ScAnyRefDlg	( pB, pCW, pParent, RID_SCDLG_NAMES ),
		//
        aFlName         ( this, ScResId( FL_NAME ) ),
		aEdName 		( this, ScResId( ED_NAME ) ),
		//
        aFlAssign       ( this, ScResId( FL_ASSIGN ) ),
        aEdAssign       ( this, this, ScResId( ED_ASSIGN ) ),
		aRbAssign		( this, ScResId( RB_ASSIGN ), &aEdAssign, this ),
		//
		aFlType 		( this, ScResId( FL_TYPE ) ),
		aBtnPrintArea	( this, ScResId( BTN_PRINTAREA ) ),
		aBtnColHeader	( this, ScResId( BTN_COLHEADER ) ),
		aBtnCriteria	( this, ScResId( BTN_CRITERIA ) ),
		aBtnRowHeader	( this, ScResId( BTN_ROWHEADER ) ),
		//
		aBtnOk			( this, ScResId( BTN_OK ) ),
		aBtnCancel		( this, ScResId( BTN_CANCEL ) ),
		aBtnHelp		( this, ScResId( BTN_HELP ) ),
		aBtnAdd 		( this, ScResId( BTN_ADD ) ),
		aBtnRemove		( this, ScResId( BTN_REMOVE ) ),
		aBtnMore		( this, ScResId( BTN_MORE ) ),
		//
		bSaved			(FALSE),
		aStrAdd 		( ScResId( STR_ADD ) ),
		aStrModify		( ScResId( STR_MODIFY ) ),
		errMsgInvalidSym( ScResId( STR_INVALIDSYMBOL ) ),
		//
		pViewData		( ptrViewData ),
		pDoc			( ptrViewData->GetDocument() ),
		aLocalRangeName ( *(pDoc->GetRangeName()) ),
		theCursorPos	( aCursorPos )  // zum Berechnen der Referenzen
{
	pSaveObj = new SaveData;
	Init();
	FreeResource();
}


//----------------------------------------------------------------------------

__EXPORT ScNameDlg::~ScNameDlg()
{
	DELETEZ( pSaveObj );
}


//----------------------------------------------------------------------------

void __EXPORT ScNameDlg::Init()
{
	String	aAreaStr;
	ScRange aRange;

	DBG_ASSERT( pViewData && pDoc, "ViewData oder Document nicht gefunden!" );

	aBtnOk.SetClickHdl		( LINK( this, ScNameDlg, OkBtnHdl ) );
	aBtnCancel.SetClickHdl	( LINK( this, ScNameDlg, CancelBtnHdl ) );
	aBtnAdd.SetClickHdl		( LINK( this, ScNameDlg, AddBtnHdl ) );
	aBtnRemove.SetClickHdl	( LINK( this, ScNameDlg, RemoveBtnHdl ) );
	aEdAssign.SetGetFocusHdl( LINK( this, ScNameDlg, AssignGetFocusHdl ) );
	aEdAssign.SetModifyHdl	( LINK( this, ScNameDlg, EdModifyHdl ) );
	aEdName.SetModifyHdl 	( LINK( this, ScNameDlg, EdModifyHdl ) );
	aEdName.SetSelectHdl 	( LINK( this, ScNameDlg, NameSelectHdl ) );

	aBtnCriteria .Hide();
	aBtnPrintArea.Hide();
	aBtnColHeader.Hide();
	aBtnRowHeader.Hide();

	aBtnMore.AddWindow( &aFlType );
	aBtnMore.AddWindow( &aBtnCriteria );
	aBtnMore.AddWindow( &aBtnPrintArea );
	aBtnMore.AddWindow( &aBtnColHeader );
	aBtnMore.AddWindow( &aBtnRowHeader );

	UpdateNames();

	pViewData->GetSimpleArea( aRange );
	aRange.Format( aAreaStr, ABS_DREF3D, pDoc,
                   ScAddress::Details(pDoc->GetAddressConvention(), 0, 0) );

	theCurSel = Selection( 0, SELECTION_MAX );
	aEdAssign.GrabFocus();
	aEdAssign.SetText( aAreaStr );
	aEdAssign.SetSelection( theCurSel );
	aEdName.GrabFocus();

	aBtnAdd.Disable();
	aBtnRemove.Disable();
	if ( aEdName.GetEntryCount() > 0 )
		aBtnAdd.SetText( aStrAdd );
	UpdateChecks();
	EdModifyHdl( 0 );

	bSaved=TRUE;
	SAVE_DATA()

	//@BugID 54702
	//SFX_APPWINDOW->Disable(FALSE);		//! allgemeine Methode im ScAnyRefDlg
}

//----------------------------------------------------------------------------
BOOL ScNameDlg::IsRefInputMode() const
{
	return aEdAssign.IsEnabled();
}

void ScNameDlg::RefInputDone( BOOL bForced)
{
	ScAnyRefDlg::RefInputDone(bForced);
	EdModifyHdl(&aEdAssign);
}
//----------------------------------------------------------------------------
// Uebergabe eines mit der Maus selektierten Tabellenbereiches, der dann als
// neue Selektion im Referenz-Edit angezeigt wird.


void ScNameDlg::SetReference( const ScRange& rRef, ScDocument* pDocP )
{
	if ( aEdAssign.IsEnabled() )
	{
		if ( rRef.aStart != rRef.aEnd )
			RefInputStart(&aEdAssign);
		String aRefStr;
        rRef.Format( aRefStr, ABS_DREF3D, pDocP, 
                     ScAddress::Details(pDocP->GetAddressConvention(), 0, 0) );
		aEdAssign.SetRefString( aRefStr );
	}
}


//----------------------------------------------------------------------------
BOOL __EXPORT ScNameDlg::Close()
{
	return DoClose( ScNameDlgWrapper::GetChildWindowId() );
}


//----------------------------------------------------------------------------

void ScNameDlg::SetActive()
{
	aEdAssign.GrabFocus();
	RefInputDone();
}


//----------------------------------------------------------------------------

void __EXPORT ScNameDlg::UpdateChecks()
{
	USHORT		 nCurPos=0;

	if(aLocalRangeName.SearchName( aEdName.GetText(), nCurPos))
	{
		ScRangeData* pData=(ScRangeData*)(aLocalRangeName.At( nCurPos ));
		aBtnCriteria .Check( pData->HasType( RT_CRITERIA ) );
		aBtnPrintArea.Check( pData->HasType( RT_PRINTAREA ) );
		aBtnColHeader.Check( pData->HasType( RT_COLHEADER ) );
		aBtnRowHeader.Check( pData->HasType( RT_ROWHEADER ) );
	}

	// Falls Edit-Feld leer ist: Typ-CheckBoxen deaktivieren:

	if ( aEdName.GetText().Len() != 0 )
	{
		if ( !aFlType.IsEnabled() )
		{
			aFlType 	 .Enable();
			aBtnCriteria .Enable();
			aBtnPrintArea.Enable();
			aBtnColHeader.Enable();
			aBtnRowHeader.Enable();
            aFlAssign    .Enable();
			aEdAssign    .Enable();
			aRbAssign    .Enable();
		}
	}
	else if ( aFlType.IsEnabled() )
	{
		aFlType 	 .Disable();
		aBtnCriteria .Disable();
		aBtnPrintArea.Disable();
		aBtnColHeader.Disable();
		aBtnRowHeader.Disable();
        aFlAssign    .Disable();
		aEdAssign    .Disable();
		aRbAssign    .Disable();
	}
}


//----------------------------------------------------------------------------

void __EXPORT ScNameDlg::UpdateNames()
{
	USHORT	nRangeCount = aLocalRangeName.GetCount();

	aEdName.SetUpdateMode( FALSE );
	//-----------------------------------------------------------
	USHORT  nNamePos = aEdName.GetTopEntry();
	aEdName.Clear();

	aEdAssign.SetText( EMPTY_STRING );

	if ( nRangeCount > 0 )
	{
		ScRangeData*	pRangeData = NULL;
		String			aString;

		for ( USHORT i=0; i<nRangeCount; i++ )
		{
			pRangeData = (ScRangeData*)(aLocalRangeName.At( i ));
			if ( pRangeData )
			{
				if (   !pRangeData->HasType( RT_DATABASE )
					&& !pRangeData->HasType( RT_SHARED ) )
				{
					pRangeData->GetName( aString );
					aEdName.InsertEntry( aString );
				}
			}
		}
	}
	else
	{
		aBtnAdd.SetText( aStrAdd );
		aBtnAdd.Disable();
		aBtnRemove.Disable();
	}
	//-----------------------------------------------------------
	aEdName.SetUpdateMode( TRUE );
	aEdName.SetTopEntry(nNamePos);
	aEdName.Invalidate();
}


//----------------------------------------------------------------------------

void __EXPORT ScNameDlg::CalcCurTableAssign( String& aAssign, USHORT nCurPos )
{
	ScRangeData* pRangeData = (ScRangeData*)(aLocalRangeName.At( nCurPos ));

	if ( pRangeData )
	{
        rtl::OUStringBuffer sBuffer;
		pRangeData->UpdateSymbol( sBuffer, theCursorPos );
        aAssign = sBuffer;
	}
	else
	{
		aAssign.Erase();
	}
}


//----------------------------------------------------------------------------
// Handler:
// ========

IMPL_LINK( ScNameDlg, OkBtnHdl, void *, EMPTYARG )
{
	if ( aBtnAdd.IsEnabled() )
		AddBtnHdl( 0 );

	if ( !aBtnAdd.IsEnabled() && !aBtnRemove.IsEnabled() )
	{
		ScDocShell* pDocSh = pViewData->GetDocShell();
		ScDocFunc aFunc(*pDocSh);
		aFunc.ModifyRangeNames( aLocalRangeName, FALSE );
		Close();
	}
	return 0;
}


//----------------------------------------------------------------------------

IMPL_LINK_INLINE_START( ScNameDlg, CancelBtnHdl, void *, EMPTYARG )
{
	Close();
	return 0;
}
IMPL_LINK_INLINE_END( ScNameDlg, CancelBtnHdl, void *, EMPTYARG )


//----------------------------------------------------------------------------

IMPL_LINK( ScNameDlg, AddBtnHdl, void *, EMPTYARG )
{
	BOOL	bAdded	  = FALSE;
	String	aNewEntry = aEdName.GetText();
	USHORT  nNamePos = aEdName.GetTopEntry();
	aNewEntry.EraseLeadingChars( ' ' );
	aNewEntry.EraseTrailingChars( ' ' );

	if ( aNewEntry.Len() > 0 )
	{
		if ( ScRangeData::IsNameValid( aNewEntry, pDoc ) )
		{
			if ( pDoc )
			{
				ScRangeData*	pNewEntry	= NULL;
				RangeType		nType		= RT_NAME;
				USHORT			nFoundAt	= 0;
				String			theSymbol	= aEdAssign.GetText();
				String			aStrPos;
				String			aStrArea;

				pNewEntry = new ScRangeData( pDoc,
											 aNewEntry,
											 theSymbol,
											 theCursorPos,
											 nType );
				if (pNewEntry)
				{
					nType = nType
					| (aBtnRowHeader .IsChecked() ? RT_ROWHEADER  : RangeType(0))
					| (aBtnColHeader .IsChecked() ? RT_COLHEADER  : RangeType(0))
					| (aBtnPrintArea .IsChecked() ? RT_PRINTAREA  : RangeType(0))
					| (aBtnCriteria  .IsChecked() ? RT_CRITERIA   : RangeType(0));
					pNewEntry->AddType(nType);
				}

				// theSymbol gueltig?
				// (= konnte theSymbol im ScRangeData-Ctor
				//	  in ein Token-Array uebersetzt werden?)
				if ( 0 == pNewEntry->GetErrCode() )
				{
					// Eintrag bereits vorhanden? Dann vorher entfernen (=Aendern)
					if ( aLocalRangeName.SearchName( aNewEntry, nFoundAt ) )
					{									// alten Index uebernehmen
						pNewEntry->SetIndex(
							((ScRangeData*)(aLocalRangeName.At(nFoundAt)))->GetIndex() );
						aLocalRangeName.AtFree( nFoundAt );
					}
					else
						pSaveObj->Clear();

					if ( !aLocalRangeName.Insert( pNewEntry ) )
						delete pNewEntry;

					UpdateNames();
					bSaved=FALSE;
					RESTORE_DATA()
					aEdName.SetText(EMPTY_STRING);
					aEdName.GrabFocus();
					UpdateChecks();
					aBtnAdd.SetText( aStrAdd );
					aBtnAdd.Disable();
					aBtnRemove.Disable();

					//@BugID 54702 raus mit dem Sch.
					//SFX_APPWINDOW->Disable(FALSE);		//! allgemeine Methode im ScAnyRefDlg

					bAdded = TRUE;
				}
				else // theSymbol ungueltig
				{
					delete pNewEntry;
					ERRORBOX( errMsgInvalidSym );
					theCurSel = Selection( 0, SELECTION_MAX );
					aEdAssign.GrabFocus();
				}
			}
		}
		else
		{
			ERRORBOX( ScGlobal::GetRscString(STR_INVALIDNAME) );
			aEdName.SetSelection( Selection( 0, SELECTION_MAX ) );
			aEdName.GrabFocus();
		}
	}

    aEdName.SetTopEntry(nNamePos);
	return bAdded;
}


//----------------------------------------------------------------------------

IMPL_LINK( ScNameDlg, RemoveBtnHdl, void *, EMPTYARG )
{
	USHORT		 nRemoveAt = 0;
	const String aStrEntry = aEdName.GetText();

	if ( aLocalRangeName.SearchName( aStrEntry, nRemoveAt ) )
	{
		String aStrDelMsg = ScGlobal::GetRscString( STR_QUERY_DELENTRY );
		String aMsg		  = aStrDelMsg.GetToken( 0, '#' );

		aMsg += aStrEntry;
		aMsg += aStrDelMsg.GetToken( 1, '#' );

		if ( RET_YES ==
			 QueryBox( this, WinBits( WB_YES_NO | WB_DEF_YES ), aMsg ).Execute() )
		{
			aLocalRangeName.AtFree( nRemoveAt );
			UpdateNames();
			UpdateChecks();
			bSaved=FALSE;
			RESTORE_DATA()
			theCurSel = Selection( 0, SELECTION_MAX );
			aBtnAdd.SetText( aStrAdd );
			aBtnAdd.Disable();
			aBtnRemove.Disable();
		}
	}
	return 0;
}


//----------------------------------------------------------------------------

IMPL_LINK( ScNameDlg, NameSelectHdl, void *, EMPTYARG )
{
	USHORT nAtPos;

	if ( aLocalRangeName.SearchName( aEdName.GetText(), nAtPos ) )
	{
		String		 aSymbol;
		ScRangeData* pData	= (ScRangeData*)(aLocalRangeName.At( nAtPos ));

		if ( pData )
		{
			pData->GetSymbol( aSymbol );
			CalcCurTableAssign( aSymbol, nAtPos );
			aEdAssign.SetText( aSymbol );
			aBtnAdd.SetText( aStrModify );
			theCurSel = Selection( 0, SELECTION_MAX );
		}
	}
	UpdateChecks();
	return 0;
}


//----------------------------------------------------------------------------

IMPL_LINK( ScNameDlg, EdModifyHdl, Edit *, pEd )
{
	String	theName 	= aEdName.GetText();
	String	theSymbol	= aEdAssign.GetText();
	BOOL	bNameFound	= (COMBOBOX_ENTRY_NOTFOUND
						   != aEdName.GetEntryPos( theName ));

	if ( pEd == &aEdName )
	{
		if ( theName.Len() == 0 )
		{
			if ( aBtnAdd.GetText() != aStrAdd )
				aBtnAdd.SetText( aStrAdd );
			aBtnAdd.Disable();
			aBtnRemove.Disable();
            aFlAssign.Disable();
			aEdAssign.Disable();
			aRbAssign.Disable();
			//@BugID 54702 raus mit dem Sch.
			//SFX_APPWINDOW->Disable(FALSE);		//! allgemeine Methode im ScAnyRefDlg
		}
		else
		{
			if ( bNameFound )
			{
				if ( aBtnAdd.GetText() != aStrModify )
					aBtnAdd.SetText( aStrModify );

				aBtnRemove.Enable();

				if(!bSaved)
				{
					bSaved=TRUE;
					SAVE_DATA()
				}
				NameSelectHdl( 0 );
			}
			else
			{
				if ( aBtnAdd.GetText() != aStrAdd )
					aBtnAdd.SetText( aStrAdd );
				aBtnRemove.Disable();

				bSaved=FALSE;
				RESTORE_DATA()
			}
			theSymbol = aEdAssign.GetText();

			if ( theSymbol.Len() > 0 )
				aBtnAdd.Enable();
			else
				aBtnAdd.Disable();

            aFlAssign.Enable();
			aEdAssign.Enable();
			aRbAssign.Enable();
			//@BugID 54702 raus mit dem Sch.
			//SFX_APPWINDOW->Enable();
		}
		UpdateChecks();
		theCurSel = Selection( 0, SELECTION_MAX );
	}
	else if ( pEd == &aEdAssign )
	{
		if ( (theName.Len()>0) && (theSymbol.Len()>0) )
		{
			aBtnAdd.Enable();
			if ( bNameFound )
				aBtnRemove.Enable();
		}
		else
		{
			aBtnAdd.Disable();
			aBtnRemove.Disable();
		}
	}
	return 0;
}

//------------------------------------------------------------------------

IMPL_LINK_INLINE_START( ScNameDlg, AssignGetFocusHdl, void *, EMPTYARG )
{
	EdModifyHdl( &aEdAssign );
	return 0;
}
IMPL_LINK_INLINE_END( ScNameDlg, AssignGetFocusHdl, void *, EMPTYARG )


