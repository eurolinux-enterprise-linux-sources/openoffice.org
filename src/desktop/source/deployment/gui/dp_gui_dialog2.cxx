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
#include "precompiled_desktop.hxx"

#include "dp_gui.hrc"
#include "svtools/controldims.hrc"
#include "svtools/svtools.hrc"

#include "dp_gui.h"
#include "dp_gui_dialog2.hxx"
#include "dp_gui_extlistbox.hxx"
#include "dp_gui_shared.hxx"
#include "dp_gui_theextmgr.hxx"
#include "dp_misc.h"

#include "vcl/ctrl.hxx"
#include "vcl/menu.hxx"
#include "vcl/msgbox.hxx"
#include "vcl/scrbar.hxx"
#include "vcl/svapp.hxx"

#include "vos/mutex.hxx"

#include "svtools/extensionlistbox.hxx"

#include "sfx2/sfxdlg.hxx"

#include "comphelper/anytostring.hxx"
#include "cppuhelper/exc_hlp.hxx"
#include "cppuhelper/bootstrap.hxx"

#include "comphelper/processfactory.hxx"
#include "ucbhelper/content.hxx"
#include "unotools/collatorwrapper.hxx"

#include "com/sun/star/beans/StringPair.hpp"

#include "com/sun/star/i18n/CollatorOptions.hpp"

#include "com/sun/star/system/SystemShellExecuteFlags.hpp"
#include "com/sun/star/system/XSystemShellExecute.hpp"

#include "com/sun/star/ui/dialogs/ExecutableDialogResults.hpp"
#include "com/sun/star/ui/dialogs/TemplateDescription.hpp"
#include "com/sun/star/ui/dialogs/XFilePicker.hpp"
#include "com/sun/star/ui/dialogs/XFilterManager.hpp"

#include "com/sun/star/uno/Any.hxx"
#include "com/sun/star/uno/XComponentContext.hpp"

#include "com/sun/star/deployment/thePackageManagerFactory.hpp"

#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>

#define OUSTR(x) ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(x) )

using namespace ::com::sun::star;
using namespace ::com::sun::star::system;

using ::rtl::OUString;


namespace dp_gui {

#define TOP_OFFSET           5
#define LINE_SIZE            4
#define PROGRESS_WIDTH      60
#define PROGRESS_HEIGHT     14

//------------------------------------------------------------------------------
struct StrAllFiles : public rtl::StaticWithInit< const OUString, StrAllFiles >
{
    const OUString operator () () {
        const ::vos::OGuard guard( Application::GetSolarMutex() );
        ::std::auto_ptr< ResMgr > const resmgr( ResMgr::CreateResMgr( "fps_office" ) );
        OSL_ASSERT( resmgr.get() != 0 );
        String ret( ResId( STR_FILTERNAME_ALL, *resmgr.get() ) );
        return ret;
    }
};

//------------------------------------------------------------------------------
UpdateListEntry::UpdateListEntry( const uno::Reference< deployment::XPackage > &xPackage,
                                  const uno::Reference< deployment::XPackageManager > &xPackageManager ) :
    m_xPackage( xPackage ),
    m_xPackageManager( xPackageManager )
{}

//------------------------------------------------------------------------------
UpdateListEntry::~UpdateListEntry()
{}

//------------------------------------------------------------------------------
//                            ExtBoxWithBtns_Impl
//------------------------------------------------------------------------------

enum MENU_COMMAND
{
    CMD_NONE    = 0,
    CMD_REMOVE  = 1,
    CMD_ENABLE,
    CMD_DISABLE,
    CMD_UPDATE
};

class ExtBoxWithBtns_Impl : public ExtensionBox_Impl
{
    Size            m_aOutputSize;
    bool            m_bInterfaceLocked;
    
    PushButton     *m_pOptionsBtn;
    PushButton     *m_pEnableBtn;
    PushButton     *m_pRemoveBtn;

    ExtMgrDialog   *m_pParent;

    void            SetButtonPos( const Rectangle& rRect );
    void            SetButtonStatus( const TEntry_Impl pEntry );
    bool            HandleTabKey( bool bReverse );
    MENU_COMMAND    ShowPopupMenu( const Point &rPos, const long nPos );

    //-----------------
    DECL_DLLPRIVATE_LINK( ScrollHdl, ScrollBar * );

    DECL_DLLPRIVATE_LINK( HandleOptionsBtn, void * );
    DECL_DLLPRIVATE_LINK( HandleEnableBtn, void * );
    DECL_DLLPRIVATE_LINK( HandleRemoveBtn, void * );
    DECL_DLLPRIVATE_LINK( HandleHyperlink, svt::FixedHyperlink * );

public:
                    ExtBoxWithBtns_Impl( ExtMgrDialog* pParent, TheExtensionManager *pManager );
                   ~ExtBoxWithBtns_Impl();

    virtual void    MouseButtonDown( const MouseEvent& rMEvt );
    virtual long    Notify( NotifyEvent& rNEvt );

    const Size      GetMinOutputSizePixel() const;

    virtual void    RecalcAll();
    virtual void    selectEntry( const long nPos );
    //-----------------
    void            enableButtons( bool bEnable );
};

//------------------------------------------------------------------------------
ExtBoxWithBtns_Impl::ExtBoxWithBtns_Impl( ExtMgrDialog* pParent, TheExtensionManager *pManager ) :
    ExtensionBox_Impl( pParent, pManager ),
    m_bInterfaceLocked( false ),
    m_pOptionsBtn( NULL ),
    m_pEnableBtn( NULL ),
    m_pRemoveBtn( NULL ),
    m_pParent( pParent )
{
    m_pOptionsBtn = new PushButton( this, WB_TABSTOP );
    m_pEnableBtn = new PushButton( this, WB_TABSTOP );
    m_pRemoveBtn = new PushButton( this, WB_TABSTOP );

    SetHelpId( HID_EXTENSION_MANAGER_LISTBOX );
    m_pOptionsBtn->SetHelpId( HID_EXTENSION_MANAGER_LISTBOX_OPTIONS );
    m_pEnableBtn->SetHelpId( HID_EXTENSION_MANAGER_LISTBOX_DISABLE );
    m_pRemoveBtn->SetHelpId( HID_EXTENSION_MANAGER_LISTBOX_REMOVE );

    m_pOptionsBtn->SetClickHdl( LINK( this, ExtBoxWithBtns_Impl, HandleOptionsBtn ) );
    m_pEnableBtn->SetClickHdl( LINK( this, ExtBoxWithBtns_Impl, HandleEnableBtn ) );
    m_pRemoveBtn->SetClickHdl( LINK( this, ExtBoxWithBtns_Impl, HandleRemoveBtn ) );

    m_pOptionsBtn->SetText( DialogHelper::getResourceString( RID_CTX_ITEM_OPTIONS ) );
    m_pEnableBtn->SetText( DialogHelper::getResourceString( RID_CTX_ITEM_DISABLE ) );
    m_pRemoveBtn->SetText( DialogHelper::getResourceString( RID_CTX_ITEM_REMOVE ) );

    Size aSize = LogicToPixel( Size( RSC_CD_PUSHBUTTON_WIDTH, RSC_CD_PUSHBUTTON_HEIGHT ),
                               MapMode( MAP_APPFONT ) );
    m_pOptionsBtn->SetSizePixel( aSize );
    m_pEnableBtn->SetSizePixel( aSize );
    m_pRemoveBtn->SetSizePixel( aSize );

    SetExtraSize( aSize.Height() + 2 * TOP_OFFSET );

    SetScrollHdl( LINK( this, ExtBoxWithBtns_Impl, ScrollHdl ) );
}

//------------------------------------------------------------------------------
ExtBoxWithBtns_Impl::~ExtBoxWithBtns_Impl()
{
    delete m_pOptionsBtn;
    delete m_pEnableBtn;
    delete m_pRemoveBtn;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const Size ExtBoxWithBtns_Impl::GetMinOutputSizePixel() const
{
    Size aMinSize( ExtensionBox_Impl::GetMinOutputSizePixel() );
    long nHeight = aMinSize.Height();
    nHeight += m_pOptionsBtn->GetSizePixel().Height();
    nHeight +=  2 * TOP_OFFSET;
    long nWidth = m_pOptionsBtn->GetSizePixel().Width();
    nWidth *= 3;
    nWidth += 5*TOP_OFFSET + 20;

    return Size( nWidth, nHeight );
}

// -----------------------------------------------------------------------
void ExtBoxWithBtns_Impl::RecalcAll()
{
    ExtensionBox_Impl::RecalcAll();

    const sal_Int32 nActive = getSelIndex();

    if ( nActive != EXTENSION_LISTBOX_ENTRY_NOTFOUND )
        SetButtonPos( GetEntryRect( nActive ) );
    else
    {
        m_pOptionsBtn->Hide();
        m_pEnableBtn->Hide();
        m_pRemoveBtn->Hide();
    }
}


//------------------------------------------------------------------------------
//This function may be called with nPos < 0
void ExtBoxWithBtns_Impl::selectEntry( const long nPos )
{
    if ( HasActive() && ( nPos == getSelIndex() ) )
        return;

    ExtensionBox_Impl::selectEntry( nPos );

    if ( ( nPos >= 0 ) && ( nPos < GetEntryCount() ) )
    {
        if ( IsReallyVisible() )
        {
            SetButtonPos( GetEntryRect( nPos ) );
        }
        SetButtonStatus( GetEntryData( nPos) );
    }
    else
    {
        m_pOptionsBtn->Hide();
        m_pEnableBtn->Hide();
        m_pRemoveBtn->Hide();
    }
}

// -----------------------------------------------------------------------
void ExtBoxWithBtns_Impl::SetButtonPos( const Rectangle& rRect )
{
    Size  aBtnSize( m_pOptionsBtn->GetSizePixel() );
    Point aBtnPos( rRect.Left() + ICON_OFFSET,
                   rRect.Bottom() - TOP_OFFSET - aBtnSize.Height() );

    m_pOptionsBtn->SetPosPixel( aBtnPos );
    aBtnPos.X() = rRect.Right() - TOP_OFFSET - aBtnSize.Width();
    m_pRemoveBtn->SetPosPixel( aBtnPos );
    aBtnPos.X() -= ( TOP_OFFSET + aBtnSize.Width() );
    m_pEnableBtn->SetPosPixel( aBtnPos );
}

// -----------------------------------------------------------------------
void ExtBoxWithBtns_Impl::SetButtonStatus( const TEntry_Impl pEntry )
{
    if ( ( pEntry->m_eState == REGISTERED ) || ( pEntry->m_eState == NOT_AVAILABLE ) )
    {
        m_pEnableBtn->SetText( DialogHelper::getResourceString( RID_CTX_ITEM_DISABLE ) );
        m_pEnableBtn->SetHelpId( HID_EXTENSION_MANAGER_LISTBOX_DISABLE );
    }
    else
    {
        m_pEnableBtn->SetText( DialogHelper::getResourceString( RID_CTX_ITEM_ENABLE ) );
        m_pEnableBtn->SetHelpId( HID_EXTENSION_MANAGER_LISTBOX_ENABLE );
    }

    if ( ( pEntry->m_eState == NOT_AVAILABLE ) || pEntry->m_bMissingDeps )
        m_pEnableBtn->Hide();
    else
    {
        m_pEnableBtn->Enable( !pEntry->m_bLocked );
        m_pEnableBtn->Show();
    }

    if ( pEntry->m_bHasOptions )
    {
        m_pOptionsBtn->Enable( pEntry->m_bHasOptions );
        m_pOptionsBtn->Show();
    }
    else
        m_pOptionsBtn->Hide();

    m_pRemoveBtn->Show();
    m_pRemoveBtn->Enable( !pEntry->m_bLocked );
}

// -----------------------------------------------------------------------
bool ExtBoxWithBtns_Impl::HandleTabKey( bool bReverse )
{
    sal_Int32 nIndex = getSelIndex();

    if ( nIndex == EXTENSION_LISTBOX_ENTRY_NOTFOUND )
        return false;

    PushButton *pNext = NULL;

    if ( m_pOptionsBtn->HasFocus() ) {
        if ( !bReverse && !GetEntryData( nIndex )->m_bLocked )
            pNext = m_pEnableBtn;
    }
    else if ( m_pEnableBtn->HasFocus() ) {
        if ( !bReverse )
            pNext = m_pRemoveBtn;
        else if ( GetEntryData( nIndex )->m_bHasOptions )
            pNext = m_pOptionsBtn;
    }
    else if ( m_pRemoveBtn->HasFocus() ) {
        if ( bReverse )
            pNext = m_pEnableBtn;
    }
    else {
        if ( !bReverse ) {
            if ( GetEntryData( nIndex )->m_bHasOptions )
                pNext = m_pOptionsBtn;
            else if ( ! GetEntryData( nIndex )->m_bLocked )
                pNext = m_pEnableBtn;
        } else {
            if ( ! GetEntryData( nIndex )->m_bLocked )
                pNext = m_pRemoveBtn;
            else if ( GetEntryData( nIndex )->m_bHasOptions )
                pNext = m_pOptionsBtn;
        }
    }

    if ( pNext )
    {
        pNext->GrabFocus();
        return true;
    }
    else
        return false;
}

// -----------------------------------------------------------------------
MENU_COMMAND ExtBoxWithBtns_Impl::ShowPopupMenu( const Point & rPos, const long nPos )
{
    if ( nPos >= (long) getItemCount() )
        return CMD_NONE;

    PopupMenu aPopup;

    aPopup.InsertItem( CMD_UPDATE, DialogHelper::getResourceString( RID_CTX_ITEM_CHECK_UPDATE ) );

    if ( ! GetEntryData( nPos )->m_bLocked )
    {
        if ( GetEntryData( nPos )->m_eState == REGISTERED )
            aPopup.InsertItem( CMD_DISABLE, DialogHelper::getResourceString( RID_CTX_ITEM_DISABLE ) );
        else if ( GetEntryData( nPos )->m_eState != NOT_AVAILABLE )
            aPopup.InsertItem( CMD_ENABLE, DialogHelper::getResourceString( RID_CTX_ITEM_ENABLE ) );
    
        aPopup.InsertItem( CMD_REMOVE, DialogHelper::getResourceString( RID_CTX_ITEM_REMOVE ) );
    }

    return (MENU_COMMAND) aPopup.Execute( this, rPos );
}

//------------------------------------------------------------------------------
void ExtBoxWithBtns_Impl::MouseButtonDown( const MouseEvent& rMEvt )
{
    if ( m_bInterfaceLocked )
        return;

    const Point aMousePos( rMEvt.GetPosPixel() );
    const long nPos = PointToPos( aMousePos );

    if ( rMEvt.IsRight() )
    {
        switch( ShowPopupMenu( aMousePos, nPos ) )
        {
            case CMD_NONE:      break;
            case CMD_ENABLE:    m_pParent->enablePackage( GetEntryData( nPos )->m_xPackageManager,
                                                          GetEntryData( nPos )->m_xPackage, true );
                                break;
            case CMD_DISABLE:   m_pParent->enablePackage( GetEntryData( nPos )->m_xPackageManager,
                                                          GetEntryData( nPos )->m_xPackage, false );
                                break;
            case CMD_UPDATE:    m_pParent->updatePackage( GetEntryData( nPos )->m_xPackageManager,
                                                          GetEntryData( nPos )->m_xPackage );
                                break;
            case CMD_REMOVE:    m_pParent->removePackage( GetEntryData( nPos )->m_xPackageManager,
                                                          GetEntryData( nPos )->m_xPackage );
                                break;
        }
    }
    else if ( rMEvt.IsLeft() )
    {
        if ( rMEvt.IsMod1() && HasActive() )
            selectEntry( EXTENSION_LISTBOX_ENTRY_NOTFOUND );   // Selecting an not existing entry will deselect the current one
        else
            selectEntry( nPos );
    }
}

//------------------------------------------------------------------------------
long ExtBoxWithBtns_Impl::Notify( NotifyEvent& rNEvt )
{
    bool bHandled = false;

    if ( rNEvt.GetType() == EVENT_KEYINPUT )
    {
        const KeyEvent* pKEvt = rNEvt.GetKeyEvent();
        KeyCode         aKeyCode = pKEvt->GetKeyCode();
        USHORT          nKeyCode = aKeyCode.GetCode();

        if ( nKeyCode == KEY_TAB )
            bHandled = HandleTabKey( aKeyCode.IsShift() );
    }

    if ( !bHandled )
        return ExtensionBox_Impl::Notify( rNEvt );
    else
        return true;
}

//------------------------------------------------------------------------------
void ExtBoxWithBtns_Impl::enableButtons( bool bEnable )
{
    m_bInterfaceLocked = ! bEnable;

    if ( bEnable )
    {
        sal_Int32 nIndex = getSelIndex();
        if ( nIndex != EXTENSION_LISTBOX_ENTRY_NOTFOUND )
            SetButtonStatus( GetEntryData( nIndex ) );
    }
    else
    {
        m_pOptionsBtn->Enable( false );
        m_pRemoveBtn->Enable( false );
        m_pEnableBtn->Enable( false );
    }
}

// -----------------------------------------------------------------------
IMPL_LINK( ExtBoxWithBtns_Impl, ScrollHdl, ScrollBar*, pScrBar )
{
    long nDelta = pScrBar->GetDelta();
    
    Point aNewOptPt( m_pOptionsBtn->GetPosPixel() - Point( 0, nDelta ) );
    Point aNewRemPt( m_pRemoveBtn->GetPosPixel() - Point( 0, nDelta ) );
    Point aNewEnPt( m_pEnableBtn->GetPosPixel() - Point( 0, nDelta ) );

    DoScroll( nDelta );
    
    m_pOptionsBtn->SetPosPixel( aNewOptPt );
    m_pRemoveBtn->SetPosPixel( aNewRemPt );
    m_pEnableBtn->SetPosPixel( aNewEnPt );

    return 1;
}

// -----------------------------------------------------------------------
IMPL_LINK( ExtBoxWithBtns_Impl, HandleOptionsBtn, void*, EMPTYARG )
{
    const sal_Int32 nActive = getSelIndex();

    if ( nActive != EXTENSION_LISTBOX_ENTRY_NOTFOUND )
    {
        SfxAbstractDialogFactory* pFact = SfxAbstractDialogFactory::Create();
    
        if ( pFact )
        {
            OUString sExtensionId = GetEntryData( nActive )->m_xPackage->getIdentifier().Value;
            VclAbstractDialog* pDlg = pFact->CreateOptionsDialog( this, sExtensionId, rtl::OUString() );
    
            pDlg->Execute();
    
            delete pDlg;
        }
    }

    return 1;
}

// -----------------------------------------------------------------------
IMPL_LINK( ExtBoxWithBtns_Impl, HandleEnableBtn, void*, EMPTYARG )
{
    const sal_Int32 nActive = getSelIndex();

    if ( nActive != EXTENSION_LISTBOX_ENTRY_NOTFOUND )
    {
        TEntry_Impl pEntry = GetEntryData( nActive );
        const bool bEnable( pEntry->m_eState != REGISTERED );

        m_pParent->enablePackage( pEntry->m_xPackageManager,
                                  pEntry->m_xPackage,
                                  bEnable );
    }

    return 1;
}

// -----------------------------------------------------------------------
IMPL_LINK( ExtBoxWithBtns_Impl, HandleRemoveBtn, void*, EMPTYARG )
{
    const sal_Int32 nActive = getSelIndex();

    if ( nActive != EXTENSION_LISTBOX_ENTRY_NOTFOUND )
    {
        TEntry_Impl pEntry = GetEntryData( nActive );
        m_pParent->removePackage( pEntry->m_xPackageManager,
                                  pEntry->m_xPackage );
    }

    return 1;
}

//------------------------------------------------------------------------------
//                             DialogHelper
//------------------------------------------------------------------------------
DialogHelper::DialogHelper( const uno::Reference< uno::XComponentContext > &xContext,
                            Dialog *pWindow ) :
    m_pVCLWindow( pWindow ),
    m_nEventID(   0 ),
    m_bIsBusy(    false )
{
    m_xContext = xContext;
}

//------------------------------------------------------------------------------
DialogHelper::~DialogHelper()
{
    if ( m_nEventID )
        Application::RemoveUserEvent( m_nEventID );
}

//------------------------------------------------------------------------------
ResId DialogHelper::getResId( USHORT nId )
{
    const ::vos::OGuard guard( Application::GetSolarMutex() );
    return ResId( nId, *DeploymentGuiResMgr::get() );
}

//------------------------------------------------------------------------------
String DialogHelper::getResourceString( USHORT id )
{
    // init with non-acquired solar mutex:
    BrandName::get();
    const ::vos::OGuard guard( Application::GetSolarMutex() );
    String ret( ResId( id, *DeploymentGuiResMgr::get() ) );
    if (ret.SearchAscii( "%PRODUCTNAME" ) != STRING_NOTFOUND) {
        ret.SearchAndReplaceAllAscii( "%PRODUCTNAME", BrandName::get() );
    }
    return ret;
}

//------------------------------------------------------------------------------
bool DialogHelper::IsSharedPkgMgr( const uno::Reference< deployment::XPackageManager > &xPackageManager )
{
    if ( xPackageManager->getContext().equals( OUSTR("shared") ) )
        return true;
    else
        return false;
}

//------------------------------------------------------------------------------
bool DialogHelper::continueOnSharedExtension( const uno::Reference< deployment::XPackageManager > &xPackageManager,
                                              Window *pParent,
                                              const USHORT nResID,
                                              bool &bHadWarning )
{
    if ( !bHadWarning && IsSharedPkgMgr( xPackageManager ) )
    {
        const ::vos::OGuard guard( Application::GetSolarMutex() );
        WarningBox aInfoBox( pParent, getResId( nResID ) );
        String aMsgText = aInfoBox.GetMessText();
        aMsgText.SearchAndReplaceAllAscii( "%PRODUCTNAME", BrandName::get() );
        aInfoBox.SetMessText( aMsgText );

        bHadWarning = true;
        
        if ( RET_OK == aInfoBox.Execute() )
            return true;
        else
            return false;
    }
    else
        return true;
}

//------------------------------------------------------------------------------
void DialogHelper::openWebBrowser( const OUString & sURL, const OUString &sTitle ) const
{
    if ( ! sURL.getLength() ) // Nothing to do, when the URL is empty
        return;

    try 
    {
        uno::Reference< XSystemShellExecute > xSystemShellExecute(
            m_xContext->getServiceManager()->createInstanceWithContext( OUSTR( "com.sun.star.system.SystemShellExecute" ), m_xContext), uno::UNO_QUERY_THROW);
        //throws css::lang::IllegalArgumentException, css::system::SystemShellExecuteException
        xSystemShellExecute->execute( sURL, OUString(),  SystemShellExecuteFlags::DEFAULTS );
    }
    catch ( uno::Exception& )
    {
        uno::Any exc( ::cppu::getCaughtException() );
        OUString msg( ::comphelper::anyToString( exc ) );
        const ::vos::OGuard guard( Application::GetSolarMutex() );
        ErrorBox aErrorBox( NULL, WB_OK, msg );
        aErrorBox.SetText( sTitle );
        aErrorBox.Execute();
    }
}

//------------------------------------------------------------------------------
bool DialogHelper::installExtensionWarn( const OUString &rExtensionName ) const
{
    const ::vos::OGuard guard( Application::GetSolarMutex() );
    WarningBox aInfo( m_pVCLWindow, getResId( RID_WARNINGBOX_INSTALL_EXTENSION ) );

    String sText( aInfo.GetMessText() );
    sText.SearchAndReplaceAllAscii( "%NAME", rExtensionName );
    aInfo.SetMessText( sText );

    return ( RET_OK == aInfo.Execute() );
}

//------------------------------------------------------------------------------
bool DialogHelper::installForAllUsers( bool &bInstallForAll ) const
{
    const ::vos::OGuard guard( Application::GetSolarMutex() );
    QueryBox aQuery( m_pVCLWindow, getResId( RID_QUERYBOX_INSTALL_FOR_ALL ) );

    String sMsgText = aQuery.GetMessText();
    sMsgText.SearchAndReplaceAllAscii( "%PRODUCTNAME", BrandName::get() );
    aQuery.SetMessText( sMsgText );

    USHORT nYesBtnID = aQuery.GetButtonId( 0 );
    USHORT nNoBtnID = aQuery.GetButtonId( 1 );

    if ( nYesBtnID != BUTTONDIALOG_BUTTON_NOTFOUND )
        aQuery.SetButtonText( nYesBtnID, getResourceString( RID_STR_INSTALL_FOR_ME ) );
    if ( nNoBtnID != BUTTONDIALOG_BUTTON_NOTFOUND )
        aQuery.SetButtonText( nNoBtnID, getResourceString( RID_STR_INSTALL_FOR_ALL ) );

    short nRet = aQuery.Execute();

    if ( nRet == RET_CANCEL )
        return false;

    bInstallForAll = ( nRet == RET_NO );
    return true;
}

//------------------------------------------------------------------------------
void DialogHelper::PostUserEvent( const Link& rLink, void* pCaller )
{
    if ( m_nEventID )
        Application::RemoveUserEvent( m_nEventID );

    m_nEventID = Application::PostUserEvent( rLink, pCaller );
}

//------------------------------------------------------------------------------
//                             ExtMgrDialog
//------------------------------------------------------------------------------
ExtMgrDialog::ExtMgrDialog( Window *pParent, TheExtensionManager *pManager ) :
    ModelessDialog( pParent, getResId( RID_DLG_EXTENSION_MANAGER ) ),
    DialogHelper( pManager->getContext(), (Dialog*) this ),
    m_aAddBtn( this,        getResId( RID_EM_BTN_ADD ) ),
    m_aUpdateBtn( this,     getResId( RID_EM_BTN_CHECK_UPDATES ) ),
    m_aCloseBtn( this,      getResId( RID_EM_BTN_CLOSE ) ),
    m_aHelpBtn( this,       getResId( RID_EM_BTN_HELP ) ),
    m_aDivider( this ),
    m_aGetExtensions( this, getResId( RID_EM_FT_GET_EXTENSIONS ) ),
    m_aProgressText( this,  getResId( RID_EM_FT_PROGRESS ) ),
    m_aProgressBar( this,   WB_BORDER + WB_3DLOOK ),
    m_aCancelBtn( this,     getResId( RID_EM_BTN_CANCEL ) ),
    m_sAddPackages(         getResourceString( RID_STR_ADD_PACKAGES ) ),
    m_bHasProgress(         false ),
    m_bProgressChanged(     false ),
    m_bStartProgress(       false ),
    m_bStopProgress(        false ),
    m_bUpdateWarning(       false ),
    m_bEnableWarning(       false ),
    m_bDisableWarning(      false ),
    m_bDeleteWarning(       false ),
    m_nProgress(            0 ),
    m_pManager( pManager )
{
    // free local resources (RID < 256):
    FreeResource();

    m_pExtensionBox = new ExtBoxWithBtns_Impl( this, pManager );
    m_pExtensionBox->SetHyperlinkHdl( LINK( this, ExtMgrDialog, HandleHyperlink ) );

    m_aAddBtn.SetClickHdl( LINK( this, ExtMgrDialog, HandleAddBtn ) );
    m_aUpdateBtn.SetClickHdl( LINK( this, ExtMgrDialog, HandleUpdateBtn ) );
    m_aGetExtensions.SetClickHdl( LINK( this, ExtMgrDialog, HandleHyperlink ) );
    m_aCancelBtn.SetClickHdl( LINK( this, ExtMgrDialog, HandleCancelBtn ) );

    // resize update button
    Size aBtnSize = m_aUpdateBtn.GetSizePixel();
    String sTitle = m_aUpdateBtn.GetText();
    long nWidth = m_aUpdateBtn.GetCtrlTextWidth( sTitle );
    nWidth += 2 * m_aUpdateBtn.GetTextHeight();
    if ( nWidth > aBtnSize.Width() )
        m_aUpdateBtn.SetSizePixel( Size( nWidth, aBtnSize.Height() ) );

    // minimum size:
    SetMinOutputSizePixel(
        Size( // width:
              (3 * m_aHelpBtn.GetSizePixel().Width()) +
                   m_aUpdateBtn.GetSizePixel().Width() +
              (5 * RSC_SP_DLG_INNERBORDER_LEFT ),
              // height:
              (1 * m_aHelpBtn.GetSizePixel().Height()) +
              (1 * m_aGetExtensions.GetSizePixel().Height()) +
              (1 * m_pExtensionBox->GetMinOutputSizePixel().Height()) +
              (3 * RSC_SP_DLG_INNERBORDER_LEFT) ) );

    m_aDivider.Show();
    m_aProgressBar.Hide();

    m_aUpdateBtn.Enable( false );

    m_aTimeoutTimer.SetTimeout( 500 ); // mSec
    m_aTimeoutTimer.SetTimeoutHdl( LINK( this, ExtMgrDialog, TimeOutHdl ) );
}

//------------------------------------------------------------------------------
ExtMgrDialog::~ExtMgrDialog()
{
    m_aTimeoutTimer.Stop();
    delete m_pExtensionBox;
}

//------------------------------------------------------------------------------
void ExtMgrDialog::selectEntry( long nPos )
{
    m_pExtensionBox->selectEntry( nPos );
}

//------------------------------------------------------------------------------
void ExtMgrDialog::setGetExtensionsURL( const ::rtl::OUString &rURL )
{
    m_aGetExtensions.SetURL( rURL );
}

//------------------------------------------------------------------------------
long ExtMgrDialog::addPackageToList( const uno::Reference< deployment::XPackage > &xPackage,
                                     const uno::Reference< deployment::XPackageManager > &xPackageManager )
{
    m_aUpdateBtn.Enable( true );
    return m_pExtensionBox->addEntry( xPackage, xPackageManager );
}

//------------------------------------------------------------------------------
void ExtMgrDialog::prepareChecking( const uno::Reference< deployment::XPackageManager > &xPackageManager )
{
    if ( xPackageManager.is() )
        m_pExtensionBox->prepareChecking( xPackageManager );
}

//------------------------------------------------------------------------------
void ExtMgrDialog::checkEntries()
{
    const ::vos::OGuard guard( Application::GetSolarMutex() );
    m_pExtensionBox->checkEntries();
}

//------------------------------------------------------------------------------
bool ExtMgrDialog::removeExtensionWarn( const OUString &rExtensionName ) const
{
    const ::vos::OGuard guard( Application::GetSolarMutex() );
    WarningBox aInfo( const_cast< ExtMgrDialog* >(this), getResId( RID_WARNINGBOX_REMOVE_EXTENSION ) );

    String sText( aInfo.GetMessText() );
    sText.SearchAndReplaceAllAscii( "%NAME", rExtensionName );
    aInfo.SetMessText( sText );

    return ( RET_OK == aInfo.Execute() );
}

//------------------------------------------------------------------------------
bool ExtMgrDialog::enablePackage( const uno::Reference< deployment::XPackageManager > &xPackageManager,
                                  const uno::Reference< deployment::XPackage > &xPackage,
                                  bool bEnable )
{
    if ( !xPackageManager.is() || !xPackage.is() )
        return false;

    if ( bEnable )
    {
        if ( ! continueOnSharedExtension( xPackageManager, this, RID_WARNINGBOX_ENABLE_SHARED_EXTENSION, m_bEnableWarning ) )
            return false;
    }
    else
    {
        if ( ! continueOnSharedExtension( xPackageManager, this, RID_WARNINGBOX_DISABLE_SHARED_EXTENSION, m_bDisableWarning ) )
            return false;
    }

    m_pManager->enablePackage( xPackage, bEnable );

    return true;
}

//------------------------------------------------------------------------------
bool ExtMgrDialog::removePackage( const uno::Reference< deployment::XPackageManager > &xPackageManager,
                                  const uno::Reference< deployment::XPackage > &xPackage )
{
    if ( !xPackageManager.is() || !xPackage.is() )
        return false;

    if ( !IsSharedPkgMgr( xPackageManager ) || m_bDeleteWarning )
    {
        if ( ! removeExtensionWarn( xPackage->getDisplayName() ) )
            return false;
    }

    if ( ! continueOnSharedExtension( xPackageManager, this, RID_WARNINGBOX_REMOVE_SHARED_EXTENSION, m_bDeleteWarning ) )
        return false;

    m_pManager->removePackage( xPackageManager, xPackage );

    return true;
}

//------------------------------------------------------------------------------
bool ExtMgrDialog::updatePackage( const uno::Reference< deployment::XPackageManager > &xPackageManager,
                                  const uno::Reference< deployment::XPackage > &xPackage )
{
    if ( !xPackageManager.is() || !xPackage.is() )
        return false;

    std::vector< TUpdateListEntry > vEntries;
    TUpdateListEntry pEntry( new UpdateListEntry( xPackage, xPackageManager ) );
    vEntries.push_back( pEntry );

    m_pManager->updatePackages( vEntries );

    return true;
}

//------------------------------------------------------------------------------
uno::Sequence< OUString > ExtMgrDialog::raiseAddPicker( const uno::Reference< deployment::XPackageManager > &xPackageManager )
{
    const uno::Any mode( static_cast< sal_Int16 >( ui::dialogs::TemplateDescription::FILEOPEN_SIMPLE ) );
    const uno::Reference< uno::XComponentContext > xContext( m_pManager->getContext() );
    const uno::Reference< ui::dialogs::XFilePicker > xFilePicker(
        xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
            OUSTR("com.sun.star.ui.dialogs.FilePicker"),
            uno::Sequence< uno::Any >( &mode, 1 ), xContext ), uno::UNO_QUERY_THROW );
    xFilePicker->setTitle( m_sAddPackages );

    if ( m_sLastFolderURL.Len() )
        xFilePicker->setDisplayDirectory( m_sLastFolderURL );

    // collect and set filter list:
    typedef ::std::map< OUString, OUString > t_string2string;
    t_string2string title2filter;
    OUString sDefaultFilter( StrAllFiles::get() );

    const uno::Sequence< uno::Reference< deployment::XPackageTypeInfo > > packageTypes( xPackageManager->getSupportedPackageTypes() );

    for ( sal_Int32 pos = 0; pos < packageTypes.getLength(); ++pos )
    {
        uno::Reference< deployment::XPackageTypeInfo > const & xPackageType = packageTypes[ pos ];
        const OUString filter( xPackageType->getFileFilter() );
        if (filter.getLength() > 0)
        {
            const OUString title( xPackageType->getShortDescription() );
            const ::std::pair< t_string2string::iterator, bool > insertion(
                title2filter.insert( t_string2string::value_type( title, filter ) ) );
            if ( ! insertion.second ) 
            { // already existing, append extensions:
                ::rtl::OUStringBuffer buf;
                buf.append( insertion.first->second );
                buf.append( static_cast<sal_Unicode>(';') );
                buf.append( filter );
                insertion.first->second = buf.makeStringAndClear();
            }
            if ( xPackageType->getMediaType() == OUSTR( "application/vnd.sun.star.package-bundle" ) )
                sDefaultFilter = title;
        }
    }

    const uno::Reference< ui::dialogs::XFilterManager > xFilterManager( xFilePicker, uno::UNO_QUERY_THROW );
    // All files at top:
    xFilterManager->appendFilter( StrAllFiles::get(), OUSTR("*.*") );
    // then supported ones:
    t_string2string::const_iterator iPos( title2filter.begin() );
    const t_string2string::const_iterator iEnd( title2filter.end() );
    for ( ; iPos != iEnd; ++iPos ) {
        try {
            xFilterManager->appendFilter( iPos->first, iPos->second );
        }
        catch (lang::IllegalArgumentException & exc) {
            OSL_ENSURE( 0, ::rtl::OUStringToOString(
                            exc.Message, RTL_TEXTENCODING_UTF8 ).getStr() );
            (void) exc;
        }
    }
    xFilterManager->setCurrentFilter( sDefaultFilter );

    if ( xFilePicker->execute() != ui::dialogs::ExecutableDialogResults::OK )
        return uno::Sequence<OUString>(); // cancelled

    m_sLastFolderURL = xFilePicker->getDisplayDirectory();
    uno::Sequence< OUString > files( xFilePicker->getFiles() );
    OSL_ASSERT( files.getLength() > 0 );
    return files;
}

//------------------------------------------------------------------------------
IMPL_LINK( ExtMgrDialog, HandleCancelBtn, void*, EMPTYARG )
{
    // m_dialog->m_cmdEnv->m_aborted = true;
    if ( m_xAbortChannel.is() )
    {
        try
        {
            m_xAbortChannel->sendAbort();
        }
        catch ( uno::RuntimeException & )
        {
            OSL_ENSURE( 0, "### unexpected RuntimeException!" );
        }
    }
    return 1;
}

// ------------------------------------------------------------------------------
IMPL_LINK( ExtMgrDialog, startProgress, void*, _bLockInterface )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    bool bLockInterface = (bool) _bLockInterface;

    if ( m_bStartProgress && !m_bHasProgress )
        m_aTimeoutTimer.Start();

    if ( m_bStopProgress )
    {
		if ( m_aProgressBar.IsVisible() )
	        m_aProgressBar.SetValue( 100 );
        m_xAbortChannel.clear();

        OSL_TRACE( " startProgress handler: stop\n" );
    }
    else
    {
        OSL_TRACE( " startProgress handler: start\n" );
    }

    m_aCancelBtn.Enable( bLockInterface );
    m_aAddBtn.Enable( !bLockInterface );
    m_aUpdateBtn.Enable( !bLockInterface && m_pExtensionBox->getItemCount() );
    m_pExtensionBox->enableButtons( !bLockInterface );

    clearEventID();

    return 0;
}

// ------------------------------------------------------------------------------
void ExtMgrDialog::showProgress( bool _bStart )
{
    ::osl::MutexGuard aGuard( m_aMutex );

    bool bStart = _bStart;

    if ( bStart )
    {
        m_nProgress = 0;
        m_bStartProgress = true;
        OSL_TRACE( "showProgress start\n" );
    }
    else
    {
        m_nProgress = 100;
        m_bStopProgress = true;
        OSL_TRACE( "showProgress stop!\n" );
    }

    DialogHelper::PostUserEvent( LINK( this, ExtMgrDialog, startProgress ), (void*) bStart );
}

// -----------------------------------------------------------------------
void ExtMgrDialog::updateProgress( const long nProgress )
{
    ::osl::MutexGuard aGuard( m_aMutex );

    m_nProgress = nProgress;
}

// -----------------------------------------------------------------------
void ExtMgrDialog::updateProgress( const OUString &rText,
                                   const uno::Reference< task::XAbortChannel > &xAbortChannel)
{
    ::osl::MutexGuard aGuard( m_aMutex );

    m_xAbortChannel = xAbortChannel;
    m_sProgressText = rText;
    m_bProgressChanged = true;
}

//------------------------------------------------------------------------------
void ExtMgrDialog::updatePackageInfo( const uno::Reference< deployment::XPackage > &xPackage )
{
    m_pExtensionBox->updateEntry( xPackage );
}

// -----------------------------------------------------------------------
IMPL_LINK( ExtMgrDialog, HandleAddBtn, void*, EMPTYARG )
{
    setBusy( true );

    uno::Reference< deployment::XPackageManager > xUserPkgMgr = m_pManager->getUserPkgMgr();
    uno::Sequence< OUString > aFileList = raiseAddPicker( xUserPkgMgr );

    if ( aFileList.getLength() )
    {
        m_pManager->installPackage( aFileList[0] );
    }

    setBusy( false );
    return 1;
}

// -----------------------------------------------------------------------
IMPL_LINK( ExtMgrDialog, HandleUpdateBtn, void*, EMPTYARG )
{
    m_pManager->checkUpdates( false, true );

    return 1;
}

// -----------------------------------------------------------------------
IMPL_LINK( ExtMgrDialog, HandleHyperlink, svt::FixedHyperlink*, pHyperlink )
{
    openWebBrowser( pHyperlink->GetURL(), GetText() );

    return 1;
}

// -----------------------------------------------------------------------
IMPL_LINK( ExtMgrDialog, TimeOutHdl, Timer*, EMPTYARG )
{
    if ( m_bStopProgress )
    {
        m_bHasProgress = false;
        m_bStopProgress = false;
        m_aProgressText.Hide();
        m_aProgressBar.Hide();
        m_aCancelBtn.Hide();
    }
    else
    {
        if ( m_bProgressChanged )
        {
            m_bProgressChanged = false;
            m_aProgressText.SetText( m_sProgressText );
        }

        if ( m_bStartProgress )
        {
            m_bStartProgress = false;
            m_bHasProgress = true;
            m_aProgressBar.Show();
            m_aProgressText.Show();
            m_aCancelBtn.Enable();
            m_aCancelBtn.Show();
        }

		if ( m_aProgressBar.IsVisible() )
			m_aProgressBar.SetValue( (USHORT) m_nProgress );

        m_aTimeoutTimer.Start();
    }

	return 1;
}

//------------------------------------------------------------------------------
// VCL::Window / Dialog
void ExtMgrDialog::Resize()
{
    Size aTotalSize( GetOutputSizePixel() );
    Size aBtnSize( m_aHelpBtn.GetSizePixel() );
    Size aUpdBtnSize( m_aUpdateBtn.GetSizePixel() );

    Point aPos( RSC_SP_DLG_INNERBORDER_LEFT,
                aTotalSize.Height() - RSC_SP_DLG_INNERBORDER_BOTTOM - aBtnSize.Height() );
    
    m_aHelpBtn.SetPosPixel( aPos );

    aPos.X() = aTotalSize.Width() - RSC_SP_DLG_INNERBORDER_RIGHT - aBtnSize.Width();
    m_aCloseBtn.SetPosPixel( aPos );

    aPos.X() -= ( RSC_SP_CTRL_X + aUpdBtnSize.Width() );
    m_aUpdateBtn.SetPosPixel( aPos );

    aPos.X() -= ( RSC_SP_CTRL_GROUP_Y + aBtnSize.Width() );
    m_aAddBtn.SetPosPixel( aPos );

    Size aDivSize( aTotalSize.Width(), LINE_SIZE );
    aPos = Point( 0, aPos.Y() - LINE_SIZE - RSC_SP_DLG_INNERBORDER_BOTTOM );
    m_aDivider.SetPosSizePixel( aPos, aDivSize );

    Size aFTSize( m_aGetExtensions.CalcMinimumSize() );
    aPos = Point( RSC_SP_DLG_INNERBORDER_LEFT, aPos.Y() - RSC_CD_FIXEDTEXT_HEIGHT - 2*RSC_SP_DLG_INNERBORDER_BOTTOM );
    
    m_aGetExtensions.SetPosSizePixel( aPos, aFTSize );

    aPos.X() = aTotalSize.Width() - RSC_SP_DLG_INNERBORDER_RIGHT - aBtnSize.Width();
    m_aCancelBtn.SetPosPixel( Point( aPos.X(), aPos.Y() - ((aBtnSize.Height()-aFTSize.Height())/2) ) );
    
    // Calc progress height
    long nProgressHeight = aFTSize.Height();

    if( IsNativeControlSupported( CTRL_PROGRESS, PART_ENTIRE_CONTROL ) )
    {
        ImplControlValue aValue;
        bool bNativeOK;
        Region aControlRegion( Rectangle( (const Point&)Point(), m_aProgressBar.GetSizePixel() ) );
        Region aNativeControlRegion, aNativeContentRegion;
        if( (bNativeOK = GetNativeControlRegion( CTRL_PROGRESS, PART_ENTIRE_CONTROL, aControlRegion,
                                                 CTRL_STATE_ENABLED, aValue, rtl::OUString(),
                                                 aNativeControlRegion, aNativeContentRegion ) ) != FALSE )
        {
            nProgressHeight = aNativeControlRegion.GetBoundRect().GetHeight();
        }
    }

    if ( nProgressHeight < PROGRESS_HEIGHT )
        nProgressHeight = PROGRESS_HEIGHT;

    aPos.X() -= ( RSC_SP_CTRL_GROUP_Y + PROGRESS_WIDTH );
    m_aProgressBar.SetPosSizePixel( Point( aPos.X(), aPos.Y() - ((nProgressHeight-aFTSize.Height())/2) ),
                                    Size( PROGRESS_WIDTH, nProgressHeight ) );

    Rectangle aRect1( m_aGetExtensions.GetPosPixel(), m_aGetExtensions.GetSizePixel() );
    Rectangle aRect2( m_aProgressBar.GetPosPixel(), m_aProgressBar.GetSizePixel() );

    aFTSize.Width() = ( aRect2.Left() - aRect1.Right() ) - 2*RSC_SP_DLG_INNERBORDER_LEFT;
    aPos.X() = aRect1.Right() + RSC_SP_DLG_INNERBORDER_LEFT;
    m_aProgressText.SetPosSizePixel( aPos, aFTSize );

    Size aSize( aTotalSize.Width() - RSC_SP_DLG_INNERBORDER_LEFT - RSC_SP_DLG_INNERBORDER_RIGHT,
                aTotalSize.Height() - 2*aBtnSize.Height() - LINE_SIZE -
                RSC_SP_DLG_INNERBORDER_TOP - 3*RSC_SP_DLG_INNERBORDER_BOTTOM );

    m_pExtensionBox->SetSizePixel( aSize );
}
//------------------------------------------------------------------------------
// VCL::Window / Dialog

long ExtMgrDialog::Notify( NotifyEvent& rNEvt )
{
    bool bHandled = false;

    if ( rNEvt.GetType() == EVENT_KEYINPUT )
    {
        const KeyEvent* pKEvt = rNEvt.GetKeyEvent();
        KeyCode         aKeyCode = pKEvt->GetKeyCode();
        USHORT          nKeyCode = aKeyCode.GetCode();

        if ( nKeyCode == KEY_TAB )
        {
            if ( aKeyCode.IsShift() ) {
                if ( m_aAddBtn.HasFocus() ) {
                    m_pExtensionBox->GrabFocus();
                    bHandled = true;
                }
            } else {
                if ( m_aGetExtensions.HasFocus() ) {
                    m_pExtensionBox->GrabFocus();
                    bHandled = true;
                }
            }
        }
        if ( aKeyCode.GetGroup() == KEYGROUP_CURSOR )
            bHandled = m_pExtensionBox->Notify( rNEvt );
    }
// VCLEVENT_WINDOW_CLOSE
    if ( !bHandled )
        return ModelessDialog::Notify( rNEvt );
    else
        return true;
}

//------------------------------------------------------------------------------
BOOL ExtMgrDialog::Close()
{
    bool bRet = m_pManager->queryTermination();
    if ( bRet )
    {
        bRet = ModelessDialog::Close();
        m_pManager->terminateDialog();
    }
    return bRet;
}

//------------------------------------------------------------------------------
//                             UpdateRequiredDialog
//------------------------------------------------------------------------------
UpdateRequiredDialog::UpdateRequiredDialog( Window *pParent, TheExtensionManager *pManager ) :
    ModalDialog( pParent,   getResId( RID_DLG_UPDATE_REQUIRED ) ),
    DialogHelper( pManager->getContext(), (Dialog*) this ),
    m_aUpdateNeeded( this,  getResId( RID_EM_FT_MSG ) ),
    m_aUpdateBtn( this,     getResId( RID_EM_BTN_CHECK_UPDATES ) ),
    m_aCloseBtn( this,      getResId( RID_EM_BTN_CLOSE ) ),
    m_aHelpBtn( this,       getResId( RID_EM_BTN_HELP ) ),
    m_aCancelBtn( this,     getResId( RID_EM_BTN_CANCEL ) ),
    m_aDivider( this ),
    m_aProgressText( this,  getResId( RID_EM_FT_PROGRESS ) ),
    m_aProgressBar( this,   WB_BORDER + WB_3DLOOK ),
    m_sAddPackages(         getResourceString( RID_STR_ADD_PACKAGES ) ),
    m_sCloseText(           getResourceString( RID_STR_CLOSE_BTN ) ),
    m_bHasProgress(         false ),
    m_bProgressChanged(     false ),
    m_bStartProgress(       false ),
    m_bStopProgress(        false ),
    m_bUpdateWarning(       false ),
    m_bDisableWarning(      false ),
    m_bHasLockedEntries(    false ),
    m_nProgress(            0 ),
    m_pManager( pManager )
{
    // free local resources (RID < 256):
    FreeResource();

    m_pExtensionBox = new ExtensionBox_Impl( this, pManager );
    m_pExtensionBox->SetHyperlinkHdl( LINK( this, UpdateRequiredDialog, HandleHyperlink ) );

    m_aUpdateBtn.SetClickHdl( LINK( this, UpdateRequiredDialog, HandleUpdateBtn ) );
    m_aCloseBtn.SetClickHdl( LINK( this, UpdateRequiredDialog, HandleCloseBtn ) );
    m_aCancelBtn.SetClickHdl( LINK( this, UpdateRequiredDialog, HandleCancelBtn ) );

    String aText = m_aUpdateNeeded.GetText();
    aText.SearchAndReplaceAllAscii( "%PRODUCTNAME", BrandName::get() );
    m_aUpdateNeeded.SetText( aText );

    // resize update button
    Size aBtnSize = m_aUpdateBtn.GetSizePixel();
    String sTitle = m_aUpdateBtn.GetText();
    long nWidth = m_aUpdateBtn.GetCtrlTextWidth( sTitle );
    nWidth += 2 * m_aUpdateBtn.GetTextHeight();
    if ( nWidth > aBtnSize.Width() )
        m_aUpdateBtn.SetSizePixel( Size( nWidth, aBtnSize.Height() ) );

    // resize update button
    aBtnSize = m_aCloseBtn.GetSizePixel();
    sTitle = m_aCloseBtn.GetText();
    nWidth = m_aCloseBtn.GetCtrlTextWidth( sTitle );
    nWidth += 2 * m_aCloseBtn.GetTextHeight();
    if ( nWidth > aBtnSize.Width() )
        m_aCloseBtn.SetSizePixel( Size( nWidth, aBtnSize.Height() ) );

    // minimum size:
    SetMinOutputSizePixel(
        Size( // width:
              (5 * m_aHelpBtn.GetSizePixel().Width()) +
              (5 * RSC_SP_DLG_INNERBORDER_LEFT ),
              // height:
              (1 * m_aHelpBtn.GetSizePixel().Height()) +
              (1 * m_aUpdateNeeded.GetSizePixel().Height()) +
              (1 * m_pExtensionBox->GetMinOutputSizePixel().Height()) +
              (3 * RSC_SP_DLG_INNERBORDER_LEFT) ) );

    m_aDivider.Show();
    m_aProgressBar.Hide();
    m_aUpdateBtn.Enable( false );
    m_aCloseBtn.GrabFocus();

    m_aTimeoutTimer.SetTimeout( 50 ); // mSec
    m_aTimeoutTimer.SetTimeoutHdl( LINK( this, UpdateRequiredDialog, TimeOutHdl ) );
}

//------------------------------------------------------------------------------
UpdateRequiredDialog::~UpdateRequiredDialog()
{
    m_aTimeoutTimer.Stop();

    delete m_pExtensionBox;
}

//------------------------------------------------------------------------------
long UpdateRequiredDialog::addPackageToList( const uno::Reference< deployment::XPackage > &xPackage,
                                             const uno::Reference< deployment::XPackageManager > &xPackageManager )
{
    // We will only add entries to the list with unsatisfied dependencies
    if ( !checkDependencies( xPackage ) )
    {
        m_bHasLockedEntries |= (bool) xPackageManager->isReadOnly();
        m_aUpdateBtn.Enable( true );
        return m_pExtensionBox->addEntry( xPackage, xPackageManager );
    }
    return 0;
}

//------------------------------------------------------------------------------
void UpdateRequiredDialog::prepareChecking( const uno::Reference< deployment::XPackageManager > &xPackageManager )
{
    if ( xPackageManager.is() )
        m_pExtensionBox->prepareChecking( xPackageManager );
}

//------------------------------------------------------------------------------
void UpdateRequiredDialog::checkEntries()
{
    const ::vos::OGuard guard( Application::GetSolarMutex() );
    m_pExtensionBox->checkEntries();

    if ( ! hasActiveEntries() )
    {
        m_aCloseBtn.SetText( m_sCloseText );
        m_aCloseBtn.GrabFocus();
    }
}

//------------------------------------------------------------------------------
bool UpdateRequiredDialog::enablePackage( const uno::Reference< deployment::XPackageManager > &,
                                  const uno::Reference< deployment::XPackage > &xPackage,
                                  bool bEnable )
{
    m_pManager->enablePackage( xPackage, bEnable );

    return true;
}

//------------------------------------------------------------------------------
IMPL_LINK( UpdateRequiredDialog, HandleCancelBtn, void*, EMPTYARG )
{
    // m_dialog->m_cmdEnv->m_aborted = true;
    if ( m_xAbortChannel.is() )
    {
        try
        {
            m_xAbortChannel->sendAbort();
        }
        catch ( uno::RuntimeException & )
        {
            OSL_ENSURE( 0, "### unexpected RuntimeException!" );
        }
    }
    return 1;
}

// ------------------------------------------------------------------------------
IMPL_LINK( UpdateRequiredDialog, startProgress, void*, _bLockInterface )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    bool bLockInterface = (bool) _bLockInterface;

    if ( m_bStartProgress && !m_bHasProgress )
        m_aTimeoutTimer.Start();

    if ( m_bStopProgress )
    {
		if ( m_aProgressBar.IsVisible() )
	        m_aProgressBar.SetValue( 100 );
        m_xAbortChannel.clear();
        OSL_TRACE( " startProgress handler: stop\n" );
    }
    else
    {
        OSL_TRACE( " startProgress handler: start\n" );
    }

    m_aCancelBtn.Enable( bLockInterface );
    m_aUpdateBtn.Enable( false );
    clearEventID();

    return 0;
}

// ------------------------------------------------------------------------------
void UpdateRequiredDialog::showProgress( bool _bStart )
{
    ::osl::MutexGuard aGuard( m_aMutex );

    bool bStart = _bStart;

    if ( bStart )
    {
        m_nProgress = 0;
        m_bStartProgress = true;
        OSL_TRACE( "showProgress start\n" );
    }
    else
    {
        m_nProgress = 100;
        m_bStopProgress = true;
        OSL_TRACE( "showProgress stop!\n" );
    }

    DialogHelper::PostUserEvent( LINK( this, UpdateRequiredDialog, startProgress ), (void*) bStart );
}

// -----------------------------------------------------------------------
void UpdateRequiredDialog::updateProgress( const long nProgress )
{
    ::osl::MutexGuard aGuard( m_aMutex );

    m_nProgress = nProgress;
}

// -----------------------------------------------------------------------
void UpdateRequiredDialog::updateProgress( const OUString &rText,
                                           const uno::Reference< task::XAbortChannel > &xAbortChannel)
{
    ::osl::MutexGuard aGuard( m_aMutex );

    m_xAbortChannel = xAbortChannel;
    m_sProgressText = rText;
    m_bProgressChanged = true;
}

//------------------------------------------------------------------------------
void UpdateRequiredDialog::updatePackageInfo( const uno::Reference< deployment::XPackage > &xPackage )
{
    // We will remove all updated packages with satisfied dependencies, but
    // we will show all disabled entries so the user sees the result
    // of the 'disable all' button
    if ( isEnabled( xPackage ) && checkDependencies( xPackage ) )
        m_pExtensionBox->removeEntry( xPackage );
    else
        m_pExtensionBox->updateEntry( xPackage );

    if ( ! hasActiveEntries() )
    {
        m_aCloseBtn.SetText( m_sCloseText );
        m_aCloseBtn.GrabFocus();
    }
}

// -----------------------------------------------------------------------
IMPL_LINK( UpdateRequiredDialog, HandleUpdateBtn, void*, EMPTYARG )
{
    ::osl::ClearableMutexGuard aGuard( m_aMutex );

    std::vector< TUpdateListEntry > vUpdateEntries;
    sal_Int32 nCount = m_pExtensionBox->GetEntryCount();

    for ( sal_Int32 i = 0; i < nCount; ++i )
    {
        TEntry_Impl pEntry = m_pExtensionBox->GetEntryData( i );
        TUpdateListEntry pUpdateEntry( new UpdateListEntry( pEntry->m_xPackage,
                                                            pEntry->m_xPackageManager ) );
        vUpdateEntries.push_back( pUpdateEntry );
    }

    aGuard.clear();

    m_pManager->updatePackages( vUpdateEntries );

    return 1;
}

// -----------------------------------------------------------------------
IMPL_LINK( UpdateRequiredDialog, HandleCloseBtn, void*, EMPTYARG )
{
    ::osl::MutexGuard aGuard( m_aMutex );

    if ( !isBusy() )
    {
        if ( m_bHasLockedEntries )
            EndDialog( -1 );
        else if ( hasActiveEntries() )
            disableAllEntries();
        else
            EndDialog( 0 );
    }

    return 1;
}

// -----------------------------------------------------------------------
IMPL_LINK( UpdateRequiredDialog, HandleHyperlink, svt::FixedHyperlink*, pHyperlink )
{
    openWebBrowser( pHyperlink->GetURL(), GetText() );

    return 1;
}

// -----------------------------------------------------------------------
IMPL_LINK( UpdateRequiredDialog, TimeOutHdl, Timer*, EMPTYARG )
{
    if ( m_bStopProgress )
    {
        m_bHasProgress = false;
        m_bStopProgress = false;
        m_aProgressText.Hide();
        m_aProgressBar.Hide();
        m_aCancelBtn.Hide();
    }
    else
    {
        if ( m_bProgressChanged )
        {
            m_bProgressChanged = false;
            m_aProgressText.SetText( m_sProgressText );
        }

        if ( m_bStartProgress )
        {
            m_bStartProgress = false;
            m_bHasProgress = true;
            m_aProgressBar.Show();
            m_aProgressText.Show();
            m_aCancelBtn.Enable();
            m_aCancelBtn.Show();
        }

		if ( m_aProgressBar.IsVisible() )
			m_aProgressBar.SetValue( (USHORT) m_nProgress );

        m_aTimeoutTimer.Start();
    }

	return 1;
}

//------------------------------------------------------------------------------
// VCL::Window / Dialog
void UpdateRequiredDialog::Resize()
{
    Size aTotalSize( GetOutputSizePixel() );
    Size aBtnSize( m_aHelpBtn.GetSizePixel() );

    Point aPos( RSC_SP_DLG_INNERBORDER_LEFT,
                aTotalSize.Height() - RSC_SP_DLG_INNERBORDER_BOTTOM - aBtnSize.Height() );
    
    m_aHelpBtn.SetPosPixel( aPos );

    aPos.X() = aTotalSize.Width() - RSC_SP_DLG_INNERBORDER_RIGHT - m_aCloseBtn.GetSizePixel().Width();
    m_aCloseBtn.SetPosPixel( aPos );

    aPos.X() -= ( RSC_SP_CTRL_X + m_aUpdateBtn.GetSizePixel().Width() );
    m_aUpdateBtn.SetPosPixel( aPos );

    Size aDivSize( aTotalSize.Width(), LINE_SIZE );
    aPos = Point( 0, aPos.Y() - LINE_SIZE - RSC_SP_DLG_INNERBORDER_BOTTOM );
    m_aDivider.SetPosSizePixel( aPos, aDivSize );

    // Calc fixed text size
    aPos = Point( RSC_SP_DLG_INNERBORDER_LEFT, RSC_SP_DLG_INNERBORDER_TOP );
    Size aFTSize = m_aUpdateNeeded.CalcMinimumSize( aTotalSize.Width() - RSC_SP_DLG_INNERBORDER_RIGHT - RSC_SP_DLG_INNERBORDER_LEFT );
    m_aUpdateNeeded.SetPosSizePixel( aPos, aFTSize );

    // Calc list box size
    Size aSize( aTotalSize.Width() - RSC_SP_DLG_INNERBORDER_LEFT - RSC_SP_DLG_INNERBORDER_RIGHT,
                aTotalSize.Height() - 2*aBtnSize.Height() - LINE_SIZE -
                2*RSC_SP_DLG_INNERBORDER_TOP - 3*RSC_SP_DLG_INNERBORDER_BOTTOM - aFTSize.Height() );
    aPos.Y() += aFTSize.Height()+RSC_SP_DLG_INNERBORDER_TOP;

    m_pExtensionBox->SetPosSizePixel( aPos, aSize );

    aPos.X() = aTotalSize.Width() - RSC_SP_DLG_INNERBORDER_RIGHT - aBtnSize.Width();
    aPos.Y() += aSize.Height()+RSC_SP_DLG_INNERBORDER_TOP;
    m_aCancelBtn.SetPosPixel( aPos );

    // Calc progress height
    aFTSize = m_aProgressText.GetSizePixel();
    long nProgressHeight = aFTSize.Height();

    if( IsNativeControlSupported( CTRL_PROGRESS, PART_ENTIRE_CONTROL ) )
    {
        ImplControlValue aValue;
        bool bNativeOK;
        Region aControlRegion( Rectangle( (const Point&)Point(), m_aProgressBar.GetSizePixel() ) );
        Region aNativeControlRegion, aNativeContentRegion;
        if( (bNativeOK = GetNativeControlRegion( CTRL_PROGRESS, PART_ENTIRE_CONTROL, aControlRegion,
                                                 CTRL_STATE_ENABLED, aValue, rtl::OUString(),
                                                 aNativeControlRegion, aNativeContentRegion ) ) != FALSE )
        {
            nProgressHeight = aNativeControlRegion.GetBoundRect().GetHeight();
        }
    }

    if ( nProgressHeight < PROGRESS_HEIGHT )
        nProgressHeight = PROGRESS_HEIGHT;

    aPos.X() -= ( RSC_SP_CTRL_GROUP_Y + PROGRESS_WIDTH );
    m_aProgressBar.SetPosSizePixel( Point( aPos.X(), aPos.Y() + ((aBtnSize.Height()-nProgressHeight)/2) ),
                                    Size( PROGRESS_WIDTH, nProgressHeight ) );

    aFTSize.Width() = aPos.X() - 2*RSC_SP_DLG_INNERBORDER_LEFT;
    aPos.X() = RSC_SP_DLG_INNERBORDER_LEFT;
    aPos.Y() += ( aBtnSize.Height() - aFTSize.Height() - 1 ) / 2;
    m_aProgressText.SetPosSizePixel( aPos, aFTSize );
}

//------------------------------------------------------------------------------
// VCL::Dialog
short UpdateRequiredDialog::Execute()
{
    if ( m_bHasLockedEntries )
    {
        // Set other text, disable update btn, remove not shared entries from list;
        m_aUpdateNeeded.SetText( DialogHelper::getResourceString( RID_STR_NO_ADMIN_PRIVILEGE ) );
        m_aCloseBtn.SetText( DialogHelper::getResourceString( RID_STR_EXIT_BTN ) );
        m_aUpdateBtn.Enable( false );
        m_pExtensionBox->RemoveUnlocked();
        Resize();
    }

    return Dialog::Execute();
}

//------------------------------------------------------------------------------
// VCL::Dialog
BOOL UpdateRequiredDialog::Close()
{
    ::osl::MutexGuard aGuard( m_aMutex );

    if ( !isBusy() )
    {
        if ( m_bHasLockedEntries )
            EndDialog( -1 );
        else if ( hasActiveEntries() )
            disableAllEntries();
        else
            EndDialog( 0 );
    }

    return false;
}

//------------------------------------------------------------------------------
// Check dependencies of all packages
//------------------------------------------------------------------------------
bool UpdateRequiredDialog::isEnabled( const uno::Reference< deployment::XPackage > &xPackage ) const
{
    bool bRegistered = false;
    try {
        beans::Optional< beans::Ambiguous< sal_Bool > > option( xPackage->isRegistered( uno::Reference< task::XAbortChannel >(),
                                                                                        uno::Reference< ucb::XCommandEnvironment >() ) );
        if ( option.IsPresent )
        {
            ::beans::Ambiguous< sal_Bool > const & reg = option.Value;
            if ( reg.IsAmbiguous )
                bRegistered = false;
            else
                bRegistered = reg.Value ? true : false;
        }
        else
            bRegistered = false;
    }
    catch ( uno::RuntimeException & ) { throw; }
    catch ( uno::Exception & exc) {
        (void) exc;
        OSL_ENSURE( 0, ::rtl::OUStringToOString( exc.Message, RTL_TEXTENCODING_UTF8 ).getStr() );
        bRegistered = false;
    }

    return bRegistered;
}

//------------------------------------------------------------------------------
bool UpdateRequiredDialog::checkDependencies( const uno::Reference< deployment::XPackage > &xPackage ) const
{
    if ( isEnabled( xPackage ) )
    {
        bool bDependenciesValid = false;
        try {
            bDependenciesValid = xPackage->checkDependencies( uno::Reference< ucb::XCommandEnvironment >() );
        }
        catch ( deployment::DeploymentException & ) {}
        if ( ! bDependenciesValid )
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
bool UpdateRequiredDialog::hasActiveEntries()
{
    ::osl::MutexGuard aGuard( m_aMutex );

    bool bRet = false;
    long nCount = m_pExtensionBox->GetEntryCount();
    for ( long nIndex = 0; nIndex < nCount; nIndex++ )
    {
        TEntry_Impl pEntry = m_pExtensionBox->GetEntryData( nIndex );
        
        if ( !checkDependencies( pEntry->m_xPackage ) )
        {
            bRet = true;
            break;
        }
    }

    return bRet;
}

//------------------------------------------------------------------------------
void UpdateRequiredDialog::disableAllEntries()
{
    ::osl::MutexGuard aGuard( m_aMutex );

    setBusy( true );

    long nCount = m_pExtensionBox->GetEntryCount();
    for ( long nIndex = 0; nIndex < nCount; nIndex++ )
    {
        TEntry_Impl pEntry = m_pExtensionBox->GetEntryData( nIndex );
        enablePackage( pEntry->m_xPackageManager, pEntry->m_xPackage, false );
    }

    setBusy( false );
    
    if ( ! hasActiveEntries() )
        m_aCloseBtn.SetText( m_sCloseText );
}

//=================================================================================
// UpdateRequiredDialogService
//=================================================================================
UpdateRequiredDialogService::UpdateRequiredDialogService( uno::Sequence< uno::Any > const&,
                                                          uno::Reference< uno::XComponentContext > const& xComponentContext )
    : m_xComponentContext( xComponentContext )
{
}

//------------------------------------------------------------------------------
// XExecutableDialog
//------------------------------------------------------------------------------
void UpdateRequiredDialogService::setTitle( OUString const & ) throw ( uno::RuntimeException )
{
}

//------------------------------------------------------------------------------
sal_Int16 UpdateRequiredDialogService::execute() throw ( uno::RuntimeException )
{
    ::rtl::Reference< ::dp_gui::TheExtensionManager > xManager( TheExtensionManager::get(
                                                              m_xComponentContext,
                                                              uno::Reference< awt::XWindow >(),
                                                              OUString() ) );
    xManager->createDialog( true );
    sal_Int16 nRet = xManager->execute();

    return nRet;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SelectedPackage::~SelectedPackage() {}

} //namespace dp_gui

