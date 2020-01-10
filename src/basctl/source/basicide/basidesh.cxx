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

// CLOOKS:
//#define _MENUBTN_HXX
#define _SPIN_HXX
#define _PRVWIN_HXX
//#define _FIELD_HXX ***
//#define _TAB_HXX ***
#define _DIALOGS_HXX
#define _SVRTF_HXX
#define _ISETBRW_HXX
#define _VCTRLS_HXX
#define SI_NOCONTROL
#define SI_NOSBXCONTROLS

#define ITEMID_SIZE	0

// Falls ohne PCH's:
#include <ide_pch.hxx>


#define _SOLAR__PRIVATE 1
#include <basic/sbx.hxx>
#include <svtools/hint.hxx>
#include <tools/diagnose_ex.h>
#include <basidesh.hrc>
#include <basidesh.hxx>
#include <basdoc.hxx>
#include <basobj.hxx>
#include <bastypes.hxx>
#include <basicbox.hxx>
#include <objdlg.hxx>
#include <sbxitem.hxx>
#include <tbxctl.hxx>
#include <iderdll2.hxx>
#include <basidectrlr.hxx>
#include <localizationmgr.hxx>

#define BasicIDEShell
#define SFX_TYPEMAP
#include <idetemp.hxx>
#include <basslots.hxx>
#include <iderdll.hxx>
#include <svx/pszctrl.hxx>
#include <svx/insctrl.hxx>
#include <svx/srchdlg.hxx>
#include <svx/lboxctrl.hxx>
#include <svx/tbcontrl.hxx>
#include <com/sun/star/script/XLibraryContainer.hpp>
#include <com/sun/star/script/XLibraryContainerPassword.hpp>
#include <com/sun/star/container/XNameContainer.hpp>

#include <svx/xmlsecctrl.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star;
using ::rtl::OUString;


TYPEINIT1( BasicIDEShell, SfxViewShell );

SFX_IMPL_VIEWFACTORY( BasicIDEShell, IDEResId( SVX_INTERFACE_BASIDE_VIEWSH ) )
{
	SFX_VIEW_REGISTRATION( BasicDocShell );
}


// MI: Prinzipiel IDL, aber ich lieber doch nicht?
// SFX_IMPL_ /*IDL_*/ INTERFACE( BasicIDEShell, SfxViewShell, IDEResId( RID_STR_IDENAME ) )
SFX_IMPL_INTERFACE( BasicIDEShell, SfxViewShell, IDEResId( RID_STR_IDENAME ) )
{
	SFX_CHILDWINDOW_REGISTRATION( SID_SEARCH_DLG );
    SFX_FEATURED_CHILDWINDOW_REGISTRATION(SID_SHOW_PROPERTYBROWSER, BASICIDE_UI_FEATURE_SHOW_BROWSER);
    SFX_POPUPMENU_REGISTRATION( IDEResId( RID_POPUP_DLGED ) );
}



#define IDE_VIEWSHELL_FLAGS		SFX_VIEW_MAXIMIZE_FIRST|SFX_VIEW_CAN_PRINT|SFX_VIEW_NO_NEWWINDOW


// Hack for #101048
static sal_Int32 GnBasicIDEShellCount;
sal_Int32 getBasicIDEShellCount( void )
    { return GnBasicIDEShellCount; }

BasicIDEShell::BasicIDEShell( SfxViewFrame* pFrame_, SfxViewShell* /* pOldShell */ ) :
		SfxViewShell( pFrame_, IDE_VIEWSHELL_FLAGS ),
        m_aCurDocument( ScriptDocument::getApplicationScriptDocument() ),
		aHScrollBar( &GetViewFrame()->GetWindow(), WinBits( WB_HSCROLL | WB_DRAG ) ),
		aVScrollBar( &GetViewFrame()->GetWindow(), WinBits( WB_VSCROLL | WB_DRAG ) ),
		aScrollBarBox( &GetViewFrame()->GetWindow(), WinBits( WB_SIZEABLE ) ),
        m_bAppBasicModified( FALSE ),
        m_aNotifier( *this )
{
	Init();
    GnBasicIDEShellCount++;
}



void BasicIDEShell::Init()
{
	TbxControls::RegisterControl( SID_CHOOSE_CONTROLS );
	SvxPosSizeStatusBarControl::RegisterControl();
	SvxInsertStatusBarControl::RegisterControl();
	XmlSecStatusBarControl::RegisterControl( SID_SIGNATURE );
    SvxSimpleUndoRedoController::RegisterControl( SID_UNDO );
    SvxSimpleUndoRedoController::RegisterControl( SID_REDO );

	SvxSearchDialogWrapper::RegisterChildWindow( sal_False );

	IDE_DLL()->GetExtraData()->ShellInCriticalSection() = TRUE;

	SetName( String( RTL_CONSTASCII_USTRINGPARAM( "BasicIDE" ) ) );
	SetHelpId( SVX_INTERFACE_BASIDE_VIEWSH );

	SFX_APP()->EnterBasicCall();

	LibBoxControl::RegisterControl( SID_BASICIDE_LIBSELECTOR );
	LanguageBoxControl::RegisterControl( SID_BASICIDE_CURRENT_LANG );

	CreateModulWindowLayout();

    GetViewFrame()->GetWindow().SetBackground();

	pCurWin = 0;
    m_aCurDocument = ScriptDocument::getApplicationScriptDocument();
	pObjectCatalog = 0;
	bCreatingWindow = FALSE;

	m_pCurLocalizationMgr = NULL;

	pTabBar = new BasicIDETabBar( &GetViewFrame()->GetWindow() );
	pTabBar->SetSplitHdl( LINK( this, BasicIDEShell, TabBarSplitHdl ) );
	bTabBarSplitted = FALSE;

	nCurKey = 100;
	InitScrollBars();
	InitTabBar();

    SetCurLib( ScriptDocument::getApplicationScriptDocument(), String::CreateFromAscii( "Standard" ), false, false );

    if ( IDE_DLL() && IDE_DLL()->pShell == NULL )
        IDE_DLL()->pShell = this;

    IDE_DLL()->GetExtraData()->ShellInCriticalSection() = FALSE;

    // It's enough to create the controller ...
    // It will be public by using magic :-)
    new BasicIDEController( this );

    // Force updating the title ! Because it must be set to the controller
    // it has to be called directly after creating those controller.
    SetMDITitle ();

	UpdateWindows();
}

__EXPORT BasicIDEShell::~BasicIDEShell()
{
    m_aNotifier.dispose();

    if ( IDE_DLL() && IDE_DLL()->pShell == this )
        IDE_DLL()->pShell = NULL;

	// Damit bei einem Basic-Fehler beim Speichern die Shell nicht sofort
	// wieder hoch kommt:
	IDE_DLL()->GetExtraData()->ShellInCriticalSection() = TRUE;

	SetWindow( 0 );
	SetCurWindow( 0 );

	// Alle Fenster zerstoeren:
	IDEBaseWindow* pWin = aIDEWindowTable.First();
	while ( pWin )
	{
		// Kein Store, passiert bereits, wenn die BasicManager zerstoert werden.
		delete pWin;
		pWin = aIDEWindowTable.Next();
	}

	aIDEWindowTable.Clear();
	delete pTabBar;
	delete pObjectCatalog;
	DestroyModulWindowLayout();
	// MI: Das gab einen GPF im SDT beim Schliessen da dann der ViewFrame die
	// ObjSh loslaesst. Es wusste auch keiner mehr wozu das gut war.
	// GetViewFrame()->GetObjectShell()->Broadcast( SfxSimpleHint( SFX_HINT_DYING ) );

	SFX_APP()->LeaveBasicCall();
	IDE_DLL()->GetExtraData()->ShellInCriticalSection() = FALSE;

    GnBasicIDEShellCount--;
}

void BasicIDEShell::onDocumentCreated( const ScriptDocument& /*_rDocument*/ )
{
    UpdateWindows();
}

void BasicIDEShell::onDocumentOpened( const ScriptDocument& /*_rDocument*/ )
{
    UpdateWindows();
}

void BasicIDEShell::onDocumentSave( const ScriptDocument& /*_rDocument*/ )
{
    StoreAllWindowData();
}

void BasicIDEShell::onDocumentSaveDone( const ScriptDocument& /*_rDocument*/ )
{
    // not interested in
}

void BasicIDEShell::onDocumentSaveAs( const ScriptDocument& /*_rDocument*/ )
{
    StoreAllWindowData();
}

void BasicIDEShell::onDocumentSaveAsDone( const ScriptDocument& /*_rDocument*/ )
{
    // not interested in
}

void BasicIDEShell::onDocumentClosed( const ScriptDocument& _rDocument )
{
    if ( !_rDocument.isValid() )
        return;

    bool bSetCurWindow = false;
    bool bSetCurLib = ( _rDocument == m_aCurDocument );

    // remove all windows which belong to this document
    for ( ULONG nWin = aIDEWindowTable.Count(); nWin; )
    {
        IDEBaseWindow* pWin = aIDEWindowTable.GetObject( --nWin );
        if ( pWin->IsDocument( _rDocument ) )
        {
            if ( pWin->GetStatus() & (BASWIN_RUNNINGBASIC|BASWIN_INRESCHEDULE) )
            {
                pWin->AddStatus( BASWIN_TOBEKILLED );
                pWin->Hide();
                StarBASIC::Stop();
                // there's no notify
                pWin->BasicStopped();
            }
            else
            {
                pWin->StoreData();
                if ( pWin == pCurWin )
                    bSetCurWindow = true;
                RemoveWindow( pWin, TRUE, FALSE );
            }
        }
    }

    // remove lib info
    BasicIDEData* pData = IDE_DLL()->GetExtraData();
    if ( pData )
        pData->GetLibInfos().RemoveInfoFor( _rDocument );

    if ( bSetCurLib )
        SetCurLib( ScriptDocument::getApplicationScriptDocument(), String::CreateFromAscii( "Standard" ), true, false );
    else if ( bSetCurWindow )
        SetCurWindow( FindApplicationWindow(), TRUE );
}

void BasicIDEShell::onDocumentTitleChanged( const ScriptDocument& /*_rDocument*/ )
{
    SfxBindings* pBindings = BasicIDE::GetBindingsPtr();
    if ( pBindings )
        pBindings->Invalidate( SID_BASICIDE_LIBSELECTOR, TRUE, FALSE );
    SetMDITitle();
}

void BasicIDEShell::onDocumentModeChanged( const ScriptDocument& _rDocument )
{
    for ( ULONG nWin = aIDEWindowTable.Count(); nWin; )
    {
        IDEBaseWindow* pWin = aIDEWindowTable.GetObject( --nWin );
        if ( pWin->IsDocument( _rDocument ) && _rDocument.isDocument() )
            pWin->SetReadOnly( _rDocument.isReadOnly() );
    }
}

void BasicIDEShell::StoreAllWindowData( BOOL bPersistent )
{
	for ( ULONG nWin = 0; nWin < aIDEWindowTable.Count(); nWin++ )
	{
		IDEBaseWindow* pWin = aIDEWindowTable.GetObject( nWin );
		DBG_ASSERT( pWin, "PrepareClose: NULL-Pointer in Table?" );
		if ( !pWin->IsSuspended() )
			pWin->StoreData();
	}

	if ( bPersistent  )
	{
		SFX_APP()->SaveBasicAndDialogContainer();
        SetAppBasicModified( FALSE );

        SfxBindings* pBindings = BasicIDE::GetBindingsPtr();
        if ( pBindings )
        {
            pBindings->Invalidate( SID_SAVEDOC );
            pBindings->Update( SID_SAVEDOC );
        }
	}
}


USHORT __EXPORT BasicIDEShell::PrepareClose( BOOL bUI, BOOL bForBrowsing )
{
	(void)bForBrowsing;

	// da es nach Drucken etc. (DocInfo) modifiziert ist, hier resetten
	GetViewFrame()->GetObjectShell()->SetModified(FALSE);

	if ( StarBASIC::IsRunning() )
	{
        if( bUI )
        {
		    String aErrorStr( IDEResId( RID_STR_CANNOTCLOSE ) );
		    Window *pParent = &GetViewFrame()->GetWindow();
		    InfoBox( pParent, aErrorStr ).Execute();
        }
		return FALSE;
	}
	else
	{
		// Hier unguenstig, wird zweimal gerufen...
//		StoreAllWindowData();

		BOOL bCanClose = TRUE;
		for ( ULONG nWin = 0; bCanClose && ( nWin < aIDEWindowTable.Count() ); nWin++ )
		{
			IDEBaseWindow* pWin = aIDEWindowTable.GetObject( nWin );
			if ( /* !pWin->IsSuspended() && */ !pWin->CanClose() )
			{
                if ( m_aCurLibName.Len() && ( pWin->IsDocument( m_aCurDocument ) || pWin->GetLibName() != m_aCurLibName ) )
                    SetCurLib( ScriptDocument::getApplicationScriptDocument(), String(), false );
				SetCurWindow( pWin, TRUE );
				bCanClose = FALSE;
			}
		}

		if ( bCanClose )
			StoreAllWindowData( FALSE );	// Nicht auf Platte schreiben, das passiert am Ende automatisch

		return bCanClose;
	}
}

void BasicIDEShell::InitScrollBars()
{
	aVScrollBar.SetLineSize( 300 );
	aVScrollBar.SetPageSize( 2000 );
	aHScrollBar.SetLineSize( 300 );
	aHScrollBar.SetPageSize( 2000 );
	aHScrollBar.Enable();
	aVScrollBar.Enable();
	aVScrollBar.Show();
	aHScrollBar.Show();
	aScrollBarBox.Show();
}



void BasicIDEShell::InitTabBar()
{
	pTabBar->Enable();
	pTabBar->Show();
	pTabBar->SetSelectHdl( LINK( this, BasicIDEShell, TabBarHdl ) );
}


Size __EXPORT BasicIDEShell::GetOptimalSizePixel() const
{
	return Size( 400, 300 );
}



void __EXPORT BasicIDEShell::OuterResizePixel( const Point &rPos, const Size &rSize )
{
	// Adjust fliegt irgendwann raus...
	AdjustPosSizePixel( rPos, rSize );
}


IMPL_LINK_INLINE_START( BasicIDEShell, TabBarSplitHdl, TabBar *, pTBar )
{
	(void)pTBar;
	bTabBarSplitted = TRUE;
	ArrangeTabBar();

	return 0;
}
IMPL_LINK_INLINE_END( BasicIDEShell, TabBarSplitHdl, TabBar *, pTBar )



IMPL_LINK( BasicIDEShell, TabBarHdl, TabBar *, pCurTabBar )
{
	USHORT nCurId = pCurTabBar->GetCurPageId();
	IDEBaseWindow* pWin = aIDEWindowTable.Get( nCurId );
	DBG_ASSERT( pWin, "Eintrag in TabBar passt zu keinem Fenster!" );
	SetCurWindow( pWin );

	return 0;
}



BOOL BasicIDEShell::NextPage( BOOL bPrev )
{
	BOOL bRet = FALSE;
	USHORT nPos = pTabBar->GetPagePos( pTabBar->GetCurPageId() );

	if ( bPrev )
		--nPos;
	else
		++nPos;

	if ( nPos < pTabBar->GetPageCount() )
	{
		IDEBaseWindow* pWin = aIDEWindowTable.Get( pTabBar->GetPageId( nPos ) );
		SetCurWindow( pWin, TRUE );
		bRet = TRUE;
	}

	return bRet;
}



void BasicIDEShell::ArrangeTabBar()
{
	Size aSz( GetViewFrame()->GetWindow().GetOutputSizePixel() );
	long nBoxPos = aScrollBarBox.GetPosPixel().X() - 1;
	long nPos = pTabBar->GetSplitSize();
	if ( nPos <= nBoxPos )
	{
		Point aPnt( pTabBar->GetPosPixel() );
		long nH = aHScrollBar.GetSizePixel().Height();
		pTabBar->SetPosSizePixel( aPnt, Size( nPos, nH ) );
		long nScrlStart = aPnt.X() + nPos;
		aHScrollBar.SetPosSizePixel( Point( nScrlStart, aPnt.Y() ), Size( nBoxPos - nScrlStart + 2, nH ) );
		aHScrollBar.Update();
	}
}



SfxUndoManager* BasicIDEShell::GetUndoManager()
{
	SfxUndoManager* pMgr = NULL;
	if( pCurWin )
		pMgr = pCurWin->GetUndoManager();

	return pMgr;
}



void BasicIDEShell::ShowObjectDialog( BOOL bShow, BOOL bCreateOrDestroy )
{
	if ( bShow )
	{
		if ( !pObjectCatalog && bCreateOrDestroy )
		{
			pObjectCatalog = new ObjectCatalog( &GetViewFrame()->GetWindow() );
			// Position wird in BasicIDEData gemerkt und vom Dlg eingestellt
            if ( pObjectCatalog )
            {
                pObjectCatalog->SetCancelHdl( LINK( this, BasicIDEShell, ObjectDialogCancelHdl ) );
                BasicEntryDescriptor aDesc;
                IDEBaseWindow* pCurWin_ = GetCurWindow();
                if ( pCurWin_ )
                    aDesc = pCurWin_->CreateEntryDescriptor();
                pObjectCatalog->SetCurrentEntry( aDesc );
            }
		}

		// Die allerletzten Aenderungen...
		if ( pCurWin )
			pCurWin->StoreData();

		if ( pObjectCatalog )
		{
			pObjectCatalog->UpdateEntries();
			pObjectCatalog->Show();
		}
	}
	else if ( pObjectCatalog )
	{
		pObjectCatalog->Hide();
		if ( bCreateOrDestroy )
		{
			// Wegen OS/2-Focus-Problem pObjectCatalog vorm delete auf NULL
			ObjectCatalog* pTemp = pObjectCatalog;
			pObjectCatalog = 0;
			delete pTemp;
		}
	}
}



void __EXPORT BasicIDEShell::SFX_NOTIFY( SfxBroadcaster& rBC, const TypeId&,
										const SfxHint& rHint, const TypeId& )
{
    if ( IDE_DLL()->GetShell() )
    {
        if ( rHint.IsA( TYPE( SfxSimpleHint ) ) )
        {
            switch ( ((SfxSimpleHint&)rHint).GetId() )
            {
                case SFX_HINT_DYING:
                {
                    EndListening( rBC, TRUE /* Alle abmelden */ );
                    if ( pObjectCatalog )
                        pObjectCatalog->UpdateEntries();
                }
                break;
            }

            if ( rHint.IsA( TYPE( SbxHint ) ) )
            {
                SbxHint& rSbxHint = (SbxHint&)rHint;
                ULONG nHintId = rSbxHint.GetId();
                if (	( nHintId == SBX_HINT_BASICSTART ) ||
                        ( nHintId == SBX_HINT_BASICSTOP ) )
                {
                    SfxBindings* pBindings = BasicIDE::GetBindingsPtr();
                    if ( pBindings )
                    {
                        pBindings->Invalidate( SID_BASICRUN );
                        pBindings->Update( SID_BASICRUN );
                        pBindings->Invalidate( SID_BASICCOMPILE );
                        pBindings->Update( SID_BASICCOMPILE );
                        pBindings->Invalidate( SID_BASICSTEPOVER );
                        pBindings->Update( SID_BASICSTEPOVER );
                        pBindings->Invalidate( SID_BASICSTEPINTO );
                        pBindings->Update( SID_BASICSTEPINTO );
                        pBindings->Invalidate( SID_BASICSTEPOUT );
                        pBindings->Update( SID_BASICSTEPOUT );
                        pBindings->Invalidate( SID_BASICSTOP );
                        pBindings->Update( SID_BASICSTOP );
                        pBindings->Invalidate( SID_BASICIDE_TOGGLEBRKPNT );
                        pBindings->Update( SID_BASICIDE_TOGGLEBRKPNT );
                        pBindings->Invalidate( SID_BASICIDE_MANAGEBRKPNTS );
                        pBindings->Update( SID_BASICIDE_MANAGEBRKPNTS );
                        pBindings->Invalidate( SID_BASICIDE_MODULEDLG );
                        pBindings->Update( SID_BASICIDE_MODULEDLG );
                        pBindings->Invalidate( SID_BASICLOAD );
                        pBindings->Update( SID_BASICLOAD );
                    }

                    if ( nHintId == SBX_HINT_BASICSTOP )
                    {
                        // Nicht nur bei Error/Break oder explizitem anhalten,
                        // falls durch einen Programmierfehler das Update abgeschaltet ist.
                        BasicIDE::BasicStopped();
                        UpdateModulWindowLayout( true );    // Leer machen...
						if( m_pCurLocalizationMgr )
							m_pCurLocalizationMgr->handleBasicStopped();
                    }
					else if( m_pCurLocalizationMgr )
                    {
						m_pCurLocalizationMgr->handleBasicStarted();
                    }

                    IDEBaseWindow* pWin = aIDEWindowTable.First();
                    while ( pWin )
                    {
                        if ( nHintId == SBX_HINT_BASICSTART )
                            pWin->BasicStarted();
                        else
                            pWin->BasicStopped();
                        pWin = aIDEWindowTable.Next();
                    }
                }
            }
        }
    }
}



void BasicIDEShell::CheckWindows()
{
	BOOL bSetCurWindow = FALSE;
	for ( ULONG nWin = 0; nWin < aIDEWindowTable.Count(); nWin++ )
	{
		IDEBaseWindow* pWin = aIDEWindowTable.GetObject( nWin );
		if ( pWin->GetStatus() & BASWIN_TOBEKILLED )
		{
			pWin->StoreData();
			if ( pWin == pCurWin )
				bSetCurWindow = TRUE;
			RemoveWindow( pWin, TRUE, FALSE );
			nWin--;
		}
	}
	if ( bSetCurWindow )
		SetCurWindow( FindApplicationWindow(), TRUE );
}



void BasicIDEShell::RemoveWindows( const ScriptDocument& rDocument, const String& rLibName, BOOL bDestroy )
{
	BOOL bChangeCurWindow = pCurWin ? FALSE : TRUE;
	for ( ULONG nWin = 0; nWin < aIDEWindowTable.Count(); nWin++ )
	{
		IDEBaseWindow* pWin = aIDEWindowTable.GetObject( nWin );
        if ( pWin->IsDocument( rDocument ) && pWin->GetLibName() == rLibName )
		{
			if ( pWin == pCurWin )
				bChangeCurWindow = TRUE;
			pWin->StoreData();
			RemoveWindow( pWin, bDestroy, FALSE );
			nWin--;
		}
	}
	if ( bChangeCurWindow )
		SetCurWindow( FindApplicationWindow(), TRUE );
}



void BasicIDEShell::UpdateWindows()
{
	// Alle Fenster, die nicht angezeigt werden duerfen, entfernen
	BOOL bChangeCurWindow = pCurWin ? FALSE : TRUE;
    if ( m_aCurLibName.Len() )
	{
		for ( ULONG nWin = 0; nWin < aIDEWindowTable.Count(); nWin++ )
		{
			IDEBaseWindow* pWin = aIDEWindowTable.GetObject( nWin );
            if ( !pWin->IsDocument( m_aCurDocument ) || pWin->GetLibName() != m_aCurLibName )
			{
				if ( pWin == pCurWin )
					bChangeCurWindow = TRUE;
				pWin->StoreData();
				// Die Abfrage auf RUNNING verhindert den Absturz, wenn in Reschedule.
				// Fenster bleibt erstmal stehen, spaeter sowieso mal umstellen,
				// dass Fenster nur als Hidden markiert werden und nicht
				// geloescht.
				if ( !(pWin->GetStatus() & ( BASWIN_TOBEKILLED | BASWIN_RUNNINGBASIC | BASWIN_SUSPENDED ) ) )
				{
					RemoveWindow( pWin, FALSE, FALSE );
					nWin--;
				}
			}
		}
	}

	if ( bCreatingWindow )
		return;

    IDEBaseWindow* pNextActiveWindow = 0;

	// Alle anzuzeigenden Fenster anzeigen
    ScriptDocuments aDocuments( ScriptDocument::getAllScriptDocuments( ScriptDocument::AllWithApplication ) );
    for (   ScriptDocuments::const_iterator doc = aDocuments.begin();
            doc != aDocuments.end();
            ++doc
        )
	{
		StartListening( *doc->getBasicManager(), TRUE /* Nur einmal anmelden */ );

        // libraries
        Sequence< ::rtl::OUString > aLibNames( doc->getLibraryNames() );
        sal_Int32 nLibCount = aLibNames.getLength();
	    const ::rtl::OUString* pLibNames = aLibNames.getConstArray();

        for ( sal_Int32 i = 0 ; i < nLibCount ; i++ )
	    {
            String aLibName = pLibNames[ i ];

            if ( !m_aCurLibName.Len() || ( *doc == m_aCurDocument && aLibName == m_aCurLibName ) )
            {
                // check, if library is password protected and not verified
                BOOL bProtected = FALSE;
                Reference< script::XLibraryContainer > xModLibContainer( doc->getLibraryContainer( E_SCRIPTS ) );
                if ( xModLibContainer.is() && xModLibContainer->hasByName( aLibName ) )
                {
                    Reference< script::XLibraryContainerPassword > xPasswd( xModLibContainer, UNO_QUERY );
                    if ( xPasswd.is() && xPasswd->isLibraryPasswordProtected( aLibName ) && !xPasswd->isLibraryPasswordVerified( aLibName ) )
                    {
                        bProtected = TRUE;
                    }
                }

                if ( !bProtected )
                {
                    LibInfoItem* pLibInfoItem = 0;
                    BasicIDEData* pData = IDE_DLL()->GetExtraData();
                    if ( pData )
                        pLibInfoItem = pData->GetLibInfos().GetInfo( LibInfoKey( *doc, aLibName ) );

                    // modules
                    if ( xModLibContainer.is() && xModLibContainer->hasByName( aLibName ) )
                    {
                        StarBASIC* pLib = doc->getBasicManager()->GetLib( aLibName );
                        if ( pLib )
                            ImplStartListening( pLib );

                        try
					    {
                            Sequence< ::rtl::OUString > aModNames( doc->getObjectNames( E_SCRIPTS, aLibName ) );
                            sal_Int32 nModCount = aModNames.getLength();
	                        const ::rtl::OUString* pModNames = aModNames.getConstArray();

                            for ( sal_Int32 j = 0 ; j < nModCount ; j++ )
				            {
					            String aModName = pModNames[ j ];
						        ModulWindow* pWin = FindBasWin( *doc, aLibName, aModName, FALSE );
                                if ( !pWin )
							        pWin = CreateBasWin( *doc, aLibName, aModName );
                                if ( !pNextActiveWindow && pLibInfoItem && pLibInfoItem->GetCurrentName() == aModName &&
                                        pLibInfoItem->GetCurrentType() == BASICIDE_TYPE_MODULE )
                                {
                                    pNextActiveWindow = (IDEBaseWindow*)pWin;
                                }
                            }
                        }
					    catch ( container::NoSuchElementException& )
					    {
                            DBG_UNHANDLED_EXCEPTION();
					    }
                    }

                    // dialogs
                    Reference< script::XLibraryContainer > xDlgLibContainer( doc->getLibraryContainer( E_DIALOGS ) );
                    if ( xDlgLibContainer.is() && xDlgLibContainer->hasByName( aLibName ) )
                    {
                        try
                        {
                            Sequence< ::rtl::OUString > aDlgNames = doc->getObjectNames( E_DIALOGS, aLibName );
                            sal_Int32 nDlgCount = aDlgNames.getLength();
	                        const ::rtl::OUString* pDlgNames = aDlgNames.getConstArray();

                            for ( sal_Int32 j = 0 ; j < nDlgCount ; j++ )
				            {
					            String aDlgName = pDlgNames[ j ];
                                // this find only looks for non-suspended windows;
                                // suspended windows are handled in CreateDlgWin
                                DialogWindow* pWin = FindDlgWin( *doc, aLibName, aDlgName, FALSE );
                                if ( !pWin )
								    pWin = CreateDlgWin( *doc, aLibName, aDlgName );
                                if ( !pNextActiveWindow && pLibInfoItem && pLibInfoItem->GetCurrentName() == aDlgName &&
                                        pLibInfoItem->GetCurrentType() == BASICIDE_TYPE_DIALOG )
                                {
                                    pNextActiveWindow = (IDEBaseWindow*)pWin;
                                }
                            }
                        }
					    catch ( container::NoSuchElementException& )
					    {
						    DBG_UNHANDLED_EXCEPTION();
					    }
                    }
				}
            }
        }
	}

	if ( bChangeCurWindow )
    {
        if ( !pNextActiveWindow )
            pNextActiveWindow = FindApplicationWindow();
        SetCurWindow( pNextActiveWindow, TRUE );
    }
}

void BasicIDEShell::RemoveWindow( IDEBaseWindow* pWindow_, BOOL bDestroy, BOOL bAllowChangeCurWindow )
{
	DBG_ASSERT( pWindow_, "Kann keinen NULL-Pointer loeschen!" );
	ULONG nKey = aIDEWindowTable.GetKey( pWindow_ );
	pTabBar->RemovePage( (USHORT)nKey );
	aIDEWindowTable.Remove( nKey );
	if ( pWindow_ == pCurWin )
	{
		if ( bAllowChangeCurWindow )
			SetCurWindow( FindApplicationWindow(), TRUE );
		else
			SetCurWindow( NULL, FALSE );
	}
	if ( bDestroy )
	{
		if ( !( pWindow_->GetStatus() & BASWIN_INRESCHEDULE ) )
		{
			delete pWindow_;
		}
		else
		{
			pWindow_->AddStatus( BASWIN_TOBEKILLED );
			pWindow_->Hide();
			StarBASIC::Stop();
			// Es kommt kein Notify...
			pWindow_->BasicStopped();
			aIDEWindowTable.Insert( nKey, pWindow_ );	// wieder einhaegen
		}
	}
	else
	{
		pWindow_->Hide();
		pWindow_->AddStatus( BASWIN_SUSPENDED );
		pWindow_->Deactivating();
		aIDEWindowTable.Insert( nKey, pWindow_ );	// wieder einhaegen
	}

}



USHORT BasicIDEShell::InsertWindowInTable( IDEBaseWindow* pNewWin )
{
	// Eigentlich prueffen,
	nCurKey++;
	aIDEWindowTable.Insert( nCurKey, pNewWin );
	return nCurKey;
}



void BasicIDEShell::InvalidateBasicIDESlots()
{
	// Nur die, die eine optische Auswirkung haben...

	if ( IDE_DLL()->GetShell() )
	{
        SfxBindings* pBindings = BasicIDE::GetBindingsPtr();
        if ( pBindings )
        {
            pBindings->Invalidate( SID_UNDO );
            pBindings->Invalidate( SID_REDO );
            pBindings->Invalidate( SID_SAVEDOC );
            pBindings->Invalidate( SID_SIGNATURE );
            pBindings->Invalidate( SID_BASICIDE_CHOOSEMACRO );
            pBindings->Invalidate( SID_BASICIDE_MODULEDLG );
            pBindings->Invalidate( SID_BASICIDE_OBJCAT );
            pBindings->Invalidate( SID_BASICSTOP );
            pBindings->Invalidate( SID_BASICRUN );
            pBindings->Invalidate( SID_BASICCOMPILE );
            pBindings->Invalidate( SID_BASICLOAD );
            pBindings->Invalidate( SID_BASICSAVEAS );
            pBindings->Invalidate( SID_BASICIDE_MATCHGROUP );
            pBindings->Invalidate( SID_BASICSTEPINTO );
            pBindings->Invalidate( SID_BASICSTEPOVER );
            pBindings->Invalidate( SID_BASICSTEPOUT );
            pBindings->Invalidate( SID_BASICIDE_TOGGLEBRKPNT );
            pBindings->Invalidate( SID_BASICIDE_MANAGEBRKPNTS );
            pBindings->Invalidate( SID_BASICIDE_ADDWATCH );
            pBindings->Invalidate( SID_BASICIDE_REMOVEWATCH );
            pBindings->Invalidate( SID_CHOOSE_CONTROLS );
            pBindings->Invalidate( SID_PRINTDOC );
            pBindings->Invalidate( SID_PRINTDOCDIRECT );
            pBindings->Invalidate( SID_SETUPPRINTER );
            pBindings->Invalidate( SID_DIALOG_TESTMODE );

            pBindings->Invalidate( SID_DOC_MODIFIED );
            pBindings->Invalidate( SID_BASICIDE_STAT_TITLE );
            pBindings->Invalidate( SID_BASICIDE_STAT_POS );
            pBindings->Invalidate( SID_ATTR_INSERT );
            pBindings->Invalidate( SID_ATTR_SIZE );
        }
	}
}

void BasicIDEShell::EnableScrollbars( BOOL bEnable )
{
	if ( bEnable )
	{
		aHScrollBar.Enable();
		aVScrollBar.Enable();
	}
	else
	{
		aHScrollBar.Disable();
		aVScrollBar.Disable();
	}
}

void BasicIDEShell::SetCurLib( const ScriptDocument& rDocument, String aLibName, bool bUpdateWindows, bool bCheck )
{
    if ( !bCheck || ( rDocument != m_aCurDocument || aLibName != m_aCurLibName ) )
    {
        m_aCurDocument = rDocument;
        m_aCurLibName = aLibName;
        if ( bUpdateWindows )
            UpdateWindows();

		SetMDITitle();

		SetCurLibForLocalization( rDocument, aLibName );

        SfxBindings* pBindings = BasicIDE::GetBindingsPtr();
        if ( pBindings )
		{
            pBindings->Invalidate( SID_BASICIDE_LIBSELECTOR );
			pBindings->Invalidate( SID_BASICIDE_CURRENT_LANG );
			pBindings->Invalidate( SID_BASICIDE_MANAGE_LANG );
		}
    }
}

void BasicIDEShell::SetCurLibForLocalization( const ScriptDocument& rDocument, String aLibName )
{
    // Create LocalizationMgr
	delete m_pCurLocalizationMgr;
	Reference< resource::XStringResourceManager > xStringResourceManager;
	try
	{
		if( aLibName.Len() )
		{
			Reference< container::XNameContainer > xDialogLib( rDocument.getLibrary( E_DIALOGS, aLibName, TRUE ) );
			xStringResourceManager = LocalizationMgr::getStringResourceFromDialogLibrary( xDialogLib );
	    }
	}
    catch ( container::NoSuchElementException& )
	{}
	m_pCurLocalizationMgr = new LocalizationMgr
		( this, rDocument, aLibName, xStringResourceManager );

	m_pCurLocalizationMgr->handleTranslationbar();
}

void BasicIDEShell::ImplStartListening( StarBASIC* pBasic )
{
	StartListening( pBasic->GetBroadcaster(), TRUE /* Nur einmal anmelden */ );
}


