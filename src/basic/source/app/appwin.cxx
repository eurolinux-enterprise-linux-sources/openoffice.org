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
#include "precompiled_basic.hxx"

#include <stdio.h>
#ifndef _MSGBOX_HXX //autogen
#include <vcl/msgbox.hxx>
#endif
#include <tools/fsys.hxx>
#include <svtools/stringtransfer.hxx>

#include "basic.hrc"
#include "app.hxx"
#include <basic/mybasic.hxx>
#include "status.hxx"
#include "appwin.hxx"
#include "dataedit.hxx"
#include "dialogs.hxx"
#include "basrid.hxx"

String *AppWin::pNoName = NULL;	// contains the "Untitled"-String
short AppWin::nNumber = 0;      // consecutive number
short AppWin::nCount = 0;       // number of edit windows

TYPEINIT0(AppWin);
AppWin::AppWin( BasicFrame* pParent )
: DockingWindow( pParent, WB_SIZEMOVE | WB_CLOSEABLE | WB_PINABLE )
, nSkipReload(0)
, bHasFile( FALSE )
, bReloadAborted( FALSE )
, pFrame( pParent )
, bFind( TRUE )
, pDataEdit(NULL)
{
	// Load the Untitled string if not yet loaded
	if( !pNoName )
		pNoName = new String( SttResId( IDS_NONAME ) );
	nCount++;

	// Get maximized state from current window
	USHORT nInitialWinState;
	if ( pFrame->pWork )
	{
		nInitialWinState = pFrame->pWork->GetWinState();
		nInitialWinState &= TT_WIN_STATE_MAX | TT_WIN_STATE_FLOAT;
	}
	else
		nInitialWinState = TT_WIN_STATE_MAX;

	StartListening( *pFrame );
	pFrame->AddWindow( this );

	ShowTitleButton( TITLE_BUTTON_DOCKING );
	ShowTitleButton( TITLE_BUTTON_HIDE );
	SetActivateMode( ACTIVATE_MODE_GRABFOCUS );

	Cascade( nCount );
	if ( TT_WIN_STATE_MAX == nInitialWinState )
		Maximize();
}

AppWin::~AppWin()
{
	nCount--;
	pFrame->RemoveWindow( this );
	pFrame = NULL; // Set to stop setting window text after BasicRun
}

void AppWin::SetText( const XubString& rStr )
{
	DockingWindow::SetText( rStr );
	pFrame->WindowRenamed( this );
}

void AppWin::TitleButtonClick( USHORT nButton )
{
	if ( TITLE_BUTTON_DOCKING == nButton )
		if ( TT_WIN_STATE_MAX != nWinState )
			Maximize();
		else
			Restore();
	else // if ( TITLE_BUTTON_HIDE == nButton )
		Minimize( TRUE );
}

void AppWin::Maximize()
{
	if ( TT_WIN_STATE_MAX != nWinState )
	{
		nNormalPos = GetPosPixel();
		nNormalSize = GetSizePixel();

		SetFloatingMode( FALSE );

		pFrame->nMaximizedWindows++;
		nWinState = TT_WIN_STATE_MAX;
	}
	sal_Int32 nTitleHeight;
	{
		sal_Int32 nDummy1, nDummy2, nDummy3;
		pFrame->GetBorder( nDummy1, nTitleHeight, nDummy2, nDummy3 );
	}

	Size aSize = pFrame->GetOutputSizePixel();
	aSize.Height() -= nTitleHeight;
	aSize.Height() -= 2;
	aSize.Width() -= 2;
	SetSizePixel( aSize );
	SetPosPixel( Point( 1,1 ) );
	pFrame->WinMax_Restore();
}

void AppWin::Restore()
{
	SetFloatingMode( TRUE );
	SetPosSizePixel( nNormalPos, nNormalSize );

	if ( TT_WIN_STATE_MAX == nWinState )
		pFrame->nMaximizedWindows--;

	nWinState = TT_WIN_STATE_FLOAT;
	pFrame->WinMax_Restore();
}

void AppWin::Minimize( BOOL bMinimize )
{
	if ( bMinimize )
		nWinState |= TT_WIN_STATE_HIDE;
	else
		nWinState &= ~TT_WIN_STATE_HIDE;
	pFrame->WinMax_Restore();
}

void AppWin::Cascade( USHORT nNr )
{
	Restore();

	nNr--;
	nNr %= 10;
	nNr++;

	sal_Int32 nTitleHeight;
	{
		sal_Int32 nDummy1, nDummy2, nDummy3;
		pFrame->GetBorder( nDummy1, nTitleHeight, nDummy2, nDummy3 );
	}

	Size aWinSize = pFrame->GetOutputSizePixel();
	aWinSize.Width() -= aWinSize.Width() / 5;   // reduce to 80 %
	aWinSize.Height() -= nTitleHeight * nNr;    // snip height to appropriate value
	aWinSize.Height() -= 2;

	Point nPos( nTitleHeight * nNr, nTitleHeight * nNr );

	SetPosSizePixel( nPos, aWinSize );
}

void AppWin::RequestHelp( const HelpEvent& )
{
	Help();
}

void AppWin::Help()
{
  String s = pDataEdit->GetSelected();
  if( s.Len() > 0 ) 
  {
    // Trim leading whitespaces
    while( s.GetChar(0) == ' ' ) 
      s.Erase( 0, 1 );
//		aBasicApp.pHelp->Start( s );
  } 
  else 
  {
//		aBasicApp.pHelp->Start( OOO_HELP_INDEX );
  }
}

void AppWin::Resize()
{
	if( pDataEdit )
	{
		pDataEdit->SetPosPixel( Point( 0, 0 ) );
		pDataEdit->SetSizePixel( GetOutputSizePixel() );
	}
}

void AppWin::GetFocus()
{
	pFrame->FocusWindow( this );
	if( pDataEdit ) // GetFocus is called by the destructor, so this check
	{
		pDataEdit->GrabFocus();
//		InitMenu(GetpApp()->GetAppMenu()->GetPopupMenu( RID_APPEDIT ));
	}
}

long AppWin::PreNotify( NotifyEvent& rNEvt )
{

	if ( rNEvt.GetType() == EVENT_MOUSEBUTTONDOWN )
		Activate();
	if ( rNEvt.GetType() == EVENT_GETFOCUS )
		if ( pFrame->pList->Last() != this )
			Activate();
	return FALSE;		// Der event soll weiter verarbeitet werden
}

void AppWin::Activate()
{
	GrabFocus();
}

// Set up the menu
long AppWin::InitMenu( Menu* pMenu )
{

	::rtl::OUString aTemp;
	BOOL bMarked;
	if( pDataEdit )
    {
        TextSelection r = pDataEdit->GetSelection();
        bMarked = r.HasRange();
    }
    else
        bMarked = FALSE;
	pMenu->EnableItem( RID_EDITREPEAT,	(aFind.Len() != 0 ) );
	pMenu->EnableItem( RID_EDITCUT, 	bMarked );
	pMenu->EnableItem( RID_EDITCOPY,	bMarked );
	pMenu->EnableItem( RID_EDITPASTE,	( ::svt::OStringTransfer::PasteString( aTemp, this ) ) );
	pMenu->EnableItem( RID_EDITDEL, 	bMarked );
//	pMenu->EnableItem( RID_HELPTOPIC,	bMarked );

	BOOL bHasText;
	if( pDataEdit )
        bHasText = pDataEdit->HasText();
    else
        bHasText = FALSE;
	BOOL bRunning = pFrame->Basic().IsRunning();
	BOOL bCanExecute = BOOL( (!bRunning && bHasText) || pFrame->bInBreak );
	pMenu->EnableItem( RID_RUNSTART,	bCanExecute );
	pMenu->EnableItem( RID_RUNBREAK, 	bRunning && !pFrame->bInBreak);
	pMenu->EnableItem( RID_RUNSTOP, 	bRunning );
	pMenu->EnableItem( RID_RUNTOCURSOR, bCanExecute );
	pMenu->EnableItem( RID_RUNSTEPINTO, bCanExecute );
	pMenu->EnableItem( RID_RUNSTEPOVER, bCanExecute );
	return TRUE;
}

long AppWin::DeInitMenu( Menu* pMenu )
{
	pMenu->EnableItem( RID_EDITREPEAT );
	pMenu->EnableItem( RID_EDITCUT );
	pMenu->EnableItem( RID_EDITCOPY );
	pMenu->EnableItem( RID_EDITPASTE );
	pMenu->EnableItem( RID_EDITDEL );

	pMenu->EnableItem( RID_RUNSTART );
	pMenu->EnableItem( RID_RUNBREAK );
	pMenu->EnableItem( RID_RUNSTOP );
	pMenu->EnableItem( RID_RUNTOCURSOR );
	pMenu->EnableItem( RID_RUNSTEPINTO );
	pMenu->EnableItem( RID_RUNSTEPOVER );
	return TRUE;
}

// Menu Handler

void AppWin::Command( const CommandEvent& rCEvt )
{
	TextSelection r  = pDataEdit->GetSelection();
	BOOL bHasMark = r.HasRange();
	switch( rCEvt.GetCommand() ) {
		case RID_FILESAVE:
			QuerySave( QUERY_DISK_CHANGED | SAVE_NOT_DIRTY ); break;
		case RID_FILESAVEAS:
			SaveAs(); break;
		case RID_EDITSEARCH:
			Find(); break;
		case RID_EDITREPLACE:
			Replace(); break;
		case RID_EDITREPEAT:
			Repeat(); break;
		case RID_EDITCUT:
			if( bHasMark )
				pDataEdit->Cut();
			break;
		case RID_EDITCOPY:
			if( bHasMark )
				pDataEdit->Copy();
			break;
		case RID_EDITPASTE:
			{
				::rtl::OUString aTemp;
				if( ::svt::OStringTransfer::PasteString( aTemp, this ) ) 
					pDataEdit->Paste();
			}
			break;
		case RID_EDITDEL:
			/*if( bHasMark ) */pDataEdit->Delete();
			break;
		case RID_EDITUNDO:
			pDataEdit->Undo();
			break;
		case RID_EDITREDO:
			pDataEdit->Redo();
			break;
		case COMMAND_CONTEXTMENU:
			{
				PopupMenu *pKontext = NULL;
				pDataEdit->BuildKontextMenu( pKontext );
				if ( pKontext )
				{
					USHORT nRes = pKontext->Execute( this, GetPointerPosPixel() );
					if ( nRes )
						pFrame->Command( nRes );
					delete pKontext;
				}
			}
                        break;
	}
}


BOOL AppWin::IsSkipReload()
{
	return nSkipReload != 0;
}

void AppWin::SkipReload( BOOL bSkip )
{
	DBG_ASSERT( bSkip || nSkipReload, "SkipReload aufgehoben ohne es zu aktivieren");
	if ( bSkip )
		nSkipReload++;
	else
		nSkipReload--;
}

BOOL AppWin::DiskFileChanged( USHORT nWhat )
{
	if ( !bHasFile )
		return FALSE;

	switch ( nWhat )
	{
		case SINCE_LAST_LOAD:
			{
				if ( bReloadAborted )
					return TRUE;
				else
					return DiskFileChanged( SINCE_LAST_ASK_RELOAD );
			}
// uncomment to avoid compiler warning
//			break;
		case SINCE_LAST_ASK_RELOAD:
			{
				String aFilename( GetText() );

				DirEntry aFile( aFilename );
				FileStat aStat( aFile );

				return ( !aLastAccess.GetError() != !aStat.GetError() )
					|| aLastAccess.IsYounger( aStat ) || aStat.IsYounger( aLastAccess );
			}
// uncomment to avoid compiler warning
//			break;
		default:
			DBG_ERROR("Not Implemented in AppWin::DiskFileChanged");
	}
	return TRUE;
}

void AppWin::UpdateFileInfo( USHORT nWhat )
{
	switch ( nWhat )
	{
		case HAS_BEEN_LOADED:
			{
				bReloadAborted = FALSE;
				UpdateFileInfo( ASKED_RELOAD );

			}
			break;
		case ASKED_RELOAD:
			{
				String aFilename( GetText() );

				DirEntry aFile( aFilename );
				aLastAccess.Update( aFile );
			}
			break;
		default:
			DBG_ERROR("Not Implemented in AppWin::UpdateFileInfo");
	}
}

void AppWin::CheckReload()
{
	if ( IsSkipReload() || !bHasFile )
		return;

	String aFilename( GetText() );
	DirEntry aFile( aFilename );
	if ( !aFilename.Len() )
		return;

	if ( !aFile.Exists() )
		return;

//	FileStat aStat( aFile );

	if ( DiskFileChanged( SINCE_LAST_ASK_RELOAD ) && ReloadAllowed() )
	{
		UpdateFileInfo( ASKED_RELOAD );
		ToTop();
		Update();
		if ( (IsModified() && QueryBox( this, SttResId( IDS_ASKDIRTYRELOAD ) ).Execute() == RET_YES )
			|| ( !IsModified() && ( pFrame->IsAutoReload() || QueryBox( this, SttResId( IDS_ASKRELOAD ) ).Execute() == RET_YES ) ) )
		{
			Reload();
		}
		else
		{
			bReloadAborted = TRUE;
		}
	}
}

void AppWin::Reload()
{
	SkipReload();
	TextSelection aSelMemo = pDataEdit->GetSelection();
	Load( GetText() );
	pDataEdit->SetSelection( aSelMemo );
	SkipReload( FALSE );
}

// Load file
BOOL AppWin::Load( const String& aName )
{
	SkipReload();
	BOOL bErr;

//	if( !QuerySave() )
//		return;
	bErr = !pDataEdit->Load( aName );
	if( bErr )
    {
		ErrorBox aBox( this, SttResId( IDS_READERROR ) );
        String aMsg = aBox.GetMessText();
        aMsg.AppendAscii("\n\"");
        aMsg.Append( aName );
        aMsg.AppendAscii("\"");
        if ( pFrame->IsAutoRun() )
        {
            printf( "%s\n", ByteString( aMsg, osl_getThreadTextEncoding() ).GetBuffer() );
        }
        else
        {
            aBox.SetMessText( aMsg );
            aBox.Execute();
        }
    }
	else
	{
		DirEntry aEntry( aName );
		String aModName = aEntry.GetFull();
		SetText( aModName );
		UpdateFileInfo( HAS_BEEN_LOADED );
		PostLoad();
		bHasFile = TRUE;
	}
	SkipReload( FALSE );
    return !bErr;
}

// Save file
USHORT AppWin::ImplSave()
{
	SkipReload();
	USHORT nResult = SAVE_RES_NOT_SAVED;
	String s1 = *pNoName;
	String s2 = GetText().Copy( 0, s1.Len() );
	if( s1 == s2 )
		nResult = SaveAs();
	else {
		String aName = GetText();
		if ( pDataEdit->Save( aName ) )
		{
			nResult = SAVE_RES_SAVED;
			bHasFile = TRUE;
		}
		else
		{
			nResult = SAVE_RES_ERROR;
			ErrorBox( this, SttResId( IDS_WRITEERROR ) ).Execute();
		}
		UpdateFileInfo( HAS_BEEN_LOADED );
	}
	SkipReload( FALSE );
	return nResult;
}

// Save to new file name
USHORT AppWin::SaveAs()
{
	SkipReload();
	String s1 = *pNoName;
	String s2 = GetText().Copy( 0, s1.Len() );
	if( s1 == s2 ) s2.Erase();
	else s2 = GetText();
	if( pFrame->QueryFileName( s2, GetFileType(), TRUE ) )
	{
		SetText( s2 );
		PostSaveAs();
		SkipReload( FALSE );
		return ImplSave();
	}
	else
	{
		SkipReload( FALSE );
		return SAVE_RES_CANCEL;
	}
}

// Should we save the file?
USHORT AppWin::QuerySave( QueryBits nBits )
{
	BOOL bQueryDirty = ( nBits & QUERY_DIRTY ) != 0;
	BOOL bQueryDiskChanged = ( nBits & QUERY_DISK_CHANGED ) != 0;
	BOOL bSaveNotDirty = ( nBits & SAVE_NOT_DIRTY ) != 0;

	SkipReload();
	short nResult;
	if ( IsModified() || bSaveNotDirty )
		nResult = RET_YES;
	else
		nResult = RET_NO;

    BOOL bAlwaysEnableInput = pFrame->IsAlwaysEnableInput();
    pFrame->AlwaysEnableInput( FALSE );
	if( ( ( IsModified() || bSaveNotDirty ) && bQueryDirty ) || ( DiskFileChanged( SINCE_LAST_LOAD ) && bQueryDiskChanged ) )
	{
		ToTop();
		if ( ( ( IsModified() && bQueryDirty ) && DiskFileChanged( SINCE_LAST_LOAD ) )
			|| ( IsModified() && ( DiskFileChanged( SINCE_LAST_LOAD ) && bQueryDiskChanged ) ) )
			nResult = QueryBox( this, SttResId( IDS_ASK_DIRTY_AND_DISKCHANGE_SAVE ) ).Execute();
		else if ( ( IsModified() && bQueryDirty ) )
			nResult = QueryBox( this, SttResId( IDS_ASK_DIRTY_SAVE ) ).Execute();
		else
			nResult = QueryBox( this, SttResId( IDS_ASK_DISKCHANGE_SAVE ) ).Execute();
	}
    pFrame->AlwaysEnableInput( bAlwaysEnableInput );

	USHORT nReturn;
	switch( nResult )
	{
		case RET_YES:
			nReturn = ImplSave();
			break;
		case RET_NO:
			nReturn = SAVE_RES_NOT_SAVED;
			break;
		case RET_CANCEL:
			nReturn = SAVE_RES_CANCEL;
			break;
        default:
            DBG_ERROR("switch default where no default should be: Internal error");
            nReturn = SAVE_RES_CANCEL;
	}
	SkipReload( FALSE );
	return nReturn;
}

BOOL AppWin::Close()
{
	switch ( QuerySave( QUERY_DIRTY ) )
	{
	case SAVE_RES_NOT_SAVED:
	case SAVE_RES_SAVED:
		{
			DockingWindow::Close();
			delete this;
			return TRUE;
		}
// uncomment to avoid compiler warning
//	break;
	case SAVE_RES_ERROR:
		return FALSE;
// uncomment to avoid compiler warning
//		break;
	case SAVE_RES_CANCEL:
		return FALSE;
// uncomment to avoid compiler warning
//		break;
	default:
		DBG_ERROR("Not Implemented in AppWin::Close");
		return FALSE;
	}
}

// Search and find text
void AppWin::Find()
{
	SttResId aResId( IDD_FIND_DIALOG );
	FindDialog aDlg( this, aResId, aFind );
	if( aDlg.Execute() ) {
		bFind = TRUE;
		Repeat();
	}
}

// Replace text
void AppWin::Replace()
{
	SttResId aResId( IDD_REPLACE_DIALOG );
	ReplaceDialog* pDlg = new ReplaceDialog
						(this, aResId, aFind, aReplace );
	if( pDlg->Execute() ) {
		bFind = FALSE;
		Repeat();
	}
}

// Repeat search/replace operation
void AppWin::Repeat()
{
	if( (aFind.Len() != 0 ) && ( pDataEdit->Find( aFind ) || (ErrorBox(this,SttResId(IDS_PATTERNNOTFOUND)).Execute() && FALSE) ) && !bFind )
	  pDataEdit->ReplaceSelected( aReplace );
}

