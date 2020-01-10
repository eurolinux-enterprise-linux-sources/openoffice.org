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

#include "pages.hxx"
#include "wizard.hrc"
#include "wizard.hxx"
#include "migration.hxx"
#include <vcl/msgbox.hxx>
#include <vcl/mnemonic.hxx>
#include <vos/security.hxx>
#include <app.hxx>
#include <rtl/ustring.hxx>
#include <osl/file.hxx>
#include <unotools/bootstrap.hxx>
#include <unotools/configmgr.hxx>
#include <svtools/regoptions.hxx>
#include <svtools/useroptions.hxx>
#include <sfx2/basedlgs.hxx>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/frame/XDesktop.hpp>
#include <com/sun/star/beans/XMaterialHolder.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/container/XNameReplace.hpp>
#include <com/sun/star/task/XJobExecutor.hpp>
#include <comphelper/configurationhelper.hxx>
#include <rtl/bootstrap.hxx>
#include <rtl/ustrbuf.hxx>
#include <osl/file.hxx>
#include <osl/thread.hxx>
#include <unotools/bootstrap.hxx>
#include <tools/config.hxx>

using namespace rtl;
using namespace osl;
using namespace utl;
using namespace svt;
using namespace com::sun::star;
using namespace com::sun::star::frame;
using namespace com::sun::star::lang;
using namespace com::sun::star::util;
using namespace com::sun::star::beans;
using namespace com::sun::star::uno;
using namespace com::sun::star::container;

#define UNISTRING(s) rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(s))

namespace desktop {

static void _setBold(FixedText& ft)
{
    Font f = ft.GetControlFont();
    f.SetWeight(WEIGHT_BOLD);
    ft.SetControlFont(f);
}

WelcomePage::WelcomePage( svt::OWizardMachine* parent, const ResId& resid, sal_Bool bLicenseNeedsAcceptance )
    : OWizardPage(parent, resid)
    , m_ftHead(this, WizardResId(FT_WELCOME_HEADER))
    , m_ftBody(this, WizardResId(FT_WELCOME_BODY))
    , m_pParent(parent)
    , m_bLicenseNeedsAcceptance( bLicenseNeedsAcceptance )
    , bIsEvalVersion(false)
    , bNoEvalText(false)
{
    FreeResource();

    _setBold(m_ftHead);

    checkEval();
    // we need to choose the welcome text that is diplayed
    // choices are the default text, default text+migradtion,
    // OEM and extended OEM
    // No OEM is built, remove the check
//     switch (checkOEM())
//     {
//     case OEM_NONE:
        // check for migration
        if (Migration::checkMigration())
        {
            String aText(WizardResId(STR_WELCOME_MIGRATION));
            // replace %OLDPRODUCT with found version name
            aText.SearchAndReplaceAll( UniString::CreateFromAscii("%OLD_VERSION"), Migration::getOldVersionName());
            m_ftBody.SetText( aText );
        }
		else
		if ( ! m_bLicenseNeedsAcceptance )
		{
            String aText(WizardResId(STR_WELCOME_WITHOUT_LICENSE));
            m_ftBody.SetText( aText );
		}
}


void WelcomePage::checkEval()
{
    Reference< XMultiServiceFactory > xFactory = ::comphelper::getProcessServiceFactory();
    Reference< XMaterialHolder > xHolder(xFactory->createInstance(
        OUString::createFromAscii("com.sun.star.tab.tabreg")), UNO_QUERY);
    if (xHolder.is()) {
        Any aData = xHolder->getMaterial();
        Sequence < NamedValue > aSeq;
        if (aData >>= aSeq) {
            bIsEvalVersion = true;
            for (int i=0; i< aSeq.getLength(); i++) {
                if (aSeq[i].Name.equalsAscii("NoEvalText")) {
                    aSeq[i].Value >>= bNoEvalText;
                }
            }
        }
    }
}


void WelcomePage::ActivatePage()
{
    OWizardPage::ActivatePage();
    // this page has no controls, so forwarding to default
    // button (next) won't work if we grap focus
    // GrabFocus();
}

LicensePage::LicensePage( svt::OWizardMachine* parent, const ResId& resid, const rtl::OUString &rLicensePath )
    : OWizardPage(parent, resid)
    , m_pParent(parent)
    , m_ftHead(this, WizardResId(FT_LICENSE_HEADER))
    , m_ftBody1(this, WizardResId(FT_LICENSE_BODY_1))
    , m_ftBody1Txt(this, WizardResId(FT_LICENSE_BODY_1_TXT))
    , m_ftBody2(this, WizardResId(FT_LICENSE_BODY_2))
    , m_ftBody2Txt(this, WizardResId(FT_LICENSE_BODY_2_TXT))
    , m_mlLicense(this, WizardResId(ML_LICENSE))
    , m_pbDown(this, WizardResId(PB_LICENSE_DOWN))
    , m_bLicenseRead(sal_False)
{
    FreeResource();

    _setBold(m_ftHead);

    m_mlLicense.SetEndReachedHdl( LINK(this, LicensePage, EndReachedHdl) );
    m_mlLicense.SetScrolledHdl( LINK(this, LicensePage, ScrolledHdl) );
    m_pbDown.SetClickHdl( LINK(this, LicensePage, PageDownHdl) );

    // We want a automatic repeating page down button
    WinBits aStyle = m_pbDown.GetStyle();
    aStyle |= WB_REPEAT;
    m_pbDown.SetStyle( aStyle );

    // replace %PAGEDOWN in text2 with button text
    String aText = m_ftBody1Txt.GetText();
    aText.SearchAndReplaceAll( UniString::CreateFromAscii("%PAGEDOWN"),
        MnemonicGenerator::EraseAllMnemonicChars(m_pbDown.GetText()));

    m_ftBody1Txt.SetText( aText );

    // load license text
    File aLicenseFile(rLicensePath);
    if ( aLicenseFile.open(OpenFlag_Read) == FileBase::E_None)
    {
        DirectoryItem d;
        DirectoryItem::get(rLicensePath, d);
        FileStatus fs(FileStatusMask_FileSize);
        d.getFileStatus(fs);
        sal_uInt64 nBytesRead = 0;
        sal_uInt64 nPosition = 0;
        sal_uInt32 nBytes = (sal_uInt32)fs.getFileSize();
        sal_Char *pBuffer = new sal_Char[nBytes];
        // FileBase RC r = FileBase::E_None;
        while (aLicenseFile.read(pBuffer+nPosition, nBytes-nPosition, nBytesRead) == FileBase::E_None
            && nPosition + nBytesRead < nBytes)
        {
            nPosition += nBytesRead;
        }
        OUString aLicenseString(pBuffer, nBytes, RTL_TEXTENCODING_UTF8,
                OSTRING_TO_OUSTRING_CVTFLAGS | RTL_TEXTTOUNICODE_FLAGS_GLOBAL_SIGNATURE);
        delete[] pBuffer;
        m_mlLicense.SetText(aLicenseString);

    }
}

void LicensePage::ActivatePage()
{
    OWizardPage::ActivatePage();
    m_bLicenseRead = m_mlLicense.IsEndReached();
    m_pbDown.GrabFocus();
    updateDialogTravelUI();
}

bool LicensePage::canAdvance() const
{
    if (m_mlLicense.IsEndReached())
        const_cast< LicensePage* >( this )->m_pbDown.Disable();
    else
        const_cast< LicensePage* >( this )->m_pbDown.Enable();

    return m_bLicenseRead;
}

IMPL_LINK( LicensePage, PageDownHdl, PushButton *, EMPTYARG )
{
    m_mlLicense.ScrollDown( SCROLL_PAGEDOWN );
    return 0;
}

IMPL_LINK( LicensePage, EndReachedHdl, LicenseView *, EMPTYARG )
{
    m_bLicenseRead = TRUE;
    updateDialogTravelUI();
    return 0;
}

IMPL_LINK( LicensePage, ScrolledHdl, LicenseView *, EMPTYARG )
{
    updateDialogTravelUI();
    return 0;
}


LicenseView::LicenseView( Window* pParent, const ResId& rResId )
    : MultiLineEdit( pParent, rResId )
{
    SetLeftMargin( 5 );
    mbEndReached = IsEndReached();
    StartListening( *GetTextEngine() );
}

LicenseView::~LicenseView()
{
    maEndReachedHdl = Link();
    maScrolledHdl   = Link();
    EndListeningAll();
}

void LicenseView::ScrollDown( ScrollType eScroll )
{
    ScrollBar*  pScroll = GetVScrollBar();
    if ( pScroll )
        pScroll->DoScrollAction( eScroll );
}

BOOL LicenseView::IsEndReached() const
{
    BOOL bEndReached;

    ExtTextView*    pView = GetTextView();
    ExtTextEngine*  pEdit = GetTextEngine();
    ULONG           nHeight = pEdit->GetTextHeight();
    Size            aOutSize = pView->GetWindow()->GetOutputSizePixel();
    Point           aBottom( 0, aOutSize.Height() );

    if ( (ULONG) pView->GetDocPos( aBottom ).Y() >= nHeight - 1 )
        bEndReached = TRUE;
    else
        bEndReached = FALSE;

    return bEndReached;
}

void LicenseView::Notify( SfxBroadcaster&, const SfxHint& rHint )
{
    if ( rHint.IsA( TYPE(TextHint) ) )
    {
        BOOL    bLastVal = EndReached();
        ULONG   nId = ((const TextHint&)rHint).GetId();

        if ( nId == TEXT_HINT_PARAINSERTED )
        {
            if ( bLastVal )
                mbEndReached = IsEndReached();
        }
        else if ( nId == TEXT_HINT_VIEWSCROLLED )
        {
            if ( ! mbEndReached )
                mbEndReached = IsEndReached();
            maScrolledHdl.Call( this );
        }

        if ( EndReached() && !bLastVal )
        {
            maEndReachedHdl.Call( this );
        }
    }
}



// -------------------------------------------------------------------

class MigrationThread : public ::osl::Thread
{
    public:
        MigrationThread();
        
        virtual void SAL_CALL run();
        virtual void SAL_CALL onTerminated(); 
};

MigrationThread::MigrationThread()
{
}

void MigrationThread::run()
{
    try
    {
        Migration::doMigration();
    }
    catch ( uno::Exception& )
    {
    }
}

void MigrationThread::onTerminated()
{
}

// -------------------------------------------------------------------

MigrationPage::MigrationPage( 
    svt::OWizardMachine* parent, 
    const ResId& resid, 
    ::com::sun::star::uno::Reference< ::com::sun::star::awt::XThrobber > xThrobber)
    : OWizardPage(parent, resid)
    , m_ftHead(this, WizardResId(FT_MIGRATION_HEADER))
    , m_ftBody(this, WizardResId(FT_MIGRATION_BODY))
    , m_cbMigration(this, WizardResId(CB_MIGRATION))
    , m_bMigrationDone(sal_False)
    , m_xThrobber(xThrobber)
{
    FreeResource();
    _setBold(m_ftHead);

    // replace %OLDPRODUCT with found version name
    String aText = m_ftBody.GetText();
    aText.SearchAndReplaceAll( UniString::CreateFromAscii("%OLDPRODUCT"), Migration::getOldVersionName());
    m_ftBody.SetText( aText );
}

sal_Bool MigrationPage::commitPage( CommitPageReason _eReason )
{
    if (_eReason == eTravelForward && m_cbMigration.IsChecked() && !m_bMigrationDone)
    {        
        GetParent()->EnterWait();
        FirstStartWizard* pWizard = dynamic_cast< FirstStartWizard* >( GetParent() );
        if ( pWizard )
            pWizard->DisableButtonsWhileMigration();
        
        uno::Reference< awt::XWindow > xWin( m_xThrobber, uno::UNO_QUERY );
        xWin->setVisible( true );
        m_xThrobber->start();
        MigrationThread* pMigThread = new MigrationThread();
        pMigThread->create();
        
		while ( pMigThread->isRunning() )
        {
            Application::Reschedule();
        }
        
        m_xThrobber->stop();
        GetParent()->LeaveWait();
        // Next state will enable buttons - so no EnableButtons necessary!
        xWin->setVisible( false );
        pMigThread->join();
        delete pMigThread;
        m_bMigrationDone = sal_True;
    }
    else
        Migration::cancelMigration();
    return sal_True;
}

void MigrationPage::ActivatePage()
{
    OWizardPage::ActivatePage();
    GrabFocus();
}

UserPage::UserPage( svt::OWizardMachine* parent, const ResId& resid)
	: OWizardPage(parent, resid)
	, m_ftHead(this, WizardResId(FT_USER_HEADER))
    , m_ftBody(this, WizardResId(FT_USER_BODY))
    , m_ftFirst(this, WizardResId(FT_USER_FIRST))
    , m_edFirst(this, WizardResId(ED_USER_FIRST))
	, m_ftLast(this, WizardResId(FT_USER_LAST))
	, m_edLast(this, WizardResId(ED_USER_LAST))
    , m_ftInitials(this, WizardResId(FT_USER_INITIALS))
	, m_edInitials(this, WizardResId(ED_USER_INITIALS))
    , m_ftFather(this, WizardResId(FT_USER_FATHER))
	, m_edFather(this, WizardResId(ED_USER_FATHER))
    , m_lang(Application::GetSettings().GetUILanguage())
{
    FreeResource();
    _setBold(m_ftHead);

	// check whether this is a russian version. otherwise
	// we'll hide the 'Fathers name' field
    SvtUserOptions aUserOpt;
    m_edFirst.SetText(aUserOpt.GetFirstName());
    m_edLast.SetText(aUserOpt.GetLastName());
#if 0
    rtl::OUString aUserName;
    vos::OSecurity().getUserName( aUserName );
   	aUserOpt.SetID( aUserName );
#endif

    m_edInitials.SetText(aUserOpt.GetID());
    if (m_lang == LANGUAGE_RUSSIAN)
    {
	    m_ftFather.Show();
	    m_edFather.Show();
        m_edFather.SetText(aUserOpt.GetFathersName());
    }
}

sal_Bool UserPage::commitPage( CommitPageReason )
{
    SvtUserOptions aUserOpt;
    aUserOpt.SetFirstName(m_edFirst.GetText());
    aUserOpt.SetLastName(m_edLast.GetText());
    aUserOpt.SetID( m_edInitials.GetText());

    if (m_lang == LANGUAGE_RUSSIAN)
        aUserOpt.SetFathersName(m_edFather.GetText());

    return sal_True;
}

void UserPage::ActivatePage()
{
    OWizardPage::ActivatePage();
    GrabFocus();
}

// -------------------------------------------------------------------
UpdateCheckPage::UpdateCheckPage( svt::OWizardMachine* parent, const ResId& resid)
    : OWizardPage(parent, resid)
    , m_ftHead(this, WizardResId(FT_UPDATE_CHECK_HEADER))
    , m_ftBody(this, WizardResId(FT_UPDATE_CHECK_BODY))
    , m_cbUpdateCheck(this, WizardResId(CB_UPDATE_CHECK))
{
    FreeResource();
    _setBold(m_ftHead);
}

sal_Bool UpdateCheckPage::commitPage( CommitPageReason _eReason )
{
    if ( _eReason == eTravelForward )
    {        
        try {
            Reference < XNameReplace > xUpdateAccess;
            Reference < XMultiServiceFactory > xFactory( ::comphelper::getProcessServiceFactory() );

            xUpdateAccess = Reference < XNameReplace >(
                xFactory->createInstance( UNISTRING( "com.sun.star.setup.UpdateCheckConfig" ) ), UNO_QUERY_THROW );

            if ( !xUpdateAccess.is() )
                return sal_False;

            sal_Bool bAutoUpdChk = m_cbUpdateCheck.IsChecked();
            xUpdateAccess->replaceByName( UNISTRING("AutoCheckEnabled"), makeAny( bAutoUpdChk ) );

            Reference< XChangesBatch > xChangesBatch( xUpdateAccess, UNO_QUERY);
            if( xChangesBatch.is() && xChangesBatch->hasPendingChanges() )
                xChangesBatch->commitChanges();
        } catch (RuntimeException)
        {
        }
    }

    return sal_True;
}

void UpdateCheckPage::ActivatePage()
{
    OWizardPage::ActivatePage();
    GrabFocus();
}

// -------------------------------------------------------------------
RegistrationPage::RegistrationPage( Window* pParent, const ResId& rResid )
    : OWizardPage( pParent, rResid )
    , m_ftHeader(this, WizardResId(FT_REGISTRATION_HEADER))
    , m_ftBody(this, WizardResId(FT_REGISTRATION_BODY))
    , m_rbNow(this, WizardResId(RB_REGISTRATION_NOW))
    , m_rbLater(this, WizardResId(RB_REGISTRATION_LATER))
    , m_rbNever(this, WizardResId(RB_REGISTRATION_NEVER))
    , m_flSeparator(this, WizardResId(FL_REGISTRATION))
    , m_ftEnd(this, WizardResId(FT_REGISTRATION_END))
    , m_bNeverVisible( sal_True )
{
    FreeResource();

    // another text for OOo
    sal_Int32 nOpenSourceContext = 0;
    try
    {
        ::utl::ConfigManager::GetDirectConfigProperty(
            ::utl::ConfigManager::OPENSOURCECONTEXT ) >>= nOpenSourceContext;
    }
    catch( Exception& )
    {
        DBG_ERRORFILE( "RegistrationPage::RegistrationPage(): error while getting open source context" );
    }

    if ( nOpenSourceContext > 0 )
    {
        String sBodyText( WizardResId( STR_REGISTRATION_OOO ) );
        m_ftBody.SetText( sBodyText );
    }

    // calculate height of body text and rearrange the buttons
    Size aSize = m_ftBody.GetSizePixel();
    Size aMinSize = m_ftBody.CalcMinimumSize( aSize.Width() );
    long nTxtH = aMinSize.Height();
    long nCtrlH = aSize.Height();
    long nDelta = ( nCtrlH - nTxtH );
    aSize.Height() -= nDelta;
    m_ftBody.SetSizePixel( aSize );
    Window* pWins[] = { &m_rbNow, &m_rbLater, &m_rbNever };
    Window** pCurrent = pWins;
    for ( sal_uInt32 i = 0; i < sizeof( pWins ) / sizeof( pWins[ 0 ] ); ++i, ++pCurrent )
    {
        Point aNewPos = (*pCurrent)->GetPosPixel();
        aNewPos.Y() -= nDelta;
        (*pCurrent)->SetPosPixel( aNewPos );
    }

    _setBold(m_ftHeader);
    impl_retrieveConfigurationData();
    updateButtonStates();
}

bool RegistrationPage::canAdvance() const
{
    return false;
}

void RegistrationPage::ActivatePage()
{
    OWizardPage::ActivatePage();
    GrabFocus();
}

void RegistrationPage::impl_retrieveConfigurationData()
{
    static ::rtl::OUString PACKAGE = ::rtl::OUString::createFromAscii("org.openoffice.FirstStartWizard");
    static ::rtl::OUString PATH    = ::rtl::OUString::createFromAscii("TabPages/Registration/RegistrationOptions/NeverButton");
    static ::rtl::OUString KEY     = ::rtl::OUString::createFromAscii("Visible");

    ::com::sun::star::uno::Any aValue;
    try
    {
        aValue = ::comphelper::ConfigurationHelper::readDirectKey(
                                ::comphelper::getProcessServiceFactory(),
                                PACKAGE,
                                PATH,
                                KEY,
                                ::comphelper::ConfigurationHelper::E_READONLY);
    }
    catch(const ::com::sun::star::uno::Exception&)
        { aValue.clear(); }

    aValue >>= m_bNeverVisible;
}

void RegistrationPage::updateButtonStates()
{
    m_rbNever.Show( m_bNeverVisible );
}

sal_Bool RegistrationPage::commitPage( CommitPageReason _eReason )
{
    if ( _eReason == eFinish )
    {
        ::svt::RegOptions aOptions;
        rtl::OUString aEvent;

        if ( m_rbNow.IsChecked())
        {
            aEvent = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "RegistrationRequired" ) );
        }
        else if (m_rbLater.IsChecked())
        {
            aOptions.activateReminder(7);
            // avtivate a reminder job...
        }
        // aOptions.markSessionDone();

        try
        {
            // create the Desktop component which can load components
            Reference < XMultiServiceFactory > xFactory = ::comphelper::getProcessServiceFactory();
            if( xFactory.is() )
            {
                Reference< com::sun::star::task::XJobExecutor > xProductRegistration(
                    xFactory->createInstance( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.setup.ProductRegistration" ) ) ),
                    UNO_QUERY_THROW );

                 // tell it that the user wants to register
                 xProductRegistration->trigger( aEvent );
            }
        }
        catch( const Exception& )
        {
        }
    }
    return sal_True;
}

RegistrationPage::RegistrationMode RegistrationPage::getRegistrationMode() const
{
    RegistrationPage::RegistrationMode eMode = rmNow;
    if ( m_rbLater.IsChecked() )
        eMode = rmLater;
    else if ( m_rbNever.IsChecked() )
        eMode = rmNever;
    return eMode;
}

void RegistrationPage::prepareSingleMode()
{
    // remove wizard text (hide and cut)
    m_flSeparator.Hide();
    m_ftEnd.Hide();
    Size aNewSize = GetSizePixel();
    aNewSize.Height() -= ( aNewSize.Height() - m_flSeparator.GetPosPixel().Y() );
    SetSizePixel( aNewSize );
}

bool RegistrationPage::hasReminderDateCome()
{
    return ::svt::RegOptions().hasReminderDateCome();
}

void RegistrationPage::executeSingleMode()
{
    // opens the page in a single tabdialog
    SfxSingleTabDialog aSingleDlg( NULL, TP_REGISTRATION );
    RegistrationPage* pPage = new RegistrationPage( &aSingleDlg, WizardResId( TP_REGISTRATION ) );
    pPage->prepareSingleMode();
    aSingleDlg.SetPage( pPage );
    aSingleDlg.SetText( pPage->getSingleModeTitle() );
    aSingleDlg.Execute();
    // the registration modes "Now" and "Later" are handled by the page
    RegistrationPage::RegistrationMode eMode = pPage->getRegistrationMode();
    if ( eMode == RegistrationPage::rmNow || eMode == RegistrationPage::rmLater )
        pPage->commitPage( IWizardPage::eFinish );
    if ( eMode != RegistrationPage::rmLater )
        ::svt::RegOptions().removeReminder();
}

// -----------------------------------------------------------------------

static char const OEM_PRELOAD_SECTION[] = "Bootstrap";
static char const OEM_PRELOAD[]		= "Preload";
static char const STR_TRUE[]		= "1";
static char const STR_FALSE[]		= "0";

static sal_Bool existsURL( OUString const& _sURL )
{
    using namespace osl;
	DirectoryItem aDirItem;

    if (_sURL.getLength() != 0)
        return ( DirectoryItem::get( _sURL, aDirItem ) == DirectoryItem::E_None );

    return sal_False;
}


// locate soffice.ini/.rc file
static OUString locateIniFile()
{
	OUString aUserDataPath;
	OUString aSofficeIniFileURL;

	// Retrieve the default file URL for the soffice.ini/rc
	rtl::Bootstrap().getIniName( aSofficeIniFileURL );

	if ( utl::Bootstrap::locateUserData( aUserDataPath ) == utl::Bootstrap::PATH_EXISTS )
	{
		const char CONFIG_DIR[] = "/config";

		sal_Int32 nIndex = aSofficeIniFileURL.lastIndexOf( '/');
		if ( nIndex > 0 )
		{
			OUString		aUserSofficeIniFileURL;
			OUStringBuffer	aBuffer( aUserDataPath );
			aBuffer.appendAscii( CONFIG_DIR );
			aBuffer.append( aSofficeIniFileURL.copy( nIndex ));
			aUserSofficeIniFileURL = aBuffer.makeStringAndClear();

			if ( existsURL( aUserSofficeIniFileURL ))
				return aUserSofficeIniFileURL;
		}
	}
	// Fallback try to use the soffice.ini/rc from program folder
	return aSofficeIniFileURL;
}

// check whether the OEMPreload flag was set in soffice.ini/.rc
static sal_Int32 checkOEMPreloadFlag()
{
	OUString aSofficeIniFileURL;
	aSofficeIniFileURL = locateIniFile();
	Config aConfig(aSofficeIniFileURL);
	aConfig.SetGroup( OEM_PRELOAD_SECTION );
	ByteString sResult = aConfig.ReadKey( OEM_PRELOAD );
	return sResult.ToInt32();
	/*
	if ( sResult == STR_TRUE )
		return sal_True;
	else
		return sal_False;
	*/
}

WelcomePage::OEMType WelcomePage::checkOEM()
{
  sal_Int32 oemResult = checkOEMPreloadFlag();
  switch (oemResult) {
  case 1:
    return OEM_NORMAL;
  case 2:
    return OEM_EXTENDED;
  default:
    return OEM_NONE;
  }
}

} // namespace desktop

