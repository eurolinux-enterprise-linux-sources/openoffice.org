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

#ifndef _MSGBOX_HXX //autogen
#include <vcl/msgbox.hxx>
#endif
#include <tools/fsys.hxx>
#ifndef _SV_FILEDLG_HXX //autogen
#include <svtools/filedlg.hxx>
#endif
#include <tools/config.hxx>

#include <vcl/font.hxx>

#ifndef _BASIC_TTRESHLP_HXX
#include <basic/ttstrhlp.hxx>
#endif
#include <basic/sbx.hxx>
#include <svtools/filedlg.hxx>

#include <osl/module.h>

#include "basic.hrc"
#include "app.hxx"
#include "printer.hxx"
#include "status.hxx"
#include "appedit.hxx"
#include "appbased.hxx"
#include "apperror.hxx"
#include <basic/mybasic.hxx>
#include "ttbasic.hxx"
#include "dialogs.hxx"
#include <basic/basrdll.hxx>
#include "basrid.hxx"

#ifndef _RUNTIME_HXX
#include "runtime.hxx"
#endif
#include "sbintern.hxx"

#ifdef _USE_UNO
#include <ucbhelper/contentbroker.hxx>
#include <ucbhelper/configurationkeys.hxx>
#include <comphelper/regpathhelper.hxx>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <cppuhelper/bootstrap.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/ucb/XContentProviderManager.hpp>

#include <ucbhelper/content.hxx>
#include <svtools/syslocale.hxx>

using namespace comphelper;
using namespace cppu;
using namespace rtl;
using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::ucb;
using namespace com::sun::star::beans;

#endif /* _USE_UNO */

IMPL_GEN_RES_STR;

#ifdef DBG_UTIL
// filter Messages generated due to missing configuration  Bug:#83887#
void TestToolDebugMessageFilter( const sal_Char *pString, BOOL bIsOsl )
{
    static BOOL static_bInsideFilter = FALSE;

    // Ignore messages during filtering to avoid endless recursions
    if ( static_bInsideFilter )
        return;

    static_bInsideFilter = TRUE;

    ByteString aMessage( pString );

    BOOL bIgnore = FALSE;

    if ( bIsOsl )
    {
        // OSL
        if ( aMessage.Search( CByteString("Cannot open Configuration: Connector: unknown delegatee com.sun.star.connection.Connector.portal") ) != STRING_NOTFOUND )
            bIgnore = TRUE;
    }
    else
    {
        // DBG
#if ! (OSL_DEBUG_LEVEL > 1)
        if ( aMessage.Search( CByteString("SelectAppIconPixmap") ) != STRING_NOTFOUND )
            bIgnore = TRUE;
#endif
        if ( aMessage.Search( CByteString("PropertySetRegistry::") ) != STRING_NOTFOUND )
            bIgnore = TRUE;
        if ( aMessage.Search( CByteString("property value missing") ) != STRING_NOTFOUND )
            bIgnore = TRUE;
        if ( aMessage.Search( CByteString("getDateFormatsImpl") ) != STRING_NOTFOUND
            && aMessage.Search( CByteString("no date formats") ) != STRING_NOTFOUND )
            bIgnore = TRUE;
        if ( aMessage.Search( CByteString("ucb::configureUcb(): Bad arguments") ) != STRING_NOTFOUND )
            bIgnore = TRUE;
        if ( aMessage.Search( CByteString("CreateInstance with arguments exception") ) != STRING_NOTFOUND )
            bIgnore = TRUE;
        if ( aMessage.Search( CByteString("AcquireTree failed") ) != STRING_NOTFOUND )
            bIgnore = TRUE;
    }


    if ( bIgnore )
    {
        static_bInsideFilter = FALSE;
        return;
    }

    if ( bIsOsl )
    {
        // due to issue #i36895 only print on console
        // unfortunately the osl assertions deadlock by design :-( on recursive calls of assertions
        printf("%s\n", pString );
    }
    else
    {
        try
        {
            aBasicApp.DbgPrintMsgBox( pString );
        }
        catch ( ... )

        {
            printf("DbgPrintMsgBox failed: %s\n", pString );
        }
    }
/*    DBG_INSTOUTERROR( DBG_OUT_MSGBOX )
    DBG_ERROR( pString );
    DBG_INSTOUTERROR( DBG_OUT_TESTTOOL )*/
    static_bInsideFilter = FALSE;
}
void SAL_CALL DBG_TestToolDebugMessageFilter( const sal_Char *pString )
{
	    TestToolDebugMessageFilter( pString, FALSE );
}
extern "C" void SAL_CALL osl_TestToolDebugMessageFilter( const sal_Char *pString )
{
    if ( !getenv( "DISABLE_SAL_DBGBOX" ) )
	    TestToolDebugMessageFilter( pString, TRUE );
}
#endif

// #94145# Due to a tab in TT_SIGNATURE_FOR_UNICODE_TEXTFILES which is changed to blanks by some editors
// this routine became necessary
BOOL IsTTSignatureForUnicodeTextfile( String aLine )
{
    aLine.SearchAndReplace( '\t', ' ' );
    String ThreeBlanks = CUniString("   ");
    String TwoBlanks = CUniString("  ");
	while ( aLine.SearchAndReplace( ThreeBlanks, TwoBlanks ) != STRING_NOTFOUND )
    {}
    return aLine.EqualsAscii( TT_SIGNATURE_FOR_UNICODE_TEXTFILES );
}

BasicApp aBasicApp; // Application instance

static const char * const components[] =
{
    SAL_MODULENAME( "ucb1" )    // KSO, ABI
    , SAL_MODULENAME( "ucpfile1" )
    , "configmgr2.uno" SAL_DLLEXTENSION
    , "sax.uno" SAL_DLLEXTENSION
    , "stocservices.uno" SAL_DLLEXTENSION
    , SAL_MODULENAME( "fileacc" )
    , SAL_MODULENAME( "mcnttype" )  		// Clipboard   Ask Oliver Braun
    , "i18npool.uno" SAL_DLLEXTENSION
        // Reading of files in specific encodings like UTF-8 using
        // createUnoService( "com.sun.star.io.TextInputStream" ) and such
    , "textinstream.uno" SAL_DLLEXTENSION
    , "textoutstream.uno" SAL_DLLEXTENSION
    , "introspection.uno" SAL_DLLEXTENSION
    , "reflection.uno" SAL_DLLEXTENSION
        // RemoteUno
    , "connector.uno" SAL_DLLEXTENSION
    , "bridgefac.uno" SAL_DLLEXTENSION
    , "remotebridge.uno" SAL_DLLEXTENSION
#ifdef SAL_UNX
#ifdef QUARTZ
    , SVLIBRARY( "dtransaqua" )  // Mac OS X Aqua uses a dedicated libdtransaqua
#else
    , SVLIBRARY( "dtransX11" )        // OBR
#endif
#endif
#ifdef SAL_W32
    , SAL_MODULENAME( "sysdtrans" )
    , SAL_MODULENAME( "ftransl" )
    , SAL_MODULENAME( "dnd" )
#endif
    , 0
};

uno::Reference< XContentProviderManager > InitializeUCB( void )
{
    uno::Reference< XMultiServiceFactory > xSMgr;
    try
    {
        xSMgr = uno::Reference< XMultiServiceFactory >(
            defaultBootstrap_InitialComponentContext()->getServiceManager(),
            UNO_QUERY_THROW);
    }
    catch( com::sun::star::uno::Exception & exc )
    {
        fprintf( stderr, "Couldn't bootstrap uno servicemanager for reason : %s\n" ,
                 OUStringToOString( exc.Message, RTL_TEXTENCODING_ASCII_US ).getStr() );
        InfoBox( NULL, String( exc.Message ) ).Execute();
        throw ;
    }


	//////////////////////////////////////////////////////////////////////
	// set global factory
	setProcessServiceFactory( xSMgr );

/*	// Create simple ConfigManager
	Sequence< Any > aConfArgs(3);
	aConfArgs[0] <<= PropertyValue( OUString::createFromAscii("servertype"), 0, makeAny( OUString::createFromAscii("local") ), ::com::sun::star::beans::PropertyState_DIRECT_VALUE );
	aConfArgs[1] <<= PropertyValue( OUString::createFromAscii("sourcepath"), 0, makeAny( OUString::createFromAscii("g:\\") ), ::com::sun::star::beans::PropertyState_DIRECT_VALUE );
	aConfArgs[2] <<= PropertyValue( OUString::createFromAscii("updatepath"), 0, makeAny( OUString::createFromAscii("g:\\") ), ::com::sun::star::beans::PropertyState_DIRECT_VALUE );

	uno::Reference< XContentProvider > xConfProvider
		( xSMgr->createInstanceWithArguments( OUString::createFromAscii( "com.sun.star.configuration.ConfigurationProvider" ), aConfArgs), UNO_QUERY );
*/


//  Create unconfigured Ucb:
/*	Sequence< Any > aArgs(1);
	aArgs[1] = makeAny ( xConfProvider );*/
	Sequence< Any > aArgs;
	::ucbhelper::ContentBroker::initialize( xSMgr, aArgs );
    uno::Reference< XContentProviderManager > xUcb =
        ::ucbhelper::ContentBroker::get()->getContentProviderManagerInterface();

	uno::Reference< XContentProvider > xFileProvider
		( xSMgr->createInstance( OUString::createFromAscii( "com.sun.star.ucb.FileContentProvider" ) ), UNO_QUERY );
    xUcb->registerContentProvider( xFileProvider, OUString::createFromAscii( "file" ), sal_True );


/*	uno::Reference< XContentProvider > xPackageProvider
		( xSMgr->createInstance( OUString::createFromAscii( "com.sun.star.ucb.PackageContentProvider" ) ), UNO_QUERY );
    xUcb->registerContentProvider( xPackageProvider, OUString::createFromAscii( "vnd.sun.star.pkg" ), sal_True );
	*/

	return xUcb;
}

static void ReplaceStringHookProc( UniString& rStr )
{
    static String aTestToolName( RTL_CONSTASCII_USTRINGPARAM( "VCLTestTool" ) ); // HACK, should be read from ressources

    if ( rStr.SearchAscii( "%PRODUCT" ) != STRING_NOTFOUND )
    {
        rStr.SearchAndReplaceAllAscii( "%PRODUCTNAME", aTestToolName );
        /*
        rStr.SearchAndReplaceAllAscii( "%PRODUCTVERSION", rVersion );
        rStr.SearchAndReplaceAllAscii( "%ABOUTBOXPRODUCTVERSION", rAboutBoxVersion );
        rStr.SearchAndReplaceAllAscii( "%PRODUCTEXTENSION", rExtension );
        rStr.SearchAndReplaceAllAscii( "%PRODUCTXMLFILEFORMATNAME", rXMLFileFormatName );
        rStr.SearchAndReplaceAllAscii( "%PRODUCTXMLFILEFORMATVERSION", rXMLFileFormatVersion );
        */
    }
}

void BasicApp::Main( )
{
#ifdef DBG_UTIL
//  Install filter for OSLAsserts
    DbgPrintMsgBox = DbgGetPrintMsgBox();
	DbgSetPrintTestTool( DBG_TestToolDebugMessageFilter );
	DBG_INSTOUTERROR( DBG_OUT_TESTTOOL );

    if ( osl_setDebugMessageFunc( osl_TestToolDebugMessageFilter ) )
        DBG_ERROR("osl_setDebugMessageFunc returns non NULL pointer");
#endif

    ResMgr::SetReadStringHook( ReplaceStringHookProc );

    try
	{
#ifdef _USE_UNO
	uno::Reference< XContentProviderManager > xUcb = InitializeUCB();
#endif

    {
        DirEntry aIniPath( Config::GetConfigName( Config::GetDefDirectory(), CUniString("testtool") ) );
	    if ( !aIniPath.Exists() )
        {   // look for it besides the executable
            DirEntry aAppFileName( GetAppFileName() );
            String aAppDir ( aAppFileName.GetPath().GetFull() );

//            DirEntry aDefIniPath( Config::GetConfigName( aAppDir, CUniString("testtool") ) );
//            Do not use Config::GetConfigName here because is uses a hidden file for UNIX

            DirEntry aDefIniPath( aAppDir );
            ByteString aFileName;
#ifdef UNX
            aFileName = "testtoolrc";
#else
            aFileName = "testtool.ini";
#endif
            aDefIniPath += DirEntry( aFileName );

            if ( aDefIniPath.Exists() )
            {
                aDefIniPath.CopyTo( aIniPath, FSYS_ACTION_COPYFILE );
                FileStat::SetReadOnlyFlag( aIniPath, FALSE );
            }
        }
    }

    {
		LanguageType aRequestedLanguage;
		Config aConf(Config::GetConfigName( Config::GetDefDirectory(), CUniString("testtool") ));

		// 1033 = LANGUAGE_ENGLISH_US
		// 1031 = LANGUAGE_GERMAN
		aConf.SetGroup("Misc");
		ByteString aLang = aConf.ReadKey( "Language", ByteString::CreateFromInt32( LANGUAGE_SYSTEM ) );
		aRequestedLanguage = LanguageType( aLang.ToInt32() );

		AllSettings aSettings = GetSettings();
        aSettings.SetUILanguage( aRequestedLanguage );
        aSettings.SetLanguage( aRequestedLanguage );
//		International aInternational;
//		aInternational = GetSettings().GetInternational();
//		aInternational = International( aRequestedLanguage );
//		aSettings.SetInternational( aInternational );
		SetSettings( aSettings );
//		aInternational = GetSettings().GetInternational();
	}

//	ResMgr::CreateResMgr( CREATEVERSIONRESMGR( stt ),  )
//const char* ResMgr::GetLang( LanguageType& nType, USHORT nPrio )

//	ResMgr::CreateResMgr( CREATEVERSIONRESMGR( stt )
//	ResMgr *pRes = new ResMgr( "testtool.res" );
//	Resource::SetResManager( pRes );

	BasicDLL aBasicDLL;
	nWait = 0;

	// Hilfe:
//	pHelp = new Help;
//	SetHelp( pHelp );
//	Help::EnableContextHelp();
//	Help::EnableExtHelp();
//	DeactivateExtHelp();

	// Acceleratoren
	Accelerator aAccel( SttResId( MAIN_ACCEL ) );
	InsertAccel( &aAccel );
	pMainAccel = &aAccel;

	// Frame Window:
	pFrame = new BasicFrame;
	aAccel.SetSelectHdl( LINK( pFrame, BasicFrame, Accel ) );

	pFrame->Show();

	SetSystemWindowMode( SYSTEMWINDOW_MODE_NOAUTOMODE );
	SetSystemWindowMode( SYSTEMWINDOW_MODE_DIALOG );

    // Instantiate a SvtSysLocale to avoid permant instatiation
    // and deletion of SvtSysLocale_Impl in SvtSysLocale Ctor/Dtor
    // because in the testtool szenario Basic is the only instance
    // instatiating SvtSysLocale (#107417).
    SvtSysLocale aSysLocale;

	PostUserEvent( LINK( this, BasicApp, LateInit ) );
	Execute();

//	delete pHelp;
	delete pFrame;

	RemoveAccel( pMainAccel );

	}
	catch( class Exception & rEx)
	{
		printf( "Exception not caught: %s\n", ByteString( String(rEx.Message), RTL_TEXTENCODING_ASCII_US ).GetBuffer() );
        String aMsg( String::CreateFromAscii( "Exception not caught: " ) );
        aMsg.Append( String( rEx.Message ) );
		InfoBox( NULL, aMsg ).Execute();
		throw;
	}
	catch( ... )
	{
		printf( "unknown Exception not caught\n" );
        InfoBox( NULL, String::CreateFromAscii( "unknown Exception not caught" ) ).Execute();
		throw;
	}
}

void BasicApp::LoadIniFile()
{
	pFrame->LoadIniFile();
}

void BasicApp::SetFocus()
{
	if( pFrame->pWork && pFrame->pWork->ISA(AppEdit) )
	  ((AppEdit*)pFrame->pWork)->pDataEdit->GrabFocus();
}

IMPL_LINK( BasicApp, LateInit, void *, pDummy )
{
    (void) pDummy; /* avoid warning about unused parameter */ 
    USHORT i;
	for ( i = 0 ; i < Application::GetCommandLineParamCount() ; i++ )
	{
		if ( Application::GetCommandLineParam( i ).Copy(0,4).CompareIgnoreCaseToAscii("-run") == COMPARE_EQUAL
#ifndef UNX
            || Application::GetCommandLineParam( i ).Copy(0,4).CompareIgnoreCaseToAscii("/run") == COMPARE_EQUAL
#endif
            )
			pFrame->SetAutoRun( TRUE );
        else if ( Application::GetCommandLineParam( i ).Copy(0,7).CompareIgnoreCaseToAscii("-result") == COMPARE_EQUAL
#ifndef UNX
            || Application::GetCommandLineParam( i ).Copy(0,7).CompareIgnoreCaseToAscii("/result") == COMPARE_EQUAL
#endif
            )
        {
            if ( (i+1) < Application::GetCommandLineParamCount() )
            {
                if ( ByteString( Application::GetCommandLineParam( i+1 ), osl_getThreadTextEncoding() ).IsNumericAscii() )
                {
                    MsgEdit::SetMaxLogLen( sal::static_int_cast< USHORT >( Application::GetCommandLineParam( i+1 ).ToInt32() ) );
                }
                i++;
            }
        }
	}

    // now load the files after the switches have been set. Espechially -run is of interest sunce it changes the behavior
    for ( i = 0 ; i < Application::GetCommandLineParamCount() ; i++ )
	{
		if ( Application::GetCommandLineParam( i ).Copy(0,1).CompareToAscii("-") != COMPARE_EQUAL
#ifndef UNX
		  && Application::GetCommandLineParam( i ).Copy(0,1).CompareToAscii("/") != COMPARE_EQUAL
#endif
          )
		{
			pFrame->LoadFile( Application::GetCommandLineParam( i ) );
		}
        else if ( Application::GetCommandLineParam( i ).Copy(0,7).CompareIgnoreCaseToAscii("-result") == COMPARE_EQUAL
#ifndef UNX
            || Application::GetCommandLineParam( i ).Copy(0,7).CompareIgnoreCaseToAscii("/result") == COMPARE_EQUAL
#endif
            )
        {   // Increment count to skip the parameter. This works even if it is not given
            i++;
        }
	}

	pFrame->pStatus->SetStatusSize( pFrame->pStatus->GetStatusSize()+1 );
	pFrame->pStatus->SetStatusSize( pFrame->pStatus->GetStatusSize()-1 );

	if ( pFrame->IsAutoRun() )
	{
		pFrame->Command( RID_RUNSTART );
	}

	if ( pFrame->IsAutoRun() )
		pFrame->Command( RID_QUIT );

	return 0;
}

//////////////////////////////////////////////////////////////////////////

class FloatingExecutionStatus : public FloatingWindow
{
public:
	FloatingExecutionStatus( Window * pParent );
	void SetStatus( String aW );
	void SetAdditionalInfo( String aF );

private:
	Timer aAusblend;
	DECL_LINK(HideNow, FloatingExecutionStatus* );
	FixedText aStatus;
	FixedText aAdditionalInfo;
};


FloatingExecutionStatus::FloatingExecutionStatus( Window * pParent )
	: FloatingWindow( pParent, SttResId(LOAD_CONF) ),
	aStatus( this, SttResId( WORK ) ),
	aAdditionalInfo( this, SttResId( FILENAME ) )
{
	FreeResource();
	aAusblend.SetTimeoutHdl( LINK(this, FloatingExecutionStatus, HideNow ) );
	aAusblend.SetTimeout(5000);             // in ms
	aAusblend.Start();
}

void FloatingExecutionStatus::SetStatus( String aW )
{
	Show( TRUE, SHOW_NOFOCUSCHANGE | SHOW_NOACTIVATE );
	ToTop( TOTOP_NOGRABFOCUS );
	aAusblend.Start();
	aStatus.SetText( aW );
}

void FloatingExecutionStatus::SetAdditionalInfo( String aF )
{
	Show( TRUE, SHOW_NOFOCUSCHANGE | SHOW_NOACTIVATE );
	ToTop( TOTOP_NOGRABFOCUS );
	aAusblend.Start();
	aAdditionalInfo.SetText( aF );
}

IMPL_LINK(FloatingExecutionStatus, HideNow, FloatingExecutionStatus*, pFLC )
{
    (void) pFLC; /* avoid warning about unused parameter */ 
    Hide();
    return 0;
}

//////////////////////////////////////////////////////////////////////////

TYPEINIT1(TTExecutionStatusHint, SfxSimpleHint);

BasicFrame::BasicFrame() : WorkWindow( NULL,
	WinBits( WB_APP | WB_MOVEABLE | WB_SIZEABLE | WB_CLOSEABLE ) )
, bIsAutoRun( FALSE )
, pDisplayHidDlg( NULL )
, pEditVar ( 0 )
, bAutoReload( FALSE )
, bAutoSave( TRUE )
, pBasic( NULL )
, pExecutionStatus( NULL )
, pStatus( NULL )
, pList( NULL )
, pWork( NULL )
, pPrn( NULL )
{

	Application::SetDefDialogParent( this );
    AlwaysEnableInput( TRUE );
	pBasic  = TTBasic::CreateMyBasic();		// depending on what was linked to the executable
	bInBreak = FALSE;
	bDisas = FALSE;
	nFlags  = 0;
//	Icon aAppIcon;

	if ( pBasic->pTestObject )	// Are we the testtool?
	{
//		aAppIcon = Icon( ResId( RID_APPICON2 ) );
		aAppName = String( SttResId( IDS_APPNAME2 ) );
	}
	else
	{
//		aAppIcon = Icon( ResId( RID_APPICON ) );
		aAppName = String( SttResId( IDS_APPNAME ) );
	}

	// Menu:
	MenuBar *pBar = new MenuBar( SttResId( RID_APPMENUBAR ) );
	SetMenuBar( pBar );

	pBar->SetHighlightHdl( LINK( this, BasicFrame, HighlightMenu ) );


	// Menu Handler:
	PopupMenu* pFileMenu = pBar->GetPopupMenu( RID_APPFILE );
	pFileMenu->SetSelectHdl( LINK( this, BasicFrame, MenuCommand ) );
	pFileMenu->SetHighlightHdl( LINK( this, BasicFrame, HighlightMenu ) );
	pFileMenu->SetActivateHdl( LINK( this, BasicFrame, InitMenu ) );
	pFileMenu->SetDeactivateHdl( LINK( this, BasicFrame, DeInitMenu ) );
	if (Basic().pTestObject )		// Are we TestTool?
	{
		pFileMenu->RemoveItem( pFileMenu->GetItemPos( RID_FILELOADLIB ) -1 );	// Separator before
		pFileMenu->RemoveItem( pFileMenu->GetItemPos( RID_FILELOADLIB ) );
		pFileMenu->RemoveItem( pFileMenu->GetItemPos( RID_FILESAVELIB ) );
	}

	PopupMenu* pEditMenu = pBar->GetPopupMenu( RID_APPEDIT );
	pEditMenu->SetSelectHdl( LINK( this, BasicFrame, MenuCommand ) );
	pEditMenu->SetHighlightHdl( LINK( this, BasicFrame, HighlightMenu ) );
	pEditMenu->SetActivateHdl( LINK( this, BasicFrame, InitMenu ) );
	pEditMenu->SetDeactivateHdl( LINK( this, BasicFrame, DeInitMenu ) );
	PopupMenu* pRunMenu = pBar->GetPopupMenu( RID_APPRUN );
	pRunMenu->SetSelectHdl( LINK( this, BasicFrame, MenuCommand ) );
	pRunMenu->SetHighlightHdl( LINK( this, BasicFrame, HighlightMenu ) );
	pRunMenu->SetActivateHdl( LINK( this, BasicFrame, InitMenu ) );
	pRunMenu->SetDeactivateHdl( LINK( this, BasicFrame, DeInitMenu ) );
	if (Basic().pTestObject )		// Are we TestTool?
	{
		pRunMenu->RemoveItem( pRunMenu->GetItemPos( RID_RUNDISAS ) );
		pRunMenu->RemoveItem( pRunMenu->GetItemPos( RID_RUNCOMPILE ) );
	}

	PopupMenu *pExtras;
	if (Basic().pTestObject )		// Are we TestTool?
	{
		pExtras = new PopupMenu( SttResId( RID_TT_EXTRAS ) );
		pBar->InsertItem( RID_TT_EXTRAS, String( SttResId( RID_TT_EXTRAS_NAME ) ), 0, pBar->GetItemPos( RID_APPWINDOW ) );
		pBar->SetPopupMenu( RID_TT_EXTRAS, pExtras );

		pExtras->SetSelectHdl( LINK( this, BasicFrame, MenuCommand ) );
		pExtras->SetHighlightHdl( LINK( this, BasicFrame, HighlightMenu ) );
		pExtras->SetDeactivateHdl( LINK( this, BasicFrame, DeInitMenu ) );
	}

	PopupMenu* pWinMenu = pBar->GetPopupMenu( RID_APPWINDOW );
	pWinMenu->SetSelectHdl( LINK( this, BasicFrame, MenuCommand ) );
	pWinMenu->SetHighlightHdl( LINK( this, BasicFrame, HighlightMenu ) );
	pWinMenu->SetDeactivateHdl( LINK( this, BasicFrame, DeInitMenu ) );
	PopupMenu* pHelpMenu = pBar->GetPopupMenu( RID_APPHELP );
	pHelpMenu->SetSelectHdl( LINK( this, BasicFrame, MenuCommand ) );
	pHelpMenu->SetHighlightHdl( LINK( this, BasicFrame, HighlightMenu ) );
	pHelpMenu->SetActivateHdl( LINK( this, BasicFrame, InitMenu ) );
	pHelpMenu->SetDeactivateHdl( LINK( this, BasicFrame, DeInitMenu ) );

#ifndef UNX
	pPrn    = new BasicPrinter;
#else
	pPrn    = NULL;
#endif
	pList   = new EditList;
	pStatus = new StatusLine( this );

	LoadIniFile();

	UpdateTitle();
//	SetIcon( aAppIcon );

	// Size: half width, 0.75 * height - 2 * IconSize
	{
		Config aConf(Config::GetConfigName( Config::GetDefDirectory(), CUniString("testtool") ));
		aConf.SetGroup("WinGeom");
        SetWindowState( aConf.ReadKey("WinParams", "") );
	}

//	pWork = new AppEdit( this, NULL );
//	pWork->Show();
//	pWork->Close();

	aLineNum.SetTimeoutHdl( LINK( this, BasicFrame, ShowLineNr ) );
	aLineNum.SetTimeout(200);
	aLineNum.Start();


	aCheckFiles.SetTimeout( 10000 );
	aCheckFiles.SetTimeoutHdl( LINK( this, BasicFrame, CheckAllFiles ) );
	aCheckFiles.Start();

	GetMenuBar()->SetCloserHdl( LINK( this, BasicFrame, CloseButtonClick ) );
	GetMenuBar()->SetFloatButtonClickHdl( LINK( this, BasicFrame, FloatButtonClick ) );
	GetMenuBar()->SetHideButtonClickHdl( LINK( this, BasicFrame, HideButtonClick ) );
}

const ByteString ProfilePrefix("_profile_");
const USHORT ProfilePrefixLen = ProfilePrefix.Len();

void BasicFrame::LoadIniFile()
{
    USHORT i;
	Config aConf(Config::GetConfigName( Config::GetDefDirectory(), CUniString("testtool") ));

	for ( i = 0 ; i < aConf.GetGroupCount() ; i++ )
	{
		aConf.SetGroup( ByteString( aConf.GetGroupName( i ) ) );
		if ( ( aConf.ReadKey( "Aktuell" ).Len() || aConf.ReadKey( "Alle" ).Len() )
		   &&( !aConf.ReadKey( "Current" ).Len() && !aConf.ReadKey( "All" ).Len() ) )
		{
            aConf.WriteKey( "Current", aConf.ReadKey( "Aktuell" ) );
            aConf.WriteKey( "All", aConf.ReadKey( "Alle" ) );
		}
	}

	aConf.SetGroup("Misc");
	ByteString aTemp;
    ByteString aCurrentProfile = aConf.ReadKey( "CurrentProfile", "Misc" );

    pStatus->SetProfileName( String( aCurrentProfile.Copy( ProfilePrefixLen ), RTL_TEXTENCODING_UTF8 ) );

    aConf.SetGroup( aCurrentProfile );
    aTemp = aConf.ReadKey( "AutoReload", "0" );
	bAutoReload = ( aTemp.CompareTo("1") == COMPARE_EQUAL );
	aTemp = aConf.ReadKey( "AutoSave", "0" );
	bAutoSave = ( aTemp.CompareTo("1") == COMPARE_EQUAL );

    LoadLRU();

	if ( pBasic )
		pBasic->LoadIniFile();

	for ( i = 0 ; i < pList->Count() ; i++ )
		pList->GetObject( i )->LoadIniFile();
}

BasicFrame::~BasicFrame()
{
	AppWin* p = pList->First();
	DBG_ASSERT( !p, "Still open FileWindows");
	if( p )
		while( (p = pList->Remove() ) != NULL )
			delete p;

	MenuBar *pBar = GetMenuBar();
	SetMenuBar( NULL );
	delete pBar;

	delete pStatus;
	delete pPrn;
	delete pList;
//	delete pExecutionStatus;
//	delete pBasic;
	pBasic.Clear();
}

void BasicFrame::Command( const CommandEvent& rCEvt ) 
{
	switch( rCEvt.GetCommand() ) {
		case COMMAND_SHOWDIALOG:
			{
                const CommandDialogData* pData = rCEvt.GetDialogData();
                if ( pData)
                {
                    const int nCommand = pData->GetDialogId();

                    switch (nCommand)
                    {
                        case SHOWDIALOG_ID_PREFERENCES :
                                Command( RID_OPTIONS );
                                break;

                        case SHOWDIALOG_ID_ABOUT :
                                Command( RID_HELPABOUT );
                                break;

                        default :
                                ;
                    }
                }
            }
            break;
	}
}

void BasicFrame::UpdateTitle()
{
	String aTitle;
	aTitle += aAppName;
	if ( aAppMode.Len() )
	{
		aTitle.AppendAscii(" [");
		aTitle += aAppMode;
		aTitle.AppendAscii("]");
	}
	aTitle.AppendAscii(" - ");
	aTitle += aAppFile;
	SetText( aTitle );
}

IMPL_LINK( BasicFrame, CheckAllFiles, Timer*, pTimer )
{
	if ( pWork )
	{
		AppWin* pStartWin = pWork;
		Window* pFocusWin = Application::GetFocusWindow();
		for ( int i = pList->Count()-1 ; i >= 0 ; i-- )
			pList->GetObject( i )->CheckReload();

		if ( pWork != pStartWin )
		{
			pWork = pStartWin;
			pWork->ToTop();
		}
		if ( pFocusWin )
			pFocusWin->GrabFocus();
	}
	pTimer->Start();
	return 0;
}

BOOL BasicFrame::IsAutoRun()
{
	return bIsAutoRun;
}

void BasicFrame::SetAutoRun( BOOL bAuto )
{
	bIsAutoRun = bAuto;
}

void BasicFrame::Notify( SfxBroadcaster&, const SfxHint& rHint )
{
	if ( rHint.ISA( TTExecutionStatusHint ) )
	{
		TTExecutionStatusHint *pStatusHint = ( TTExecutionStatusHint* )&rHint;
		switch ( pStatusHint->GetType() )
		{
			case TT_EXECUTION_ENTERWAIT:
				{
					EnterWait();
				}
				break;
			case TT_EXECUTION_LEAVEWAIT:
				{
					LeaveWait();
				}
				break;
			case TT_EXECUTION_SHOW_ACTION:
				{
                    String aTotalStatus( pStatusHint->GetExecutionStatus() );
                    aTotalStatus.AppendAscii( " " );
                    aTotalStatus.Append( pStatusHint->GetAdditionalExecutionStatus() );
                    pStatus->Message( aTotalStatus );
/*					if ( !pExecutionStatus )
						pExecutionStatus = new FloatingExecutionStatus( this );
					pExecutionStatus->SetStatus( pStatusHint->GetExecutionStatus() );
					pExecutionStatus->SetAdditionalInfo( pStatusHint->GetAdditionalExecutionStatus() );*/
				}
				break;
			case TT_EXECUTION_HIDE_ACTION:
				{
/*					if ( pExecutionStatus )
                    {
						delete pExecutionStatus;
					    pExecutionStatus = NULL;
                    }*/
				}
				break;
		}
	}


	Broadcast( rHint );
}

void BasicFrame::Resize()
{
	Config aConf(Config::GetConfigName( Config::GetDefDirectory(), CUniString("testtool") ));
	aConf.SetGroup("WinGeom");
    aConf.WriteKey("WinParams",GetWindowState());

	// Statusbar
	Size aOutSize = GetOutputSizePixel();
	Size aStatusSize = pStatus->GetSizePixel();
	Point aStatusPos( 0, aOutSize.Height() - aStatusSize.Height() );
	aStatusSize.Width() = aOutSize.Width();

	pStatus->SetPosPixel( aStatusPos );
	pStatus->SetSizePixel( aStatusSize );


	// Resize possibly maximized window
	ULONG i;
	for( i = pList->Count(); i > 0 ; i-- )
	{
		if ( pList->GetObject( i-1 )->GetWinState() == TT_WIN_STATE_MAX )
			pList->GetObject( i-1 )->Maximize();
	}
}

void BasicFrame::Move()
{
	Config aConf(Config::GetConfigName( Config::GetDefDirectory(), CUniString("testtool") ));
	aConf.SetGroup("WinGeom");
    aConf.WriteKey("WinParams",GetWindowState());
}

void BasicFrame::GetFocus()
{
    if ( pWork )
        pWork->GrabFocus();
}

IMPL_LINK( BasicFrame, CloseButtonClick, void*, EMPTYARG )
{
	AppWin* p;
	for ( p = pList->Last() ; p && p->GetWinState() != TT_WIN_STATE_MAX ; p = pList->Prev() )
	{};
	if ( p )
		p->GrabFocus();
	return Command( RID_FILECLOSE, FALSE );
}

IMPL_LINK( BasicFrame, FloatButtonClick, void*, EMPTYARG )
{
	AppWin* p;
	for ( p = pList->Last() ; p && p->GetWinState() != TT_WIN_STATE_MAX ; p = pList->Prev() )
	{};
	if ( p )
		p->TitleButtonClick( TITLE_BUTTON_DOCKING );
	return 1;
}

IMPL_LINK( BasicFrame, HideButtonClick, void*, EMPTYARG )
{
	AppWin* p;
	for ( p = pList->Last() ; p && p->GetWinState() != TT_WIN_STATE_MAX ; p = pList->Prev() )
	{};
	if ( p )
		p->TitleButtonClick( TITLE_BUTTON_HIDE );
	return 1;
}

void BasicFrame::WinShow_Hide()
{
    if ( !pList->Count() )
        return;

    AppWin* p;
    BOOL bWasFullscreen = FALSE;
    for ( p = pList->Last() ; p ; p = pList->Prev() )
    {
        if ( p->pDataEdit )
        {
            if ( p->GetWinState() & TT_WIN_STATE_HIDE	// Hidden
                 || ( bWasFullscreen && ( !p->IsPined() || p->GetWinState() & TT_WIN_STATE_MAX ))
               )
                p->Hide( SHOW_NOFOCUSCHANGE | SHOW_NOACTIVATE );
            else
                p->Show( TRUE, SHOW_NOFOCUSCHANGE | SHOW_NOACTIVATE );
        }
        bWasFullscreen |= p->GetWinState() == TT_WIN_STATE_MAX;
    }
}

void BasicFrame::WinMax_Restore()
{
    // The application buttons
	AppWin* p;
	BOOL bHasFullscreenWin = FALSE;
	for( p = pList->First(); p && !bHasFullscreenWin ; p = pList->Next() )
		bHasFullscreenWin |= ( p->GetWinState() == TT_WIN_STATE_MAX );
	GetMenuBar()->ShowButtons( bHasFullscreenWin, FALSE, FALSE );
	WinShow_Hide();
}

void BasicFrame::RemoveWindow( AppWin *pWin )
{
//	delete pIcon;
	pList->Remove( pWin );
	pWork = pList->Last();

	WinShow_Hide();

	if ( pWork )
		pWork->ToTop();

	WinMax_Restore();

	Menu* pMenu = GetMenuBar();
	if( pList->Count() == 0 ) {
		pMenu->EnableItem( RID_APPEDIT,   FALSE );
		pMenu->EnableItem( RID_APPRUN,	  FALSE );
		pMenu->EnableItem( RID_APPWINDOW, FALSE );
	}

	PopupMenu* pWinMenu = pMenu->GetPopupMenu( RID_APPWINDOW );

	pWinMenu->RemoveItem( pWinMenu->GetItemPos( pWin->GetWinId() ) );

    // Remove separator
	if ( pWinMenu->GetItemType( pWinMenu->GetItemCount() - 1 ) == MENUITEM_SEPARATOR )
		pWinMenu->RemoveItem( pWinMenu->GetItemCount() - 1 );

	pStatus->LoadTaskToolBox();
}

void BasicFrame::AddWindow( AppWin *pWin )
{
	pList->Insert( pWin, LIST_APPEND );
	pWork = pWin;

	WinMax_Restore();

    // Enable main menu
	MenuBar* pMenu = GetMenuBar();
	if( pList->Count() > 0 ) {
		pMenu->EnableItem( RID_APPEDIT,   TRUE );
		pMenu->EnableItem( RID_APPRUN,	  TRUE );
		pMenu->EnableItem( RID_APPWINDOW, TRUE );
	}

	PopupMenu* pWinMenu = pMenu->GetPopupMenu( RID_APPWINDOW );
	USHORT nLastID = pWinMenu->GetItemId( pWinMenu->GetItemCount() - 1 );

    // Separator necessary
	if ( nLastID < RID_WIN_FILE1 && pWinMenu->GetItemType( pWinMenu->GetItemCount() - 1 ) != MENUITEM_SEPARATOR )
		pWinMenu->InsertSeparator();

    // Find free ID
	USHORT nFreeID = RID_WIN_FILE1;
	while ( pWinMenu->GetItemPos( nFreeID ) != MENU_ITEM_NOTFOUND && nFreeID < RID_WIN_FILEn )
		nFreeID++;

	pWin->SetWinId( nFreeID );
	pWinMenu->InsertItem( nFreeID, pWin->GetText() );
}

void BasicFrame::WindowRenamed( AppWin *pWin )
{
	MenuBar* pMenu = GetMenuBar();
	PopupMenu* pWinMenu = pMenu->GetPopupMenu( RID_APPWINDOW );

	pWinMenu->SetItemText( pWin->GetWinId(), pWin->GetText() );

	pStatus->LoadTaskToolBox();

	aAppFile = pWin->GetText();
	UpdateTitle();
}

void BasicFrame::FocusWindow( AppWin *pWin )
{
	pWork = pWin;
	pList->Remove( pWin );
	pList->Insert( pWin, LIST_APPEND );
	pWin->Minimize( FALSE );

	aAppFile = pWin->GetText();
	UpdateTitle();

	WinShow_Hide();
	pStatus->LoadTaskToolBox();
}

BOOL BasicFrame::Close()
{
	if( bInBreak || Basic().IsRunning() )
		if( RET_NO == QueryBox( this, SttResId( IDS_RUNNING ) ).Execute() )
			return FALSE;

	StarBASIC::Stop();
	bInBreak = FALSE;
	if( CloseAll() )
	{
		aLineNum.Stop();

        // Close remaining dialogs to avoid assertions
		while ( GetWindow( WINDOW_OVERLAP )->GetWindow( WINDOW_FIRSTOVERLAP ) )
		{
			delete GetWindow( WINDOW_OVERLAP )->GetWindow( WINDOW_FIRSTOVERLAP )->GetWindow( WINDOW_CLIENT );
		}

		Application::SetDefDialogParent( NULL );
		WorkWindow::Close();

		return TRUE;
	} else return FALSE;
}

BOOL BasicFrame::CloseAll()
{
	while ( pList->Count() )
		if ( !pList->Last()->Close() )
			return FALSE;
	return TRUE;
}

BOOL BasicFrame::CompileAll()
{
	AppWin* p;
	for( p = pList->First(); p; p = pList->Next() )
	  if( p->ISA(AppBasEd) && !((AppBasEd*)p)->Compile() ) return FALSE;
	return TRUE;
}

// Setup menu
#define MENU2FILENAME( Name ) Name.Copy( Name.SearchAscii(" ") +1).EraseAllChars( '~' )
#define LRUNr( nNr ) CByteString("LRU").Append( ByteString::CreateFromInt32( nNr ) )
String FILENAME2MENU( USHORT nNr, String aName )
{
    String aRet;
    if ( nNr <= 9 )
        aRet = CUniString("~").Append( UniString::CreateFromInt32( nNr ) );
    else if ( nNr == 10 )
        aRet = CUniString("1~0");
    else
        aRet = UniString::CreateFromInt32( nNr );

    return aRet.AppendAscii(" ").Append( aName );
}

void BasicFrame::AddToLRU(String const& aFile)
{
	Config aConfig(Config::GetConfigName( Config::GetDefDirectory(), CUniString("testtool") ));
	PopupMenu *pPopup  = GetMenuBar()->GetPopupMenu(RID_APPFILE);

	aConfig.SetGroup("LRU");
    USHORT nMaxLRU = (USHORT)aConfig.ReadKey("MaxLRU","4").ToInt32();
	DirEntry aFileEntry( aFile );
	USHORT i,nLastMove = nMaxLRU;

	for ( i = 1 ; i<nMaxLRU && nLastMove == nMaxLRU ; i++ )
	{
		if ( DirEntry( UniString( aConfig.ReadKey(LRUNr(i),""), RTL_TEXTENCODING_UTF8 ) ) == aFileEntry )
			nLastMove = i;
	}

	if ( pPopup->GetItemPos( IDM_FILE_LRU1 ) == MENU_ITEM_NOTFOUND )
		pPopup->InsertSeparator();
	for ( i = nLastMove ; i>1 ; i-- )
	{
        if ( aConfig.ReadKey(LRUNr(i-1),"").Len() )
        {
		    aConfig.WriteKey(LRUNr(i), aConfig.ReadKey(LRUNr(i-1),""));
            if ( pPopup->GetItemPos( IDM_FILE_LRU1 + i-1 ) == MENU_ITEM_NOTFOUND )
 			    pPopup->InsertItem(IDM_FILE_LRU1 + i-1, FILENAME2MENU( i, MENU2FILENAME( pPopup->GetItemText(IDM_FILE_LRU1 + i-1-1) ) ));
            else
    		    pPopup->SetItemText(IDM_FILE_LRU1 + i-1,FILENAME2MENU( i, MENU2FILENAME( pPopup->GetItemText(IDM_FILE_LRU1 + i-1-1) ) ));
        }
	}
	aConfig.WriteKey(LRUNr(1), ByteString( aFile, RTL_TEXTENCODING_UTF8 ) );
    if ( pPopup->GetItemPos( IDM_FILE_LRU1 ) == MENU_ITEM_NOTFOUND )
 		pPopup->InsertItem(IDM_FILE_LRU1,FILENAME2MENU( 1, aFile));
    else
    	pPopup->SetItemText(IDM_FILE_LRU1,FILENAME2MENU( 1, aFile));
}

void BasicFrame::LoadLRU()
{
	Config     aConfig(Config::GetConfigName( Config::GetDefDirectory(), CUniString("testtool") ));
	PopupMenu *pPopup  = GetMenuBar()->GetPopupMenu(RID_APPFILE);
	BOOL       bAddSep = TRUE;

	aConfig.SetGroup("LRU");
    USHORT nMaxLRU = (USHORT)aConfig.ReadKey("MaxLRU","4").ToInt32();

	if ( pPopup )
        bAddSep = pPopup->GetItemPos( IDM_FILE_LRU1 ) == MENU_ITEM_NOTFOUND;

    USHORT i;
	for ( i = 1; i <= nMaxLRU && pPopup != NULL; i++)
	{
		String aFile = UniString( aConfig.ReadKey(LRUNr(i)), RTL_TEXTENCODING_UTF8 );

		if (aFile.Len() != 0)
		{
			if (bAddSep)
			{
				pPopup->InsertSeparator();
				bAddSep = FALSE;
			}

			if ( pPopup->GetItemPos( IDM_FILE_LRU1 + i-1 ) == MENU_ITEM_NOTFOUND )
                pPopup->InsertItem(IDM_FILE_LRU1 + i-1, FILENAME2MENU( i, aFile ));
            else
                pPopup->SetItemText(IDM_FILE_LRU1 + i-1, FILENAME2MENU( i, aFile ));
		}
	}
    i = nMaxLRU+1;
    while ( pPopup->GetItemPos( IDM_FILE_LRU1 + i-1 ) != MENU_ITEM_NOTFOUND )
    {
        pPopup->RemoveItem( pPopup->GetItemPos( IDM_FILE_LRU1 + i-1 ) );
        i++;
    }
}

IMPL_LINK( BasicFrame, InitMenu, Menu *, pMenu )
{
	BOOL bNormal = BOOL( !bInBreak );
	pMenu->EnableItem( RID_RUNCOMPILE, bNormal );

	BOOL bHasEdit = BOOL( /*bNormal &&*/ pWork != NULL );

//	pMenu->EnableItem( RID_FILENEW,		bNormal );	// always possible
//	pMenu->EnableItem( RID_FILEOPEN,	bNormal );
	pMenu->EnableItem( RID_FILECLOSE,	bHasEdit );
	pMenu->EnableItem( RID_FILESAVE,	bHasEdit );
	pMenu->EnableItem( RID_FILESAVEAS,	bHasEdit );
	pMenu->EnableItem( RID_FILEPRINT, 	bHasEdit );
	pMenu->EnableItem( RID_FILESETUP,	bHasEdit );
	pMenu->EnableItem( RID_FILELOADLIB,	bNormal );
	pMenu->EnableItem( RID_FILESAVELIB,	bHasEdit );

	BOOL bHasErr = BOOL( bNormal && pBasic->GetErrors() != 0 );
	BOOL bNext   = bHasErr & bNormal;
	BOOL bPrev   = bHasErr & bNormal;
    if( bHasErr ) 
    {
        ULONG n = pBasic->aErrors.GetCurPos();
        if( n == 0 )
            bPrev = FALSE;
        if( USHORT(n+1) == pBasic->GetErrors() ) 
            bNext = FALSE;
    }
    pMenu->EnableItem( RID_RUNNEXTERR, bNext );
    pMenu->EnableItem( RID_RUNPREVERR, bPrev );
    pMenu->CheckItem( RID_RUNDISAS, bDisas );
    if( pWork ) 
        pWork->InitMenu( pMenu );

    return TRUE;
}

IMPL_LINK_INLINE_START( BasicFrame, DeInitMenu, Menu *, pMenu )
{
    (void) pMenu; /* avoid warning about unused parameter */ 
/*	pMenu->EnableItem( RID_RUNCOMPILE );

	pMenu->EnableItem( RID_FILECLOSE );
	pMenu->EnableItem( RID_FILESAVE );
	pMenu->EnableItem( RID_FILESAVEAS );
	pMenu->EnableItem( RID_FILEPRINT );
	pMenu->EnableItem( RID_FILESETUP );
	pMenu->EnableItem( RID_FILELOADLIB );
	pMenu->EnableItem( RID_FILESAVELIB );

	pMenu->EnableItem( RID_RUNNEXTERR );
	pMenu->EnableItem( RID_RUNPREVERR );
	if( pWork ) pWork->DeInitMenu( pMenu );
*/
	SetAutoRun( FALSE );
	String aString;
	pStatus->Message( aString );
	return 0L;
}
IMPL_LINK_INLINE_END( BasicFrame, DeInitMenu, Menu *, pMenu )

IMPL_LINK_INLINE_START( BasicFrame, HighlightMenu, Menu *, pMenu )
{
	String s = pMenu->GetHelpText( pMenu->GetCurItemId() );
	pStatus->Message( s );
	return 0L;
}
IMPL_LINK_INLINE_END( BasicFrame, HighlightMenu, Menu *, pMenu )

IMPL_LINK_INLINE_START( BasicFrame, MenuCommand, Menu *, pMenu )
{
	USHORT nId = pMenu->GetCurItemId();
	BOOL bChecked = pMenu->IsItemChecked( nId );
	return Command( nId, bChecked );
}
IMPL_LINK_INLINE_END( BasicFrame, MenuCommand, Menu *, pMenu )

IMPL_LINK_INLINE_START( BasicFrame, Accel, Accelerator*, pAcc )
{
	SetAutoRun( FALSE );
	return Command( pAcc->GetCurItemId() );
}
IMPL_LINK_INLINE_END( BasicFrame, Accel, Accelerator*, pAcc )

IMPL_LINK_INLINE_START( BasicFrame, ShowLineNr, AutoTimer *, pTimer )
{
    (void) pTimer; /* avoid warning about unused parameter */ 
	String aPos;
	if ( pWork && pWork->ISA(AppBasEd))
	{
		aPos = String::CreateFromInt32(pWork->GetLineNr());
	}
	pStatus->Pos( aPos );
	return 0L;
}
IMPL_LINK_INLINE_END( BasicFrame, ShowLineNr, AutoTimer *, pTimer )


MsgEdit* BasicFrame::GetMsgTree( String aLogFileName )
{
	if ( FindErrorWin( aLogFileName ) )
	{
		return FindErrorWin( aLogFileName )->GetMsgTree();
	}
	else
	{	// create new Window on the fly
		AppError *pNewWindow = new AppError( this, aLogFileName );
		pNewWindow->Show();
		pNewWindow->GrabFocus();
		return pNewWindow->GetMsgTree();
	}
}

IMPL_LINK( BasicFrame, Log, TTLogMsg *, pLogMsg )
{
	GetMsgTree( pLogMsg->aLogFileName )->AddAnyMsg( pLogMsg );
	return 0L;
}

IMPL_LINK( BasicFrame, WinInfo, WinInfoRec*, pWinInfo )
{
	if ( !pDisplayHidDlg )
		pDisplayHidDlg = new DisplayHidDlg( this );
	if ( pDisplayHidDlg )
	{
		pDisplayHidDlg->AddData( pWinInfo );
		pDisplayHidDlg->Show();
	}
	return 0;
}

AppBasEd* BasicFrame::CreateModuleWin( SbModule* pMod )
{
	String aModName = pMod->GetName();
	if ( aModName.Copy(0,2).CompareToAscii("--") == COMPARE_EQUAL )
		aModName.Erase(0,2);
	pMod->SetName(aModName);
	AppBasEd* p = new AppBasEd( this, pMod );
	p->Show();
	p->GrabFocus();
	p->ToTop();
    return p;
}

BOOL BasicFrame::LoadFile( String aFilename )
{
	BOOL bIsResult = DirEntry( aFilename ).GetExtension().CompareIgnoreCaseToAscii("RES") == COMPARE_EQUAL;
	BOOL bIsBasic = DirEntry( aFilename ).GetExtension().CompareIgnoreCaseToAscii("BAS") == COMPARE_EQUAL;
	bIsBasic |= DirEntry( aFilename ).GetExtension().CompareIgnoreCaseToAscii("INC") == COMPARE_EQUAL;

	AppWin* p;
    BOOL bSuccess = TRUE;
	if ( bIsResult )
	{
		p = new AppError( this, aFilename );
	}
	else if ( bIsBasic )
	{
		p = new AppBasEd( this, NULL );
		bSuccess = p->Load( aFilename );
	}
	else
	{
		p = new AppEdit( this );
		bSuccess = p->Load( aFilename );
	}
    if ( bSuccess )
    {
	    p->Show();
	    p->GrabFocus();
    }
    else
        delete p;

	return bSuccess;
}

// Execute command
long BasicFrame::Command( short nID, BOOL bChecked )
{
	BasicError* pErr;

	switch( nID ) {
		case RID_FILENEW: {
			AppBasEd* p = new AppBasEd( this, NULL );
			p->Show();
			p->GrabFocus();
	//		InitMenu(GetMenuBar()->GetPopupMenu( RID_APPRUN ));
			} break;
		case RID_FILEOPEN:
			{
				String s;
				if( QueryFileName( s, FT_BASIC_SOURCE | FT_RESULT_FILE, FALSE ) ) {
					AddToLRU( s );
					LoadFile( s );
//					InitMenu(GetMenuBar()->GetPopupMenu( RID_APPRUN ));
				}
			} break;
		case RID_FILELOADLIB:
			LoadLibrary();
			break;
		case RID_FILESAVELIB:
			SaveLibrary();
			break;
		case RID_FILECLOSE:
			if( pWork && pWork->Close() ){};
//			InitMenu(GetMenuBar()->GetPopupMenu( RID_APPRUN ));
			break;
		case RID_FILEPRINT:
#ifndef UNX
			if( pWork )
				pPrn->Print( pWork->GetText(), pWork->pDataEdit->GetText(), this );
#else
			InfoBox( this, SttResId( IDS_NOPRINTERERROR ) ).Execute();
#endif
			break;
		case RID_FILESETUP:
#ifndef UNX
			pPrn->Setup();
#else
			InfoBox( this, SttResId( IDS_NOPRINTERERROR ) ).Execute();
#endif
			break;
		case RID_QUIT:
			if( Close() ) aBasicApp.Quit();
			break;


		case RID_RUNSTART:
			nFlags = SbDEBUG_BREAK;
			goto start;
		case RID_RUNSTEPOVER:
			nFlags = SbDEBUG_STEPINTO | SbDEBUG_STEPOVER;
			goto start;
		case RID_RUNSTEPINTO:
			nFlags = SbDEBUG_STEPINTO;
			goto start;
		case RID_RUNTOCURSOR:
			if ( pWork && pWork->ISA(AppBasEd) && ((AppBasEd*)pWork)->GetModule()->SetBP(pWork->GetLineNr()) )
			{
				SbModule *pModule = ((AppBasEd*)pWork)->GetModule();
#if OSL_DEBUG_LEVEL > 1
				USHORT x;
				x = pWork->GetLineNr();
				x = ((AppBasEd*)pWork)->GetModule()->GetBPCount();
				if ( !x )
					x = pModule->SetBP(pWork->GetLineNr());
				x = pModule->GetBPCount();
#endif

				for ( USHORT nMethod = 0; nMethod < pModule->GetMethods()->Count(); nMethod++ )
				{
					SbMethod* pMethod = (SbMethod*)pModule->GetMethods()->Get( nMethod );
					DBG_ASSERT( pMethod, "Methode nicht gefunden! (NULL)" );
					pMethod->SetDebugFlags( pMethod->GetDebugFlags() | SbDEBUG_BREAK );
				}
			}
			nFlags = SbDEBUG_BREAK;
			goto start;
		start: {
//			InitMenu(GetMenuBar()->GetPopupMenu( RID_APPRUN ));
			if ( !Basic().IsRunning() || bInBreak )
			{
				AppBasEd* p = NULL;
				if( pWork && pWork->ISA(AppBasEd) )
				{
					p = ((AppBasEd*)pWork);
					p->ToTop();
				}
				else
                {
                    AppWin *w = NULL;
                    for ( w = pList->Last() ; w ? !w->ISA(AppBasEd) : FALSE ; w = pList->Prev() ) ;
                    if ( w )
                    {
					    p = ((AppBasEd*)w);
					    p->ToTop();
                    }
                    else
                        if ( IsAutoRun() )
                            printf( "No file loaded to run.\n" );
                }

				if( bInBreak )
					// Reset the flag
					bInBreak = FALSE;
				else
				{
					if( IsAutoSave() && !SaveAll() ) break;
					if( !CompileAll() ) break;
					String aString;
					pStatus->Message( aString );
					if( p )
					{
						BasicDLL::SetDebugMode( TRUE );
						Basic().ClearGlobalVars();
						p->Run();
						BasicDLL::SetDebugMode( FALSE );
						// If cancelled during Interactive=FALSE
//						BasicDLL::EnableBreak( TRUE );
					}
				}}
			}
//			InitMenu(GetMenuBar()->GetPopupMenu( RID_APPRUN ));	// after run
			break;
		case RID_RUNCOMPILE:
			if( pWork && pWork->ISA(AppBasEd) && SaveAll() )
			{
				((AppBasEd*)pWork)->Compile();
				pWork->ToTop();
				pWork->GrabFocus();
			}
			break;
		case RID_RUNDISAS:
			bDisas = BOOL( !bChecked );
			break;
		case RID_RUNBREAK:
			if ( Basic().IsRunning() && !bInBreak )
			{
//				pINST->CalcBreakCallLevel(SbDEBUG_STEPINTO);
				pINST->nBreakCallLvl = pINST->nCallLvl;
			}
			break;
		case RID_RUNSTOP:
			Basic().Stop();
			bInBreak = FALSE;
			break;
		case RID_RUNNEXTERR:
			pErr = pBasic->aErrors.Next();
			if( pErr ) pErr->Show();
			break;
		case RID_RUNPREVERR:
			pErr = pBasic->aErrors.Prev();
			if( pErr ) pErr->Show();
			break;

		case RID_OPTIONS:
			{
				OptionsDialog *pOptions = new OptionsDialog( this, SttResId(IDD_OPTIONS_DLG) );
				pOptions->Show();
			}
			break;
		case RID_DECLARE_HELPER:
			InfoBox( this, SttResId( IDS_NOT_YET_IMPLEMENTED ) ).Execute();
			break;

		case RID_WINTILE:
			{
				WindowArrange aArange;
				for ( ULONG i = 0 ; i < pList->Count() ; i++ )
				{
					aArange.AddWindow( pList->GetObject( i ) );
					pList->GetObject( i )->Restore();
				}


				sal_Int32 nTitleHeight;
				{
					sal_Int32 nDummy1, nDummy2, nDummy3;
					GetBorder( nDummy1, nTitleHeight, nDummy2, nDummy3 );
				}

				Size aSize = GetOutputSizePixel();
				aSize.Height() -= nTitleHeight;
				Rectangle aRect( Point( 0, nTitleHeight ), aSize );

				aArange.Arrange( WINDOWARRANGE_TILE, aRect );

			}
			break;
		case RID_WINTILEHORZ:
			{
				WindowArrange aArange;
				for ( ULONG i = 0 ; i < pList->Count() ; i++ )
				{
					aArange.AddWindow( pList->GetObject( i ) );
					pList->GetObject( i )->Restore();
				}


				sal_Int32 nTitleHeight;
				{
					sal_Int32 nDummy1, nDummy2, nDummy3;
					GetBorder( nDummy1, nTitleHeight, nDummy2, nDummy3 );
				}

				Size aSize = GetOutputSizePixel();
				aSize.Height() -= nTitleHeight;
				Rectangle aRect( Point( 0, nTitleHeight ), aSize );

				aArange.Arrange( WINDOWARRANGE_HORZ, aRect );

			}
			break;
		case RID_WINTILEVERT:
//#define WINDOWARRANGE_TILE		1
//#define WINDOWARRANGE_HORZ		2
//#define WINDOWARRANGE_VERT		3
//#define WINDOWARRANGE_CASCADE	4
			{
				WindowArrange aArange;
				for ( ULONG i = 0 ; i < pList->Count() ; i++ )
				{
					aArange.AddWindow( pList->GetObject( i ) );
					pList->GetObject( i )->Restore();
				}


				sal_Int32 nTitleHeight;
				{
					sal_Int32 nDummy1, nDummy2, nDummy3;
					GetBorder( nDummy1, nTitleHeight, nDummy2, nDummy3 );
				}

				Size aSize = GetOutputSizePixel();
				aSize.Height() -= nTitleHeight;
				Rectangle aRect( Point( 0, nTitleHeight ), aSize );

				aArange.Arrange( WINDOWARRANGE_VERT, aRect );

			}
			break;
		case RID_WINCASCADE:
			{
				for ( USHORT i = 0 ; i < pList->Count() ; i++ )
				{
					pList->GetObject( i )->Cascade( i );
				}
			}
			break;

/*		case RID_HELPTOPIC:
			if( pWork ) pWork->Help();
			break;
		case RID_HELPKEYS:
			aBasicApp.pHelp->Start( CUniString( "Keyboard" ) );
			break;
		case RID_HELPINDEX:
			aBasicApp.pHelp->Start( OOO_HELP_INDEX );
			break;
		case RID_HELPINTRO:
			aBasicApp.pHelp->Start( OOO_HELP_HELPONHELP );
			break;
*/		case RID_HELPABOUT:
			{
				SttResId aResId( IDD_ABOUT_DIALOG );
				if ( Basic().pTestObject )    // Are we TestTool?
					aResId = SttResId( IDD_TT_ABOUT_DIALOG );
				else
					aResId = SttResId( IDD_ABOUT_DIALOG );
				AboutDialog aAbout( this, aResId );
				aAbout.Execute();
			}
			break;
		case RID_POPUPEDITVAR:
			{
				new VarEditDialog( this, pEditVar );
			}
			break;
		default:
			if ( nID >= RID_WIN_FILE1 && nID  <= RID_WIN_FILEn )
			{
				MenuBar* pMenu = GetMenuBar();
				PopupMenu* pWinMenu = pMenu->GetPopupMenu( RID_APPWINDOW );
				String aName = pWinMenu->GetItemText( nID );
                aName.EraseAllChars( L'~' );
				AppWin* pWin = FindWin( aName );
				if ( pWin )
					pWin->ToTop();
			}
			else if ( nID >= IDM_FILE_LRU1 && nID  <= IDM_FILE_LRUn )
			{
				String s = MENU2FILENAME( GetMenuBar()->GetPopupMenu(RID_APPFILE)->GetItemText(nID) );

				AddToLRU( s );
				LoadFile( s );
//				InitMenu(GetMenuBar()->GetPopupMenu( RID_APPRUN ));
			}
			else
			{
//				InitMenu(GetMenuBar()->GetPopupMenu( RID_APPEDIT ));	// So da� Delete richtig ist
				if( pWork )
					pWork->Command( CommandEvent( Point(), nID ) );
//				InitMenu(GetMenuBar()->GetPopupMenu( RID_APPEDIT ));	// So da� Delete richtig ist
			}
	}
	return TRUE;
}

BOOL BasicFrame::SaveAll()
{
	AppWin* p, *q = pWork;
	for( p = pList->First(); p; p = pList->Next() )
	{
		USHORT nRes = p->QuerySave( QUERY_DISK_CHANGED );
		if( (( nRes == SAVE_RES_ERROR ) && QueryBox(this,SttResId(IDS_ASKSAVEERROR)).Execute() == RET_NO )
			|| ( nRes == SAVE_RES_CANCEL ) )
			return FALSE;
	}
	if ( q )
		q->ToTop();
	return TRUE;
}

IMPL_LINK( BasicFrame, ModuleWinExists, String*, pFilename )
{
	return FindModuleWin( *pFilename ) != NULL;
}

AppBasEd* BasicFrame::FindModuleWin( const String& rName )
{
	AppWin* p;
	for( p = pList->First(); p; p = pList->Next() )
	{
		if( p->ISA(AppBasEd) && ((AppBasEd*)p)->GetModName() == rName )
			return ((AppBasEd*)p);
	}
	return NULL;
}

AppError* BasicFrame::FindErrorWin( const String& rName )
{
	AppWin* p;
	for( p = pList->First(); p; p = pList->Next() )
	{
		if( p->ISA(AppError) && ((AppError*)p)->GetText() == rName )
			return ((AppError*)p);
	}
	return NULL;
}

AppWin* BasicFrame::FindWin( const String& rName )
{
	AppWin* p;
	for( p = pList->First(); p; p = pList->Next() )
	{
		if( p->GetText() == rName )
			return p;
	}
	return NULL;
}

AppWin* BasicFrame::FindWin( USHORT nWinId )
{
	AppWin* p;
	for( p = pList->First(); p; p = pList->Next() )
	{
		if( p->GetWinId() == nWinId )
			return p;
	}
	return NULL;
}

AppWin* BasicFrame::IsWinValid( AppWin* pMaybeWin )
{
	AppWin* p;
	for( p = pList->First(); p; p = pList->Next() )
	{
		if( p == pMaybeWin )
			return p;
	}
	return NULL;
}

IMPL_LINK( BasicFrame, WriteString, String*, pString )
{
	if ( pList->Last() )
    {
        pList->Last()->pDataEdit->ReplaceSelected( *pString );
        return TRUE;
    }
    else
        return FALSE;
}

class NewFileDialog : public FileDialog
{
private:
	String aLastPath;
public:
	ByteString aFilterType;
	NewFileDialog( Window* pParent, WinBits nWinStyle ):FileDialog( pParent, nWinStyle ){};
	virtual short	Execute();
	virtual void	FilterSelect();
};

void NewFileDialog::FilterSelect()
{
	String aTemp = GetPath();
	if ( aLastPath.Len() == 0 )
		aLastPath = DirEntry( GetPath() ).GetPath().GetFull();
	if ( aLastPath.CompareIgnoreCaseToAscii( DirEntry( GetPath() ).GetPath().GetFull() ) != COMPARE_EQUAL )
		return;	// User decides after he has changed the path

	String aCurFilter = GetCurFilter();
	USHORT nFilterNr = 0;
	while ( nFilterNr < GetFilterCount() && aCurFilter != GetFilterName( nFilterNr ) )
	{
		nFilterNr++;
	}
	aFilterType = ByteString( GetFilterType( nFilterNr ), RTL_TEXTENCODING_UTF8 );

	Config aConf(Config::GetConfigName( Config::GetDefDirectory(), CUniString("testtool") ));
	aConf.SetGroup( "Misc" );
    ByteString aCurrentProfile = aConf.ReadKey( "CurrentProfile", "Path" );
    aConf.SetGroup( aCurrentProfile );
	aLastPath = UniString( aConf.ReadKey( aFilterType, aConf.ReadKey( "BaseDir" ) ), RTL_TEXTENCODING_UTF8 );
	SetPath( aLastPath );
//	if ( IsInExecute() )
//		SetPath( "" );
}

short NewFileDialog::Execute()
{
	BOOL bRet = (BOOL)FileDialog::Execute();
	if ( bRet )
	{
		Config aConf(Config::GetConfigName( Config::GetDefDirectory(), CUniString("testtool") ));
		aConf.SetGroup( "Misc" );
        ByteString aCurrentProfile = aConf.ReadKey( "CurrentProfile", "Path" );
        aConf.SetGroup( aCurrentProfile );
		aConf.WriteKey( aFilterType, ByteString( DirEntry( GetPath() ).GetPath().GetFull(), RTL_TEXTENCODING_UTF8 ) );
		aConf.WriteKey( "LastFilterName", ByteString( GetCurFilter(), RTL_TEXTENCODING_UTF8 ) );
	}
	return bRet;
}

BOOL BasicFrame::QueryFileName
				(String& rName, FileType nFileType, BOOL bSave )
{
	NewFileDialog aDlg( this, bSave ? WinBits( WB_SAVEAS ) :
								WinBits( WB_OPEN ) );
	aDlg.SetText( String( SttResId( bSave ? IDS_SAVEDLG : IDS_LOADDLG ) ) );

	if ( nFileType & FT_RESULT_FILE )
	{
		aDlg.SetDefaultExt( String( SttResId( IDS_RESFILE ) ) );
		aDlg.AddFilter( String( SttResId( IDS_RESFILTER ) ), String( SttResId( IDS_RESFILE ) ) );
		aDlg.AddFilter( String( SttResId( IDS_TXTFILTER ) ), String( SttResId( IDS_TXTFILE ) ) );
		aDlg.SetCurFilter( SttResId( IDS_RESFILTER ) );
	}

	if ( nFileType & FT_BASIC_SOURCE )
	{
		aDlg.SetDefaultExt( String( SttResId( IDS_NONAMEFILE ) ) );
		aDlg.AddFilter( String( SttResId( IDS_BASFILTER ) ), String( SttResId( IDS_NONAMEFILE ) ) );
		aDlg.AddFilter( String( SttResId( IDS_INCFILTER ) ), String( SttResId( IDS_INCFILE ) ) );
		aDlg.SetCurFilter( SttResId( IDS_BASFILTER ) );
	}

	if ( nFileType & FT_BASIC_LIBRARY )
	{
		aDlg.SetDefaultExt( String( SttResId( IDS_LIBFILE ) ) );
		aDlg.AddFilter( String( SttResId( IDS_LIBFILTER ) ), String( SttResId( IDS_LIBFILE ) ) );
		aDlg.SetCurFilter( SttResId( IDS_LIBFILTER ) );
	}

	Config aConf(Config::GetConfigName( Config::GetDefDirectory(), CUniString("testtool") ));
	aConf.SetGroup( "Misc" );
    ByteString aCurrentProfile = aConf.ReadKey( "CurrentProfile", "Path" );
    aConf.SetGroup( aCurrentProfile );
    ByteString aFilter( aConf.ReadKey( "LastFilterName") );
    if ( aFilter.Len() )
    	aDlg.SetCurFilter( String( aFilter, RTL_TEXTENCODING_UTF8 ) );
    else
	    aDlg.SetCurFilter( String( SttResId( IDS_BASFILTER ) ) );

	aDlg.FilterSelect(); // Selects the last used path
//	if ( bSave )
	if ( rName.Len() > 0 )
		aDlg.SetPath( rName );

	if( aDlg.Execute() )
	{
		rName = aDlg.GetPath();
/*		rExtension = aDlg.GetCurrentFilter();
		var i:integer;
		for ( i = 0 ; i < aDlg.GetFilterCount() ; i++ )
			if ( rExtension == aDlg.GetFilterName( i ) )
				rExtension = aDlg.GetFilterType( i );
*/
		return TRUE;
	} else return FALSE;
}

USHORT BasicFrame::BreakHandler()
{
	bInBreak = TRUE;
//	InitMenu(GetMenuBar()->GetPopupMenu( RID_APPRUN ));
//	MenuBar aBar( ResId( RID_APPMENUBAR ) );
//	aBar.EnableItem( RID_APPEDIT, FALSE );
	SetAppMode( String( SttResId ( IDS_APPMODE_BREAK ) ) );
	while( bInBreak )
		GetpApp()->Yield();
	SetAppMode( String( SttResId ( IDS_APPMODE_RUN ) ) );
//	aBar.EnableItem( RID_APPEDIT, TRUE );
//	InitMenu(GetMenuBar()->GetPopupMenu( RID_APPRUN ));
	return nFlags;
}

void BasicFrame::LoadLibrary()
{
	String s;
	if( QueryFileName( s, FT_BASIC_LIBRARY, FALSE ) )
	{
		CloseAll();
		SvFileStream aStrm( s, STREAM_STD_READ );
		MyBasic* pNew = (MyBasic*) SbxBase::Load( aStrm );
		if( pNew && pNew->ISA( MyBasic ) )
		{
			pBasic = pNew;
			// Show all contents if existing
			SbxArray* pMods = pBasic->GetModules();
			for( USHORT i = 0; i < pMods->Count(); i++ )
			{
				SbModule* pMod = (SbModule*) pMods->Get( i );
				AppWin* p = new AppBasEd( this, pMod );
				p->Show();
			}
		}
		else
		{
			delete pNew;
			ErrorBox( this, SttResId( IDS_READERROR ) ).Execute();
		}
	}
}

void BasicFrame::SaveLibrary()
{
	String s;
	if( QueryFileName( s, FT_BASIC_LIBRARY, TRUE ) )
	{
		SvFileStream aStrm( s, STREAM_STD_WRITE );
		if( !Basic().Store( aStrm ) )
			ErrorBox( this, SttResId( IDS_WRITEERROR ) ).Execute();
	}
}

String BasicFrame::GenRealString( const String &aResString )
{
	xub_StrLen nStart,nGleich = 0,nEnd = 0,nStartPos = 0;
	String aType,aValue,aResult(aResString);
	String aString;
	xub_StrLen nInsertPos = 0;
	BOOL bFound;
    bFound = FALSE;

	while ( (nStart = aResult.Search(StartKenn,nStartPos)) != STRING_NOTFOUND &&
			(nGleich = aResult.SearchAscii("=",nStart+StartKenn.Len())) != STRING_NOTFOUND &&
			(nEnd = aResult.Search(EndKenn,nGleich+1)) != STRING_NOTFOUND)
	{
		aType = aResult.Copy(nStart,nGleich-nStart);
		aValue = aResult.Copy(nGleich+1,nEnd-nGleich-1);
		if ( aType.CompareTo(ResKenn) == COMPARE_EQUAL )
		{
            if ( bFound )
            {
                // insert results of previous resource
	            DBG_ASSERT( aString.SearchAscii( "($Arg" ) == STRING_NOTFOUND, "Argument missing in String");
	            aResult.Insert( aString, nInsertPos );
                nStart = nStart + aString.Len();
                nEnd = nEnd + aString.Len();
                aString.Erase();
            }
//			if ( Resource::GetResManager()->IsAvailable( ResId( aValue ) ) )
				aString = String( SttResId( (USHORT)(aValue.ToInt32()) ) );
//			else
			{
//				DBG_ERROR( "Could not load resource!" );
//				return aResString;
			}
			nInsertPos = nStart;
			nStartPos = nStart;
			aResult.Erase( nStart, nEnd-nStart+1 );
            bFound = TRUE;
		}
		else if ( aType.Search(BaseArgKenn) == 0 ) // Starts with BaseArgKenn
		{
			// TODO: What the hell is that for??
			USHORT nArgNr = USHORT( aType.Copy( BaseArgKenn.Len() ).ToInt32() );
			DBG_ASSERT( aString.Search( CUniString("($Arg").Append( String::CreateFromInt32(nArgNr) ).AppendAscii(")") ) != STRING_NOTFOUND, "Extra Argument given in String");
			aString.SearchAndReplace( CUniString("($Arg").Append( String::CreateFromInt32(nArgNr) ).AppendAscii(")"), aValue );
			nStartPos = nStart;
			aResult.Erase( nStart, nEnd-nStart+1 );
		}
		else
		{
			DBG_ERROR( CByteString("Unknown replacement in String: ").Append( ByteString( aResult.Copy(nStart,nEnd-nStart), RTL_TEXTENCODING_UTF8 ) ).GetBuffer() );
			nStartPos = nStartPos + StartKenn.Len();
		}
	}
    if ( bFound )
    {
	    DBG_ASSERT( aString.SearchAscii( "($Arg" ) == STRING_NOTFOUND, "Argument missing in String");
	    aResult.Insert( aString, nInsertPos );
    }
	return aResult;
}


