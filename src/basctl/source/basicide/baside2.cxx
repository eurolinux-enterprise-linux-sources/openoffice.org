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
#include "precompiled_basctl.hxx"


#include <ide_pch.hxx>


#include <svtools/texteng.hxx>
#include <svtools/textview.hxx>
#include <svtools/xtextedt.hxx>
#include <basic/sbx.hxx>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/ui/dialogs/XFilePicker.hpp>
#include <com/sun/star/ui/dialogs/XFilePickerControlAccess.hpp>
#include <com/sun/star/ui/dialogs/XFilterManager.hpp>
#include <com/sun/star/ui/dialogs/TemplateDescription.hpp>
#include <com/sun/star/ui/dialogs/ExtendedFilePickerElementIds.hpp>
#ifndef _COM_SUN_STAR_SCRIPT_XLIBRYARYCONTAINER2_HPP_
#include <com/sun/star/script/XLibraryContainer2.hpp>
#endif
#include <com/sun/star/document/MacroExecMode.hpp>
#include <toolkit/helper/vclunohelper.hxx>
#include <sfx2/docfile.hxx>
#include <basic/basrdll.hxx>


#include <baside2.hrc>
#include <baside2.hxx>
#include <objdlg.hxx>
#include <iderdll.hxx>
#include <iderdll2.hxx>

#include <basobj.hxx>
#include <brkdlg.hxx>

#include <svx/srchdlg.hxx>

#include <vcl/sound.hxx>

//#ifndef _TXTCMP_HXX //autogen
//#include <svtools/txtcmp.hxx>
//#endif

#include <unotools/textsearch.hxx>
#include <tools/diagnose_ex.h>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;


#define SPLIT_MARGIN	5
#define SPLIT_HEIGHT	2

#define LMARGPRN		1700
#define RMARGPRN		 900
#define TMARGPRN    	2000
#define BMARGPRN    	1000
#define BORDERPRN		300

#define APPWAIT_START	100

#define VALIDWINDOW		0x1234

#if defined(OW) || defined(MTF)
#define FILTERMASK_ALL "*"
#elif defined(PM2)
#define FILTERMASK_ALL ""
#else
#define FILTERMASK_ALL "*.*"
#endif

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::ui::dialogs;
using namespace utl;
using namespace comphelper;


DBG_NAME( ModulWindow )

TYPEINIT1( ModulWindow , IDEBaseWindow );

void lcl_PrintHeader( Printer* pPrinter, USHORT nPages, USHORT nCurPage, const String& rTitle )
{
	short nLeftMargin 	= LMARGPRN;
	Size aSz = pPrinter->GetOutputSize();
	short nBorder = BORDERPRN;

	const Color aOldLineColor( pPrinter->GetLineColor() );
	const Color aOldFillColor( pPrinter->GetFillColor() );
	const Font	aOldFont( pPrinter->GetFont() );

	pPrinter->SetLineColor( Color( COL_BLACK ) );
	pPrinter->SetFillColor();

	Font aFont( aOldFont );
	aFont.SetWeight( WEIGHT_BOLD );
	aFont.SetAlign( ALIGN_BOTTOM );
	pPrinter->SetFont( aFont );

	long nFontHeight = pPrinter->GetTextHeight();

	// 1.Border => Strich, 2+3 Border = Freiraum.
	long nYTop = TMARGPRN-3*nBorder-nFontHeight;

	long nXLeft = nLeftMargin-nBorder;
	long nXRight = aSz.Width()-RMARGPRN+nBorder;

	pPrinter->DrawRect( Rectangle(
		Point( nXLeft, nYTop ),
		Size( nXRight-nXLeft, aSz.Height() - nYTop - BMARGPRN + nBorder ) ) );


	long nY = TMARGPRN-2*nBorder;
	Point aPos( nLeftMargin, nY );
	pPrinter->DrawText( aPos, rTitle );
	if ( nPages != 1 )
	{
		aFont.SetWeight( WEIGHT_NORMAL );
		pPrinter->SetFont( aFont );
		String aPageStr( RTL_CONSTASCII_USTRINGPARAM( " [" ) );
		aPageStr += String( IDEResId( RID_STR_PAGE ) );
		aPageStr += ' ';
		aPageStr += String::CreateFromInt32( nCurPage );
		aPageStr += ']';
		aPos.X() += pPrinter->GetTextWidth( rTitle );
		pPrinter->DrawText( aPos, aPageStr );
	}


	nY = TMARGPRN-nBorder;

	pPrinter->DrawLine( Point( nXLeft, nY ), Point( nXRight, nY ) );

	pPrinter->SetFont( aOldFont );
	pPrinter->SetFillColor( aOldFillColor );
	pPrinter->SetLineColor( aOldLineColor );
}

void lcl_ConvertTabsToSpaces( String& rLine )
{
	if ( rLine.Len() )
	{
		USHORT nPos = 0;
		USHORT nMax = rLine.Len();
		while ( nPos < nMax )
		{
			if ( rLine.GetChar( nPos ) == '\t' )
			{
				// Nicht 4 Blanks, sondern an 4er TabPos:
				String aBlanker;
				aBlanker.Fill( ( 4 - ( nPos % 4 ) ), ' ' );
				rLine.Erase( nPos, 1 );
				rLine.Insert( aBlanker, nPos );
				nMax = rLine.Len();
			}
			nPos++;	// Nicht optimal, falls Tab, aber auch nicht verkehrt...
		}
	}
}


ModulWindow::ModulWindow( ModulWindowLayout* pParent, const ScriptDocument& rDocument, String aLibName, 
                          String aName, ::rtl::OUString& aModule )
		:IDEBaseWindow( pParent, rDocument, aLibName, aName )
		,aXEditorWindow( this )
        ,m_aModule( aModule )
{
	DBG_CTOR( ModulWindow, 0 );
	nValid = VALIDWINDOW;
	pLayout = pParent;
	aXEditorWindow.Show();

    BasicManager* pBasMgr = rDocument.getBasicManager();
    if ( pBasMgr )
    {
        StarBASIC* pBasic = pBasMgr->GetLib( aLibName );
        if ( pBasic )
        {
            xBasic = pBasic;
            xModule = (SbModule*)pBasic->FindModule( aName );
        }
    }

	SetBackground();
}


__EXPORT ModulWindow::~ModulWindow()
{
	DBG_DTOR( ModulWindow, 0 );
	nValid = 0;

	StarBASIC::Stop();
}


void __EXPORT ModulWindow::GetFocus()
{
	if ( nValid != VALIDWINDOW  )
		return;
	DBG_CHKTHIS( ModulWindow, 0 );
	aXEditorWindow.GetEdtWindow().GrabFocus();
	// Basisklasse nicht rufen, weil Focus jetzt woanders...
}

void ModulWindow::DoInit()
{
	DBG_CHKTHIS( ModulWindow, 0 );
	// Wird beim Umschalten der Fenster gerufen...
	if ( GetVScrollBar() )
		GetVScrollBar()->Hide();
	GetHScrollBar()->Show();
//	GetEditorWindow().SetScrollBarRanges();
	GetEditorWindow().InitScrollBars();
//	GetEditorWindow().GrabFocus();
}


void __EXPORT ModulWindow::Paint( const Rectangle& )
{
}

void __EXPORT ModulWindow::Resize()
{
	aXEditorWindow.SetPosSizePixel( Point( 0, 0 ),
									Size( GetOutputSizePixel() ) );
}


// "Import" von baside4.cxx
void CreateEngineForBasic( StarBASIC* pBasic );

void ModulWindow::CheckCompileBasic()
{
	DBG_CHKTHIS( ModulWindow, 0 );

    if ( xModule.Is() )
    {
	    // Zur Laufzeit wird niemals compiliert!
	    BOOL bRunning = StarBASIC::IsRunning();
	    BOOL bModified = ( !xModule->IsCompiled() ||
		    ( GetEditEngine() && GetEditEngine()->IsModified() ) );

	    if ( !bRunning && bModified )
	    {
		    BOOL bDone = FALSE;

			BasicIDEShell* pIDEShell = IDE_DLL()->GetShell();
		    pIDEShell->GetViewFrame()->GetWindow().EnterWait();

		    if( bModified )
		    {
			    AssertValidEditEngine();
			    GetEditorWindow().SetSourceInBasic( FALSE );
		    }

		    BOOL bWasModified = GetBasic()->IsModified();

		    bDone = GetBasic()->Compile( xModule );
		    if ( !bWasModified )
			    GetBasic()->SetModified( FALSE );

		    if ( bDone )
		    {
			    GetBreakPoints().SetBreakPointsInBasic( xModule );
		    }

		    pIDEShell->GetViewFrame()->GetWindow().LeaveWait();

		    aStatus.bError = !bDone;
		    aStatus.bIsRunning = FALSE;
	    }
    }
}

BOOL ModulWindow::BasicExecute()
{
	DBG_CHKTHIS( ModulWindow, 0 );

    // #116444# check security settings before macro execution
    ScriptDocument aDocument( GetDocument() );
    if ( aDocument.isDocument() )
    {
        if ( !aDocument.allowMacros() )
        {
            WarningBox( this, WB_OK, String( IDEResId( RID_STR_CANNOTRUNMACRO ) ) ).Execute();
            return FALSE;
        }
    }

	CheckCompileBasic();

	if ( xModule.Is() && xModule->IsCompiled() && !aStatus.bError )
	{
		if ( GetBreakPoints().Count() )
			aStatus.nBasicFlags = aStatus.nBasicFlags | SbDEBUG_BREAK;

		if ( !aStatus.bIsRunning )
		{
			DBG_ASSERT( xModule.Is(), "Kein Modul!" );
			AddStatus( BASWIN_RUNNINGBASIC );
			USHORT nStart, nEnd, nCurMethodStart = 0;
			SbMethod* pMethod = 0;
			// erstes Macro, sonst blind "Main" (ExtSearch?)
			for ( USHORT nMacro = 0; nMacro < xModule->GetMethods()->Count(); nMacro++ )
			{
				SbMethod* pM = (SbMethod*)xModule->GetMethods()->Get( nMacro );
				DBG_ASSERT( pM, "Method?" );
				pM->GetLineRange( nStart, nEnd );
				if ( !pMethod || ( nStart < nCurMethodStart ) )
				{
					pMethod = pM;
					nCurMethodStart = nStart;
				}
			}
			if ( !pMethod )
				pMethod = (SbMethod*)xModule->Find( String( RTL_CONSTASCII_USTRINGPARAM( "Main" ) ), SbxCLASS_METHOD );

			if ( pMethod )
			{
				pMethod->SetDebugFlags( aStatus.nBasicFlags );
				BasicDLL::SetDebugMode( TRUE );
				BasicIDE::RunMethod( pMethod );
				BasicDLL::SetDebugMode( FALSE );
				// Falls waehrend Interactive=FALSE abgebrochen
				BasicDLL::EnableBreak( TRUE );
			}
			ClearStatus( BASWIN_RUNNINGBASIC );
		}
		else
			aStatus.bIsRunning = FALSE;	// Abbruch von Reschedule()
	}

	BOOL bDone = !aStatus.bError;

	return bDone;
}

BOOL ModulWindow::CompileBasic()
{
	DBG_CHKTHIS( ModulWindow, 0 );
	CheckCompileBasic();

    BOOL bIsCompiled = FALSE;
    if ( xModule.Is() )
        bIsCompiled = xModule->IsCompiled();

    return bIsCompiled;
}

BOOL ModulWindow::BasicRun()
{
	DBG_CHKTHIS( ModulWindow, 0 );

	aStatus.nBasicFlags = 0;
	BOOL bDone = BasicExecute();
	return bDone;
}

BOOL ModulWindow::BasicStepOver()
{
	DBG_CHKTHIS( ModulWindow, 0 );
	aStatus.nBasicFlags = SbDEBUG_STEPINTO | SbDEBUG_STEPOVER;
	BOOL bDone = BasicExecute();
	return bDone;
}


BOOL ModulWindow::BasicStepInto()
{
	DBG_CHKTHIS( ModulWindow, 0 );

	aStatus.nBasicFlags = SbDEBUG_STEPINTO;
	BOOL bDone = BasicExecute();
	return bDone;
}

BOOL ModulWindow::BasicStepOut()
{
	DBG_CHKTHIS( ModulWindow, 0 );

	aStatus.nBasicFlags = SbDEBUG_STEPOUT;
	BOOL bDone = BasicExecute();
	return bDone;
}



void ModulWindow::BasicStop()
{
	DBG_CHKTHIS( ModulWindow, 0 );

	GetBasic()->Stop();
	aStatus.bIsRunning = FALSE;
}

BOOL ModulWindow::LoadBasic()
{
	DBG_CHKTHIS( ModulWindow, 0 );
	BOOL bDone = FALSE;

    Reference< lang::XMultiServiceFactory > xMSF( ::comphelper::getProcessServiceFactory() );
    Reference < XFilePicker > xFP;
    if( xMSF.is() )
    {
		Sequence <Any> aServiceType(1);
		aServiceType[0] <<= TemplateDescription::FILEOPEN_SIMPLE;
        xFP = Reference< XFilePicker >( xMSF->createInstanceWithArguments(
					::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.ui.dialogs.FilePicker" ) ), aServiceType ), UNO_QUERY );
    }

	if ( aCurPath.Len() )
		xFP->setDisplayDirectory ( aCurPath );

	//xFP->setTitle( String( IDEResId( RID_STR_OPEN ) ) );

    Reference< XFilterManager > xFltMgr(xFP, UNO_QUERY);
	xFltMgr->appendFilter( String( RTL_CONSTASCII_USTRINGPARAM( "BASIC" ) ), String( RTL_CONSTASCII_USTRINGPARAM( "*.bas" ) ) );
	xFltMgr->appendFilter( String( IDEResId( RID_STR_FILTER_ALLFILES ) ), String( RTL_CONSTASCII_USTRINGPARAM( FILTERMASK_ALL ) ) );
	xFltMgr->setCurrentFilter( String( RTL_CONSTASCII_USTRINGPARAM( "BASIC" ) ) );

    if( xFP->execute() == RET_OK )
	{
		Sequence< ::rtl::OUString > aPaths = xFP->getFiles();
		aCurPath = aPaths[0];
		SfxMedium aMedium( aCurPath, STREAM_READ | STREAM_SHARE_DENYWRITE | STREAM_NOCREATE, TRUE );
		SvStream* pStream = aMedium.GetInStream();
		if ( pStream )
		{
			AssertValidEditEngine();
			ULONG nLines = CalcLineCount( *pStream );
			// nLines*4: ReadText/Formatting/Highlighting/Formatting
			GetEditorWindow().CreateProgress( String( IDEResId( RID_STR_GENERATESOURCE ) ), nLines*4 );
			GetEditEngine()->SetUpdateMode( FALSE );
			GetEditView()->Read( *pStream );
			GetEditEngine()->SetUpdateMode( TRUE );
			GetEditorWindow().Update();	// Es wurde bei UpdateMode = TRUE nur Invalidiert
			GetEditorWindow().ForceSyntaxTimeout();
			GetEditorWindow().DestroyProgress();
			ULONG nError = aMedium.GetError();
			if ( nError )
				ErrorHandler::HandleError( nError );
			else
				bDone = TRUE;
		}
		else
			ErrorBox( this, WB_OK | WB_DEF_OK, String( IDEResId( RID_STR_COULDNTREAD ) ) ).Execute();
	}
	return bDone;
}


BOOL ModulWindow::SaveBasicSource()
{
	DBG_CHKTHIS( ModulWindow, 0 );
	BOOL bDone = FALSE;

    Reference< lang::XMultiServiceFactory > xMSF( ::comphelper::getProcessServiceFactory() );
    Reference < XFilePicker > xFP;
    if( xMSF.is() )
    {
		Sequence <Any> aServiceType(1);
		aServiceType[0] <<= TemplateDescription::FILESAVE_AUTOEXTENSION_PASSWORD;
        xFP = Reference< XFilePicker >( xMSF->createInstanceWithArguments(
					::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.ui.dialogs.FilePicker" ) ), aServiceType ), UNO_QUERY );
    }

	Reference< XFilePickerControlAccess > xFPControl(xFP, UNO_QUERY);
	xFPControl->enableControl(ExtendedFilePickerElementIds::CHECKBOX_PASSWORD, sal_False);
    Any aValue;
    aValue <<= (sal_Bool) sal_True;
	xFPControl->setValue(ExtendedFilePickerElementIds::CHECKBOX_AUTOEXTENSION, 0, aValue);

	if ( aCurPath.Len() )
		xFP->setDisplayDirectory ( aCurPath );

	//xFP->setTitle( String( IDEResId( RID_STR_SAVE ) ) );

    Reference< XFilterManager > xFltMgr(xFP, UNO_QUERY);
	xFltMgr->appendFilter( String( RTL_CONSTASCII_USTRINGPARAM( "BASIC" ) ), String( RTL_CONSTASCII_USTRINGPARAM( "*.bas" ) ) );
	xFltMgr->appendFilter( String( IDEResId( RID_STR_FILTER_ALLFILES ) ), String( RTL_CONSTASCII_USTRINGPARAM( FILTERMASK_ALL ) ) );
	xFltMgr->setCurrentFilter( String( RTL_CONSTASCII_USTRINGPARAM( "BASIC" ) ) );

    if( xFP->execute() == RET_OK )
	{
		Sequence< ::rtl::OUString > aPaths = xFP->getFiles();
		aCurPath = aPaths[0];
		SfxMedium aMedium( aCurPath, STREAM_WRITE | STREAM_SHARE_DENYWRITE | STREAM_TRUNC, TRUE, FALSE );
		SvStream* pStream = aMedium.GetOutStream();
		if ( pStream )
		{
			EnterWait();
			AssertValidEditEngine();
			GetEditEngine()->Write( *pStream );
			aMedium.Commit();
			LeaveWait();
			ULONG nError = aMedium.GetError();
			if ( nError )
				ErrorHandler::HandleError( nError );
			else
				bDone = TRUE;
		}
		else
			ErrorBox( this, WB_OK | WB_DEF_OK, String( IDEResId( RID_STR_COULDNTWRITE) ) ).Execute();
	}

	return bDone;
}

BOOL implImportDialog( Window* pWin, const String& rCurPath, const ScriptDocument& rDocument, const String& aLibName ); 

BOOL ModulWindow::ImportDialog()
{
	const ScriptDocument& rDocument = GetDocument();
	String aLibName = GetLibName();
	BOOL bRet = implImportDialog( this, aCurPath, rDocument, aLibName );
	return bRet;
}

BOOL ModulWindow::ToggleBreakPoint( ULONG nLine )
{
	DBG_ASSERT( xModule.Is(), "Kein Modul!" );

	BOOL bNewBreakPoint = FALSE;

    if ( xModule.Is() )
    {
        CheckCompileBasic();
	    if ( aStatus.bError )
	    {
		    Sound::Beep();
		    return FALSE;
	    }

        BreakPoint* pBrk = GetBreakPoints().FindBreakPoint( nLine );
	    if ( pBrk ) // entfernen
	    {
		    xModule->ClearBP( (USHORT)nLine );
		    delete GetBreakPoints().Remove( pBrk );
	    }
	    else // einen erzeugen
	    {
		    if ( xModule->SetBP( (USHORT)nLine) )
		    {
			    GetBreakPoints().InsertSorted( new BreakPoint( nLine ) );
			    bNewBreakPoint = TRUE;
			    if ( StarBASIC::IsRunning() )
			    {
				    for ( USHORT nMethod = 0; nMethod < xModule->GetMethods()->Count(); nMethod++ )
				    {
					    SbMethod* pMethod = (SbMethod*)xModule->GetMethods()->Get( nMethod );
					    DBG_ASSERT( pMethod, "Methode nicht gefunden! (NULL)" );
					    pMethod->SetDebugFlags( pMethod->GetDebugFlags() | SbDEBUG_BREAK );
				    }
			    }
		    }

		    if ( !bNewBreakPoint )
			    Sound::Beep();
	    }
    }

	return bNewBreakPoint;
}

void ModulWindow::UpdateBreakPoint( const BreakPoint& rBrk )
{
	DBG_ASSERT( xModule.Is(), "Kein Modul!" );

    if ( xModule.Is() )
    {
	    CheckCompileBasic();

        if ( rBrk.bEnabled )
		    xModule->SetBP( (USHORT)rBrk.nLine );
	    else
		    xModule->ClearBP( (USHORT)rBrk.nLine );
    }
}


BOOL ModulWindow::BasicToggleBreakPoint()
{
	DBG_CHKTHIS( ModulWindow, 0 );
	AssertValidEditEngine();

	TextSelection aSel = GetEditView()->GetSelection();
	aSel.GetStart().GetPara()++;	// Basic-Zeilen beginnen bei 1!
	aSel.GetEnd().GetPara()++;

	BOOL bNewBreakPoint = FALSE;

	for ( ULONG nLine = aSel.GetStart().GetPara(); nLine <= aSel.GetEnd().GetPara(); nLine++ )
	{
		if ( ToggleBreakPoint( nLine ) )
			bNewBreakPoint = TRUE;
	}

	aXEditorWindow.GetBrkWindow().Invalidate();
	return bNewBreakPoint;
}


void ModulWindow::BasicToggleBreakPointEnabled()
{
	DBG_CHKTHIS( ModulWindow, 0 );
	AssertValidEditEngine();

	ExtTextView* pView = GetEditView();
	if ( pView )
	{
		TextSelection aSel = pView->GetSelection();
		BreakPointList& rList = GetBreakPoints();

		for ( ULONG nLine = ++aSel.GetStart().GetPara(), nEnd = ++aSel.GetEnd().GetPara(); nLine <= nEnd; ++nLine )
		{
			BreakPoint* pBrk = rList.FindBreakPoint( nLine );
			if ( pBrk )
			{
				pBrk->bEnabled = pBrk->bEnabled ? FALSE : TRUE;
				UpdateBreakPoint( *pBrk );
			}
		}

		GetBreakPointWindow().Invalidate();
	}
}


void ModulWindow::ManageBreakPoints()
{
	BreakPointWindow& rBrkWin = GetBreakPointWindow();
	BreakPointDialog aBrkDlg( &rBrkWin, GetBreakPoints() );
	aBrkDlg.Execute();
	rBrkWin.Invalidate();
}


IMPL_LINK( ModulWindow, BasicErrorHdl, StarBASIC *, pBasic )
{
	DBG_CHKTHIS( ModulWindow, 0 );
	GoOnTop();

	// ReturnWert: BOOL
	//	FALSE:	Abbrechen
	//	TRUE:	Weiter....
	String aErrorText( pBasic->GetErrorText() );
	USHORT nErrorLine = pBasic->GetLine() - 1;
	USHORT nErrCol1 = pBasic->GetCol1();
	USHORT nErrCol2 = pBasic->GetCol2();
	if ( nErrCol2 != 0xFFFF )
		nErrCol2++;

	AssertValidEditEngine();
	GetEditView()->SetSelection( TextSelection( TextPaM( nErrorLine, nErrCol1 ), TextPaM( nErrorLine, nErrCol2 ) ) );

	String aErrorTextPrefix;
	if( pBasic->IsCompilerError() )
	{
		aErrorTextPrefix = String( IDEResId( RID_STR_COMPILEERROR ) );
	}
	else
	{
		aErrorTextPrefix = String( IDEResId( RID_STR_RUNTIMEERROR ) );
		aErrorTextPrefix += StarBASIC::GetVBErrorCode( pBasic->GetErrorCode() );
		aErrorTextPrefix += ' ';
		pLayout->GetStackWindow().UpdateCalls();
	}
	// Wenn anderes Basic, dan sollte die IDE versuchen, da richtige
	// Modul anzuzeigen...
	BOOL bMarkError = ( pBasic == GetBasic() ) ? TRUE : FALSE;
	if ( bMarkError )
		aXEditorWindow.GetBrkWindow().SetMarkerPos( nErrorLine, TRUE );
//	ErrorBox( this, WB_OK | WB_DEF_OK, String( aErrorTextPrefix + aErrorText ) ).Execute();
//	ErrorHandler::HandleError( pBasic->GetErrorCode() );

    // #i47002#
    Reference< awt::XWindow > xWindow = VCLUnoHelper::GetInterface( this );

    ErrorHandler::HandleError( StarBASIC::GetErrorCode() );

    // #i47002#
    Window* pWindow = VCLUnoHelper::GetWindow( xWindow );
    if ( !pWindow )
        return FALSE;

    if ( bMarkError )
		aXEditorWindow.GetBrkWindow().SetMarkerPos( MARKER_NOMARKER );
	return FALSE;
}

long __EXPORT ModulWindow::BasicBreakHdl( StarBASIC* pBasic )
{
	DBG_CHKTHIS( ModulWindow, 0 );
	// Ein GoOnTop aktiviert da Fenster, das veraendert aber den Context fuer
	// das Programm!
//	GoOnTop();

	// #i69280 Required in Window despite normal usage in next command!
	(void)pBasic;

	// ReturnWert: USHORT => siehe SB-Debug-Flags
	USHORT nErrorLine = pBasic->GetLine();

	// Gibt es hier einen BreakPoint?
	BreakPoint* pBrk = GetBreakPoints().FindBreakPoint( nErrorLine );    
    if ( pBrk )
    {
        pBrk->nHitCount++;
        if ( pBrk->nHitCount < pBrk->nStopAfter && GetBasic()->IsBreak() )
            return aStatus.nBasicFlags; // weiterlaufen...
    }

	nErrorLine--;	// EditEngine begint bei 0, Basic bei 1
	// Alleine schon damit gescrollt wird...
	AssertValidEditEngine();
	GetEditView()->SetSelection( TextSelection( TextPaM( nErrorLine, 0 ), TextPaM( nErrorLine, 0 ) ) );
	aXEditorWindow.GetBrkWindow().SetMarkerPos( nErrorLine );

	pLayout->GetWatchWindow().UpdateWatches();
	pLayout->GetStackWindow().UpdateCalls();

	aStatus.bIsInReschedule = TRUE;
	aStatus.bIsRunning = TRUE;

	AddStatus( BASWIN_INRESCHEDULE );

	BasicIDE::InvalidateDebuggerSlots();

	while( aStatus.bIsRunning )
		Application::Yield();

	aStatus.bIsInReschedule = FALSE;
	aXEditorWindow.GetBrkWindow().SetMarkerPos( MARKER_NOMARKER );

	ClearStatus( BASWIN_INRESCHEDULE );

	return aStatus.nBasicFlags;
}

void ModulWindow::BasicAddWatch()
{
	DBG_CHKTHIS( ModulWindow, 0 );
	String aWatchStr;
	BOOL bInserted = FALSE;
	AssertValidEditEngine();
	BOOL bAdd = TRUE;
	if ( !GetEditView()->HasSelection() )
	{
//		bAdd = GetEditView()->SelectCurrentWord();
		TextPaM aWordStart;
		String aWord = GetEditEngine()->GetWord( GetEditView()->GetSelection().GetEnd(), &aWordStart );
		if ( aWord.Len() )
		{
			TextSelection aSel( aWordStart );
			USHORT& rIndex = aSel.GetEnd().GetIndex();
			rIndex = rIndex + aWord.Len();
			// aSel.GetEnd().GetIndex() += sal::static_int_cast<int>( aWord.Len() );
			GetEditView()->SetSelection( aSel );
			bAdd = TRUE;
		}
	}
	if ( bAdd )
	{
		TextSelection aSel = GetEditView()->GetSelection();
		if ( aSel.GetStart().GetPara() == aSel.GetEnd().GetPara() )
		{
			aWatchStr = GetEditView()->GetSelected();
			pLayout->GetWatchWindow().AddWatch( aWatchStr );
			pLayout->GetWatchWindow().UpdateWatches();
			bInserted = TRUE;
		}
	}

	if ( !bInserted )
		Sound::Beep();
}



void ModulWindow::BasicRemoveWatch()
{
	DBG_CHKTHIS( ModulWindow, 0 );
	BOOL bRemoved = pLayout->GetWatchWindow().RemoveSelectedWatch();

	if ( !bRemoved )
		Sound::Beep();
}


void ModulWindow::EditMacro( const String& rMacroName )
{
	DBG_CHKTHIS( ModulWindow, 0 );
	DBG_ASSERT( xModule.Is(), "Kein Modul!" );

    if ( xModule.Is() )
    {
        CheckCompileBasic();

	    if ( !aStatus.bError )
	    {
		    USHORT nStart, nEnd;
		    SbMethod* pMethod = (SbMethod*)xModule->Find( rMacroName, SbxCLASS_METHOD );
		    if ( pMethod )
		    {
			    pMethod->GetLineRange( nStart, nEnd );
			    if ( nStart )
			    {
				    // Basic beginnt bei 1
				    nStart--;
				    nEnd--;
			    }
			    TextSelection aSel( TextPaM( nStart, 0 ), TextPaM( nStart, 0 ) );
			    AssertValidEditEngine();
			    TextView * pView = GetEditView();
			    // ggf. hinscrollen, so dass erste Zeile oben...
			    long nVisHeight = GetOutputSizePixel().Height();
			    if ( (long)pView->GetTextEngine()->GetTextHeight() > nVisHeight )
			    {
				    long nMaxY = pView->GetTextEngine()->GetTextHeight() - nVisHeight;
				    long nOldStartY = pView->GetStartDocPos().Y();
				    long nNewStartY = nStart * pView->GetTextEngine()->GetCharHeight();
				    nNewStartY = Min( nNewStartY, nMaxY );
				    pView->Scroll( 0, -(nNewStartY-nOldStartY) );
				    pView->ShowCursor( FALSE, TRUE );
				    GetEditVScrollBar().SetThumbPos( pView->GetStartDocPos().Y() );
			    }
			    pView->SetSelection( aSel );
			    pView->ShowCursor();
			    pView->GetWindow()->GrabFocus();
		    }
	    }
    }
}


void __EXPORT ModulWindow::StoreData()
{
	DBG_CHKTHIS( ModulWindow, 0 );
	// StoreData wird gerufen, wenn der BasicManager zerstoert oder
	// dieses Fenster beendet wird.
	// => Keine Unterbrechungen erwuenscht!
	// Und bei SAVE, wenn AppBasic...
	GetEditorWindow().SetSourceInBasic( TRUE );
	// Nicht das Modify loeschen, sonst wird das Basic nicht gespeichert
	// Es wird beim Speichern sowieso geloescht.
//	xModule->SetModified( FALSE );
}

BOOL __EXPORT ModulWindow::CanClose()
{
	DBG_CHKTHIS( ModulWindow, 0 );
	return TRUE;
}


BOOL __EXPORT ModulWindow::AllowUndo()
{
	return GetEditorWindow().CanModify();
}


void __EXPORT ModulWindow::UpdateData()
{
	DBG_CHKTHIS( ModulWindow, 0 );
	DBG_ASSERT( xModule.Is(), "Kein Modul!" );
	// UpdateData wird gerufen, wenn sich der Source von aussen
	// geaendert hat.
	// => Keine Unterbrechungen erwuenscht!

    if ( xModule.Is() )
    {
        SetModule( xModule->GetSource32() );

        if ( GetEditView() )
	    {
		    TextSelection aSel = GetEditView()->GetSelection();
            setTextEngineText( GetEditEngine(), xModule->GetSource32() );
		    GetEditView()->SetSelection( aSel );
		    GetEditEngine()->SetModified( FALSE );
		    BasicIDE::MarkDocumentModified( GetDocument() );
	    }
    }
}


void __EXPORT ModulWindow::PrintData( Printer* pPrinter )
{
	DBG_CHKTHIS( ModulWindow, 0 );

	AssertValidEditEngine();

	MapMode eOldMapMode( pPrinter->GetMapMode() );
	Font aOldFont( pPrinter->GetFont() );

//	Font aFont( GetEditEngine()->CreateFontFromItemSet( GetEditEngine()->GetEmptyItemSet() ) );
	Font aFont( GetEditEngine()->GetFont() );
	aFont.SetAlign( ALIGN_BOTTOM );
	aFont.SetTransparent( TRUE );
	aFont.SetSize( Size( 0, 360 ) );
	pPrinter->SetFont( aFont );
	pPrinter->SetMapMode( MAP_100TH_MM );

	String aTitle( CreateQualifiedName() );

	USHORT nLineHeight = (USHORT) pPrinter->GetTextHeight(); // etwas mehr.
	USHORT nParaSpace = 10;

	Size aPaperSz = pPrinter->GetOutputSize();
	aPaperSz.Width() -= (LMARGPRN+RMARGPRN);
	aPaperSz.Height() -= (TMARGPRN+BMARGPRN);

	// nLinepPage stimmt nicht, wenn Zeilen umgebrochen werden muessen...
	USHORT nLinespPage = (USHORT) (aPaperSz.Height()/nLineHeight);
	USHORT nCharspLine = (USHORT) (aPaperSz.Width() / pPrinter->GetTextWidth( 'X' ) );
	ULONG nParas = GetEditEngine()->GetParagraphCount();

	USHORT nPages = (USHORT) (nParas/nLinespPage+1 );
	USHORT nCurPage = 1;

	pPrinter->StartJob( aTitle );
	pPrinter->StartPage();
	// Header drucken...
	lcl_PrintHeader( pPrinter, nPages, nCurPage, aTitle );
	Point aPos( LMARGPRN, TMARGPRN );
	for ( ULONG nPara = 0; nPara < nParas; nPara++ )
	{
		String aLine( GetEditEngine()->GetText( nPara ) );
		lcl_ConvertTabsToSpaces( aLine );
		USHORT nLines = aLine.Len()/nCharspLine+1;
		for ( USHORT nLine = 0; nLine < nLines; nLine++ )
		{
			String aTmpLine( aLine, nLine*nCharspLine, nCharspLine );
			aPos.Y() += nLineHeight;
			if ( aPos.Y() > ( aPaperSz.Height()+TMARGPRN ) )
			{
				nCurPage++;
				pPrinter->EndPage();
				pPrinter->StartPage();
				lcl_PrintHeader( pPrinter, nPages, nCurPage, aTitle );
				aPos = Point( LMARGPRN, TMARGPRN+nLineHeight );
			}
			pPrinter->DrawText( aPos, aTmpLine );
		}
		aPos.Y() += nParaSpace;
	}
	pPrinter->EndPage();
	pPrinter->EndJob();

	pPrinter->SetFont( aOldFont );
	pPrinter->SetMapMode( eOldMapMode );
}


void __EXPORT ModulWindow::ExecuteCommand( SfxRequest& rReq )
{
	DBG_CHKTHIS( ModulWindow, 0 );
	AssertValidEditEngine();
	USHORT nSlot = rReq.GetSlot();
	switch ( nSlot )
	{
		case SID_BASICRUN:
		{
			BasicRun();
		}
		break;
		case SID_BASICCOMPILE:
		{
			CompileBasic();
		}
		break;
		case SID_BASICSTEPOVER:
		{
			BasicStepOver();
		}
		break;
		case SID_BASICSTEPINTO:
		{
			BasicStepInto();
		}
		break;
		case SID_BASICSTEPOUT:
		{
			BasicStepOut();
		}
		break;
		case SID_BASICLOAD:
		{
			LoadBasic();
		}
		break;
		case SID_BASICSAVEAS:
		{
			SaveBasicSource();
		}
		break;
		case SID_IMPORT_DIALOG:
		{
			ImportDialog();
		}
		break;
		case SID_BASICIDE_MATCHGROUP:
		{
			if ( !GetEditView()->MatchGroup() )
				Sound::Beep();
		}
		break;
		case SID_BASICIDE_TOGGLEBRKPNT:
		{
			BasicToggleBreakPoint();
		}
		break;
		case SID_BASICIDE_MANAGEBRKPNTS:
		{
			ManageBreakPoints();
		}
		break;
		case SID_BASICIDE_TOGGLEBRKPNTENABLED:
		{
			BasicToggleBreakPointEnabled();
		}
		break;
		case SID_BASICIDE_ADDWATCH:
		{
			BasicAddWatch();
		}
		break;
		case SID_BASICIDE_REMOVEWATCH:
		{
			BasicRemoveWatch();
		}
		break;
		case SID_CUT:
		{
            if ( !IsReadOnly() )
            {
			    GetEditView()->Cut();
                SfxBindings* pBindings = BasicIDE::GetBindingsPtr();
                if ( pBindings )
                    pBindings->Invalidate( SID_DOC_MODIFIED );
            }
		}
		break;
		case SID_COPY:
		{
			GetEditView()->Copy();
		}
		break;
		case SID_PASTE:
		{
            if ( !IsReadOnly() )
            {
			    GetEditView()->Paste();
                SfxBindings* pBindings = BasicIDE::GetBindingsPtr();
                if ( pBindings )
                    pBindings->Invalidate( SID_DOC_MODIFIED );
            }
		}
		break;
		case SID_BASICIDE_BRKPNTSCHANGED:
		{
			GetBreakPointWindow().Invalidate();
		}
		break;
	}
}



void __EXPORT ModulWindow::GetState( SfxItemSet &rSet )
{
	DBG_CHKTHIS( ModulWindow, 0 );
	SfxWhichIter aIter(rSet);
	for ( USHORT nWh = aIter.FirstWhich(); 0 != nWh; nWh = aIter.NextWhich() )
	{
		switch ( nWh )
		{
			// allgemeine Items:
			case SID_CUT:
			{
				if ( !GetEditView() || !GetEditView()->HasSelection() )
					rSet.DisableItem( nWh );

                if ( IsReadOnly() )
                    rSet.DisableItem( nWh );
            }
			break;
            case SID_COPY:
			{
				if ( !GetEditView() || !GetEditView()->HasSelection() )
					rSet.DisableItem( nWh );
			}
			break;
            case SID_PASTE:
            {
                if ( !IsPasteAllowed() )
                    rSet.DisableItem( nWh );

                if ( IsReadOnly() )
                    rSet.DisableItem( nWh );
            }
			break;
			case SID_BASICIDE_STAT_POS:
			{
				TextView* pView = GetEditView();
				if ( pView )
				{
					TextSelection aSel = pView->GetSelection();
					String aPos( IDEResId( RID_STR_LINE ) );
					aPos += ' ';
					aPos += String::CreateFromInt32( aSel.GetEnd().GetPara()+1 );
					aPos += String( RTL_CONSTASCII_USTRINGPARAM( ", " ) );
					aPos += String( IDEResId( RID_STR_COLUMN ) );
					aPos += ' ';
					aPos += String::CreateFromInt32( aSel.GetEnd().GetIndex()+1 );
					SfxStringItem aItem( SID_BASICIDE_STAT_POS, aPos );
					rSet.Put( aItem );
				}
			}
			break;
			case SID_ATTR_INSERT:
			{
				TextView* pView = GetEditView();
				if ( pView )
				{
					SfxBoolItem aItem( SID_ATTR_INSERT, pView->IsInsertMode() );
					rSet.Put( aItem );
				}
			}
			break;
		}
	}
}


void __EXPORT ModulWindow::DoScroll( ScrollBar* pCurScrollBar )
{
	DBG_CHKTHIS( ModulWindow, 0 );
	if ( ( pCurScrollBar == GetHScrollBar() ) && GetEditView() )
	{
		// Nicht mit dem Wert Scrollen, sondern lieber die Thumb-Pos fuer die
		// VisArea verwenden:
		long nDiff = GetEditView()->GetStartDocPos().X() - pCurScrollBar->GetThumbPos();
		GetEditView()->Scroll( nDiff, 0 );
		GetEditView()->ShowCursor( FALSE, TRUE );
		pCurScrollBar->SetThumbPos( GetEditView()->GetStartDocPos().X() );

	}
}


BOOL ModulWindow::RenameModule( const String& rNewName )
{
	if ( !BasicIDE::RenameModule( this, GetDocument(), GetLibName(), GetName(), rNewName ) )
        return FALSE;

    SfxBindings* pBindings = BasicIDE::GetBindingsPtr();
    if ( pBindings )
        pBindings->Invalidate( SID_DOC_MODIFIED );

	return TRUE;
}


BOOL __EXPORT ModulWindow::IsModified()
{
	return GetEditEngine() ? GetEditEngine()->IsModified() : FALSE;
}



void __EXPORT ModulWindow::GoOnTop()
{
	IDE_DLL()->GetShell()->GetViewFrame()->ToTop();
}

String ModulWindow::GetSbModuleName()
{
	String aModuleName;
	if ( xModule.Is() )
		aModuleName = xModule->GetName();
	return aModuleName;
}



String __EXPORT ModulWindow::GetTitle()
{
	return GetSbModuleName();
}



void ModulWindow::FrameWindowMoved()
{
//	if ( GetEditEngine() && GetEditEngine()->IsInSelectionMode() )
//		GetEditEngine()->StopSelectionMode();
}



void ModulWindow::ShowCursor( BOOL bOn )
{
	if ( GetEditEngine() )
	{
		TextView* pView = GetEditEngine()->GetActiveView();
		if ( pView )
		{
			if ( bOn )
				pView->ShowCursor();
			else
				pView->HideCursor();
		}
	}
}


Window* __EXPORT ModulWindow::GetLayoutWindow()
{
	return pLayout;
}

void ModulWindow::AssertValidEditEngine()
{
	if ( !GetEditEngine() )
		GetEditorWindow().CreateEditEngine();
}

void ModulWindow::Deactivating()
{
	if ( GetEditView() )
		GetEditView()->EraseVirtualDevice();
}

USHORT ModulWindow::StartSearchAndReplace( const SvxSearchItem& rSearchItem, BOOL bFromStart )
{
	// Mann koennte fuer das blinde Alle-Ersetzen auch auf
	// Syntaxhighlighting/Formatierung verzichten...
	AssertValidEditEngine();
	ExtTextView* pView = GetEditView();
	TextSelection aSel;
	if ( bFromStart )
	{
		aSel = pView->GetSelection();
		if ( !rSearchItem.GetBackward() )
			pView->SetSelection( TextSelection() );
		else
			pView->SetSelection( TextSelection( TextPaM( 0xFFFFFFFF, 0xFFFF ), TextPaM( 0xFFFFFFFF, 0xFFFF ) ) );
	}

	BOOL bForward = !rSearchItem.GetBackward();
	USHORT nFound = 0;
	if ( ( rSearchItem.GetCommand() == SVX_SEARCHCMD_FIND ) ||
		 ( rSearchItem.GetCommand() == SVX_SEARCHCMD_FIND_ALL ) )
	{
		nFound = pView->Search( rSearchItem.GetSearchOptions() , bForward );
	}
	else if ( ( rSearchItem.GetCommand() == SVX_SEARCHCMD_REPLACE ) ||
			  ( rSearchItem.GetCommand() == SVX_SEARCHCMD_REPLACE_ALL ) )
	{
        if ( !IsReadOnly() )
        {
            BOOL bAll = rSearchItem.GetCommand() == SVX_SEARCHCMD_REPLACE_ALL;
            nFound = pView->Replace( rSearchItem.GetSearchOptions() , bAll , bForward );
        }
	}

	if ( bFromStart && !nFound )
		pView->SetSelection( aSel );

	return nFound;
}

SfxUndoManager* __EXPORT ModulWindow::GetUndoManager()
{
	if ( GetEditEngine() )
		return &GetEditEngine()->GetUndoManager();
	return NULL;
}

USHORT __EXPORT ModulWindow::GetSearchOptions()
{
    USHORT nOptions = SEARCH_OPTIONS_SEARCH |
                      SEARCH_OPTIONS_WHOLE_WORDS |
                      SEARCH_OPTIONS_BACKWARDS |
                      SEARCH_OPTIONS_REG_EXP |
                      SEARCH_OPTIONS_EXACT |
                      SEARCH_OPTIONS_SELECTION |
                      SEARCH_OPTIONS_SIMILARITY;

    if ( !IsReadOnly() )
    {
        nOptions |= SEARCH_OPTIONS_REPLACE;
        nOptions |= SEARCH_OPTIONS_REPLACE_ALL;
    }

    return nOptions;
}

void __EXPORT ModulWindow::BasicStarted()
{
    if ( xModule.Is() )
    {
        aStatus.bIsRunning = TRUE;
        BreakPointList& rList = GetBreakPoints();
        if ( rList.Count() )
	    {
            rList.ResetHitCount();
		    rList.SetBreakPointsInBasic( xModule );
		    for ( USHORT nMethod = 0; nMethod < xModule->GetMethods()->Count(); nMethod++ )
		    {
			    SbMethod* pMethod = (SbMethod*)xModule->GetMethods()->Get( nMethod );
			    DBG_ASSERT( pMethod, "Methode nicht gefunden! (NULL)" );
			    pMethod->SetDebugFlags( pMethod->GetDebugFlags() | SbDEBUG_BREAK );
		    }
	    }
    }
}

void __EXPORT ModulWindow::BasicStopped()
{
	aStatus.bIsRunning = FALSE;
	GetBreakPointWindow().SetMarkerPos( MARKER_NOMARKER );
}

BasicEntryDescriptor ModulWindow::CreateEntryDescriptor()
{
    ScriptDocument aDocument( GetDocument() );
    String aLibName( GetLibName() );
    LibraryLocation eLocation = aDocument.getLibraryLocation( aLibName );
    return BasicEntryDescriptor( aDocument, eLocation, aLibName, GetName(), OBJ_TYPE_MODULE );
}

void ModulWindow::SetReadOnly( BOOL b )
{
	if ( GetEditView() )
		GetEditView()->SetReadOnly( b );
}

BOOL ModulWindow::IsReadOnly()
{
    BOOL bReadOnly = FALSE;

    if ( GetEditView() )
        bReadOnly = GetEditView()->IsReadOnly();

    return bReadOnly;
}

BOOL ModulWindow::IsPasteAllowed()
{
    BOOL bPaste = FALSE;

    // get clipboard
	Reference< datatransfer::clipboard::XClipboard > xClipboard = GetClipboard();
	if ( xClipboard.is() )
	{
		// get clipboard content
		const sal_uInt32 nRef = Application::ReleaseSolarMutex();
		Reference< datatransfer::XTransferable > xTransf = xClipboard->getContents();
		Application::AcquireSolarMutex( nRef );
		if ( xTransf.is() )
		{
			datatransfer::DataFlavor aFlavor;
			SotExchange::GetFormatDataFlavor( SOT_FORMAT_STRING, aFlavor );
            if ( xTransf->isDataFlavorSupported( aFlavor ) )
			{
                bPaste = TRUE;
            }
        }
    }

    return bPaste;
}

ModulWindowLayout::ModulWindowLayout( Window* pParent ) :
	Window( pParent, WB_CLIPCHILDREN ),
	aVSplitter( this, WinBits( WB_VSCROLL ) ),
	aHSplitter( this, WinBits( WB_HSCROLL ) ),
	aWatchWindow( this ),
	aStackWindow( this ),
	bVSplitted(FALSE),
	bHSplitted(FALSE),
    m_pModulWindow(0),
    m_aImagesNormal(IDEResId(RID_IMGLST_LAYOUT)),
    m_aImagesHighContrast(IDEResId(RID_IMGLST_LAYOUT_HC))
{
    SetBackground(GetSettings().GetStyleSettings().GetWindowColor());

	aVSplitter.SetSplitHdl( LINK( this, ModulWindowLayout, SplitHdl ) );
	aHSplitter.SetSplitHdl( LINK( this, ModulWindowLayout, SplitHdl ) );
	aVSplitter.Show();
	aHSplitter.Show();

	aWatchWindow.Show();
	aStackWindow.Show();

    Color aColor(GetSettings().GetStyleSettings().GetFieldTextColor());
    m_aSyntaxColors[TT_UNKNOWN] = aColor;
    m_aSyntaxColors[TT_WHITESPACE] = aColor;
    m_aSyntaxColors[TT_EOL] = aColor;
    StartListening(m_aColorConfig);
    m_aSyntaxColors[TT_IDENTIFIER]
        = Color(m_aColorConfig.GetColorValue(svtools::BASICIDENTIFIER).nColor);
    m_aSyntaxColors[TT_NUMBER]
        = Color(m_aColorConfig.GetColorValue(svtools::BASICNUMBER).nColor);
    m_aSyntaxColors[TT_STRING]
        = Color(m_aColorConfig.GetColorValue(svtools::BASICSTRING).nColor);
    m_aSyntaxColors[TT_COMMENT]
        = Color(m_aColorConfig.GetColorValue(svtools::BASICCOMMENT).nColor);
    m_aSyntaxColors[TT_ERROR]
        = Color(m_aColorConfig.GetColorValue(svtools::BASICERROR).nColor);
    m_aSyntaxColors[TT_OPERATOR]
        = Color(m_aColorConfig.GetColorValue(svtools::BASICOPERATOR).nColor);
    m_aSyntaxColors[TT_KEYWORDS]
        = Color(m_aColorConfig.GetColorValue(svtools::BASICKEYWORD).nColor);

	Font aFont( GetFont() );
	Size aSz( aFont.GetSize() );
	aSz.Height() *= 3;
	aSz.Height() /= 2;
	aFont.SetSize( aSz );
	aFont.SetWeight( WEIGHT_BOLD );
    aFont.SetColor(GetSettings().GetStyleSettings().GetWindowTextColor());
	SetFont( aFont );
}

ModulWindowLayout::~ModulWindowLayout()
{
    EndListening(m_aColorConfig);
}

void __EXPORT ModulWindowLayout::Resize()
{
	// ScrollBars, etc. passiert in BasicIDEShell:Adjust...
	ArrangeWindows();
//	Invalidate();
}

void __EXPORT ModulWindowLayout::Paint( const Rectangle& )
{
	DrawText( Point(), String( IDEResId( RID_STR_NOMODULE ) ) );
}


void ModulWindowLayout::ArrangeWindows()
{
	Size aSz = GetOutputSizePixel();

	// prueffen, ob der Splitter in einem gueltigen Bereich liegt...
	long nMinPos = SPLIT_MARGIN;
	long nMaxPos = aSz.Height() - SPLIT_MARGIN;

	long nVSplitPos = aVSplitter.GetSplitPosPixel();
	long nHSplitPos = aHSplitter.GetSplitPosPixel();
	if ( !bVSplitted )
	{
		// Wenn noch nie gesplitted wurde, Verhaeltniss = 3 : 4
		nVSplitPos = aSz.Height() * 3 / 4;
		aVSplitter.SetSplitPosPixel( nVSplitPos );
	}
	if ( !bHSplitted )
	{
		// Wenn noch nie gesplitted wurde, Verhaeltniss = 2 : 3
		nHSplitPos = aSz.Width() * 2 / 3;
		aHSplitter.SetSplitPosPixel( nHSplitPos );
	}
	if ( ( nVSplitPos < nMinPos ) || ( nVSplitPos > nMaxPos ) )
		nVSplitPos = ( nVSplitPos < nMinPos ) ? 0 : ( aSz.Height() - SPLIT_HEIGHT );

	Size aXEWSz;
	aXEWSz.Width() = aSz.Width();
	aXEWSz.Height() = nVSplitPos + 1;
	if ( m_pModulWindow )
	{
		DBG_CHKOBJ( m_pModulWindow, ModulWindow, 0 );
		m_pModulWindow->SetPosSizePixel( Point( 0, 0 ), aXEWSz );
	}

	aVSplitter.SetDragRectPixel( Rectangle( Point( 0, 0 ), Size( aSz.Width(), aSz.Height() ) ) );
	aVSplitter.SetPosPixel( Point( 0, nVSplitPos ) );
	aVSplitter.SetSizePixel( Size( aSz.Width(), SPLIT_HEIGHT ) );

	aHSplitter.SetDragRectPixel( Rectangle( Point( 0, nVSplitPos+SPLIT_HEIGHT ), Size( aSz.Width(), aSz.Height() - nVSplitPos - SPLIT_HEIGHT ) ) );
	aHSplitter.SetPosPixel( Point( nHSplitPos, nVSplitPos ) );
	aHSplitter.SetSizePixel( Size( SPLIT_HEIGHT, aSz.Height() - nVSplitPos ) );

	Size aWWSz;
	Point aWWPos( 0, nVSplitPos+SPLIT_HEIGHT );
	aWWSz.Width() = nHSplitPos;
	aWWSz.Height() = aSz.Height() - aWWPos.Y();
	if ( !aWatchWindow.IsFloatingMode() )
		aWatchWindow.SetPosSizePixel( aWWPos, aWWSz );

	Size aSWSz;
	Point aSWPos( nHSplitPos+SPLIT_HEIGHT, nVSplitPos+SPLIT_HEIGHT );
	aSWSz.Width() = aSz.Width() - aSWPos.X();
	aSWSz.Height() = aSz.Height() - aSWPos.Y();
	if ( !aStackWindow.IsFloatingMode() )
		aStackWindow.SetPosSizePixel( aSWPos, aSWSz );

    if ( aStackWindow.IsFloatingMode() && aWatchWindow.IsFloatingMode() )
		aHSplitter.Hide();
	else
		aHSplitter.Show();

	long nHDoubleClickSplitPosX = aSz.Width()-aHSplitter.GetSizePixel().Width();
	if ( aHSplitter.GetSplitPosPixel() < nHDoubleClickSplitPosX )
		aHSplitter.SetLastSplitPosPixel( nHDoubleClickSplitPosX );


	long nHDoubleClickSplitPosY = aSz.Height()-aVSplitter.GetSizePixel().Height();
	if ( aVSplitter.GetSplitPosPixel() < nHDoubleClickSplitPosY )
		aVSplitter.SetLastSplitPosPixel( nHDoubleClickSplitPosY );
}

IMPL_LINK( ModulWindowLayout, SplitHdl, Splitter *, pSplitter )
{
	if ( pSplitter == &aVSplitter )
		bVSplitted = TRUE;
	else
		bHSplitted = TRUE;

	ArrangeWindows();
	return 0;
}

BOOL ModulWindowLayout::IsToBeDocked( DockingWindow* pDockingWindow, const Point& rPos, Rectangle& rRect )
{
	// prueffen, ob als Dock oder als Child:
	// TRUE:	Floating
	// FALSE:	Child
	Point aPosInMe = ScreenToOutputPixel( rPos );
	Size aSz = GetOutputSizePixel();
	if ( ( aPosInMe.X() > 0 ) && ( aPosInMe.X() < aSz.Width() ) &&
		 ( aPosInMe.Y() > 0 ) && ( aPosInMe.Y() < aSz.Height() ) )
	{
		long nVSplitPos = aVSplitter.GetSplitPosPixel();
		long nHSplitPos = aHSplitter.GetSplitPosPixel();
		if ( pDockingWindow == &aWatchWindow )
		{
			if ( ( aPosInMe.Y() > nVSplitPos ) && ( aPosInMe.X() < nHSplitPos ) )
			{
				rRect.SetSize( Size( nHSplitPos, aSz.Height() - nVSplitPos ) );
				rRect.SetPos( OutputToScreenPixel( Point( 0, nVSplitPos ) ) );
				return TRUE;
			}
		}
		if ( pDockingWindow == &aStackWindow )
		{
			if ( ( aPosInMe.Y() > nVSplitPos ) && ( aPosInMe.X() > nHSplitPos ) )
			{
				rRect.SetSize( Size( aSz.Width() - nHSplitPos, aSz.Height() - nVSplitPos ) );
				rRect.SetPos( OutputToScreenPixel( Point( nHSplitPos, nVSplitPos ) ) );
				return TRUE;
			}
		}
	}
	return FALSE;
}

void ModulWindowLayout::DockaWindow( DockingWindow* pDockingWindow )
{
	if ( pDockingWindow == &aWatchWindow )
	{
		// evtl. Sonderbehandlung...
		ArrangeWindows();
	}
	else if ( pDockingWindow == &aStackWindow )
	{
		// evtl. Sonderbehandlung...
		ArrangeWindows();
	}
#ifndef PRODUCT
	else
		DBG_ERROR( "Wer will sich denn hier andocken ?" );
#endif
}

void ModulWindowLayout::SetModulWindow( ModulWindow* pModWin )
{
    m_pModulWindow = pModWin;
	ArrangeWindows();
}

// virtual
void ModulWindowLayout::DataChanged(DataChangedEvent const & rDCEvt)
{
    Window::DataChanged(rDCEvt);
    if (rDCEvt.GetType() == DATACHANGED_SETTINGS
        && (rDCEvt.GetFlags() & SETTINGS_STYLE) != 0)
    {
        bool bInvalidate = false;
        Color aColor(GetSettings().GetStyleSettings().GetWindowColor());
        if (aColor
            != rDCEvt.GetOldSettings()->GetStyleSettings().GetWindowColor())
        {
            SetBackground(Wallpaper(aColor));
            bInvalidate = true;
        }
        aColor = GetSettings().GetStyleSettings().GetWindowTextColor();
        if (aColor != rDCEvt.GetOldSettings()->
            GetStyleSettings().GetWindowTextColor())
        {
            Font aFont(GetFont());
            aFont.SetColor(aColor);
            SetFont(aFont);
            bInvalidate = true;
        }
        if (bInvalidate)
            Invalidate();
        aColor = GetSettings().GetStyleSettings().GetFieldTextColor();
        if (aColor != m_aSyntaxColors[TT_UNKNOWN])
        {
            m_aSyntaxColors[TT_UNKNOWN] = aColor;
            m_aSyntaxColors[TT_WHITESPACE] = aColor;
            m_aSyntaxColors[TT_EOL] = aColor;
            updateSyntaxHighlighting();
        }
    }
}

// virtual
void ModulWindowLayout::Notify(SfxBroadcaster & rBc, SfxHint const & rHint)
{
	(void)rBc;

    if (rHint.ISA(SfxSimpleHint)
        && (static_cast< SfxSimpleHint const & >(rHint).GetId()
            == SFX_HINT_COLORS_CHANGED))
    {
        Color aColor(m_aColorConfig.GetColorValue(svtools::BASICIDENTIFIER).
                     nColor);
        bool bChanged = aColor != m_aSyntaxColors[TT_IDENTIFIER];
        m_aSyntaxColors[TT_IDENTIFIER] = aColor;
        aColor = Color(m_aColorConfig.GetColorValue(svtools::BASICNUMBER).nColor);
	if (bChanged || aColor != m_aSyntaxColors[TT_NUMBER])
            bChanged = true;
        m_aSyntaxColors[TT_NUMBER] = aColor;
        aColor = Color(m_aColorConfig.GetColorValue(svtools::BASICSTRING).nColor);
        if (bChanged || aColor != m_aSyntaxColors[TT_STRING])
            bChanged = true;
        m_aSyntaxColors[TT_STRING] = aColor;
        aColor = Color(m_aColorConfig.GetColorValue(svtools::BASICCOMMENT).
                       nColor);
        if (bChanged || aColor != m_aSyntaxColors[TT_COMMENT])
            bChanged = true;
        m_aSyntaxColors[TT_COMMENT] = aColor;
        aColor = Color(m_aColorConfig.GetColorValue(svtools::BASICERROR).nColor);
        if (bChanged || aColor != m_aSyntaxColors[TT_ERROR])
            bChanged = true;
        m_aSyntaxColors[TT_ERROR] = aColor;
        aColor = Color(m_aColorConfig.GetColorValue(svtools::BASICOPERATOR).
                       nColor);
        if (bChanged || aColor != m_aSyntaxColors[TT_OPERATOR])
            bChanged = true;
        m_aSyntaxColors[TT_OPERATOR] = aColor;
        aColor = Color(m_aColorConfig.GetColorValue(svtools::BASICKEYWORD).
                       nColor);
        if (bChanged || aColor != m_aSyntaxColors[TT_KEYWORDS])
            bChanged = true;
        m_aSyntaxColors[TT_KEYWORDS] = aColor;
        if (bChanged)
            updateSyntaxHighlighting();
    }
}

void ModulWindowLayout::updateSyntaxHighlighting()
{
    if (m_pModulWindow != 0)
    {
        EditorWindow & rEditor = m_pModulWindow->GetEditorWindow();
        ULONG nCount = rEditor.GetEditEngine()->GetParagraphCount();
        for (ULONG i = 0; i < nCount; ++i)
            rEditor.DoDelayedSyntaxHighlight(i);
    }
}

Image ModulWindowLayout::getImage(USHORT nId, bool bHighContrastMode) const
{
    return (bHighContrastMode ? m_aImagesHighContrast : m_aImagesNormal).
        GetImage(nId);
}
