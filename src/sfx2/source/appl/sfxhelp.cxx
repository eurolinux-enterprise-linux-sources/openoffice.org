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
#include "precompiled_sfx2.hxx"

#include "sfxhelp.hxx"

#include <algorithm>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#ifndef _UNOTOOLS_PROCESSFACTORY_HXX
#include <comphelper/processfactory.hxx>
#endif
#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/awt/XTopWindow.hpp>
#include <com/sun/star/awt/PosSize.hpp>
#include <com/sun/star/frame/XDesktop.hpp>
#include <com/sun/star/util/XURLTransformer.hpp>
#include <com/sun/star/frame/XDispatch.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/frame/FrameSearchFlag.hpp>
#include <toolkit/helper/vclunohelper.hxx>
#include <com/sun/star/frame/XModuleManager.hpp>
#include <unotools/configmgr.hxx>
#include <unotools/configitem.hxx>
#include <svtools/helpopt.hxx>
#include <svtools/moduleoptions.hxx>
#include <tools/urlobj.hxx>
#include <unotools/configmgr.hxx>
#include <ucbhelper/content.hxx>

#include <svtools/pathoptions.hxx>
#include <rtl/ustring.hxx>
#include <osl/process.h>
#include <osl/file.hxx>
#include <unotools/bootstrap.hxx>

#include <rtl/uri.hxx>
#include <vcl/msgbox.hxx>
#include <svtools/ehdl.hxx>
#include <svtools/sfxecode.hxx>

#define _SVSTDARR_STRINGSDTOR
#define _SVSTDARR_ULONGSSORT
#include <svtools/svstdarr.hxx>

#include <sfx2/sfxsids.hrc>
#include <sfx2/app.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/msgpool.hxx>
#include "newhelp.hxx"
#include <sfx2/objsh.hxx>
#include <sfx2/docfac.hxx>
#include "sfxresid.hxx"
#include "helper.hxx"
#include "app.hrc"

using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::lang;

#define ERROR_TAG	String( DEFINE_CONST_UNICODE("Error: ") )
#define PATH_TAG	String( DEFINE_CONST_UNICODE("\nPath: ") )

// class NoHelpErrorBox --------------------------------------------------

class NoHelpErrorBox : public ErrorBox
{
public:
    NoHelpErrorBox( Window* _pParent );

    virtual void    RequestHelp( const HelpEvent& rHEvt );
};

NoHelpErrorBox::NoHelpErrorBox( Window* _pParent ) :

    ErrorBox( _pParent, WB_OK, String( SfxResId( RID_STR_HLPFILENOTEXIST ) ) )
{
    // Error message: "No help available"
}

void NoHelpErrorBox::RequestHelp( const HelpEvent& )
{
    // do nothing, because no help available
}

// -----------------------------------------------------------------------

#define STARTERLIST 0

rtl::OUString HelpLocaleString()
{
	static rtl::OUString aLocaleStr;
	if (!aLocaleStr.getLength())
	{
		// detect installed locale
		Any aLocale =
			::utl::ConfigManager::GetConfigManager()->GetDirectConfigProperty(
			   ::utl::ConfigManager::LOCALE );
		bool bOk = (aLocale >>= aLocaleStr);
		if ( bOk )
		{
			rtl::OUString aBaseInstallPath;
			// utl::Bootstrap::PathStatus aBaseLocateResult =
			utl::Bootstrap::locateBaseInstallation(aBaseInstallPath);
			static const char *szHelpPath = "/help/";

			rtl::OUString sHelpPath = aBaseInstallPath +
				rtl::OUString::createFromAscii(szHelpPath) + aLocaleStr;
			osl::DirectoryItem aDirItem;

			if (!osl::DirectoryItem::get(sHelpPath, aDirItem) == osl::FileBase::E_None)
			{
				bOk = false;
				String sLang(aLocaleStr);
				xub_StrLen nSepPos = sLang.Search( '-' );
				if (nSepPos != STRING_NOTFOUND)
				{
					bOk = true;
        			sLang = sLang.Copy( 0, nSepPos );
					sHelpPath = aBaseInstallPath +
						rtl::OUString::createFromAscii(szHelpPath) + sLang;
					if (!osl::DirectoryItem::get(sHelpPath, aDirItem) == osl::FileBase::E_None)
						bOk = false;
				}
			}
		}
		if (!bOk)
			aLocaleStr = rtl::OUString( DEFINE_CONST_UNICODE("en") );
	}
	return aLocaleStr;
}

void AppendConfigToken_Impl( String& rURL, sal_Bool bQuestionMark )
{
	::rtl::OUString aLocaleStr(HelpLocaleString());

	// query part exists?
	if ( bQuestionMark )
		// no, so start with '?'
		rURL += '?';
	else
		// yes, so only append with '&'
		rURL += '&';

	// set parameters
	rURL += DEFINE_CONST_UNICODE("Language=");
	rURL += String( aLocaleStr );
	rURL += DEFINE_CONST_UNICODE("&System=");
	rURL += SvtHelpOptions().GetSystem();

}

// -----------------------------------------------------------------------

sal_Bool GetHelpAnchor_Impl( const String& _rURL, String& _rAnchor )
{
	sal_Bool bRet = sal_False;
	::rtl::OUString sAnchor;

    // --> OD 2009-07-01 #159496#
    // do not release solar mutex due to crash regarding accessibility
//    ULONG nSolarCount = Application::ReleaseSolarMutex();
    // <--
	try
	{
		::ucbhelper::Content aCnt( INetURLObject( _rURL ).GetMainURL( INetURLObject::NO_DECODE ),
							 Reference< ::com::sun::star::ucb::XCommandEnvironment > () );
		if ( ( aCnt.getPropertyValue( ::rtl::OUString::createFromAscii( "AnchorName" ) ) >>= sAnchor ) )
		{

			if ( sAnchor.getLength() > 0 )
			{
				_rAnchor = String( sAnchor );
				bRet = sal_True;
			}
		}
		else
		{
			DBG_ERRORFILE( "Property 'AnchorName' is missing" );
		}
	}
	catch( ::com::sun::star::uno::Exception& )
	{
	}
    // --> OD 2009-07-01 #159496#
//    Application::AcquireSolarMutex( nSolarCount );
    // <--

	return bRet;
}

// -----------------------------------------------------------------------

class SfxHelpOptions_Impl : public utl::ConfigItem
{
private:
    SvULongsSort*   m_pIds;

public:
                    SfxHelpOptions_Impl();
                    ~SfxHelpOptions_Impl();

    BOOL            HasId( ULONG nId ) { USHORT nDummy; return m_pIds ? m_pIds->Seek_Entry( nId, &nDummy ) : FALSE; }
};

static Sequence< ::rtl::OUString > GetPropertyNames()
{
	static const char* aPropNames[] =
	{
        "HelpAgentStarterList",
	};

    const int nCount = sizeof( aPropNames ) / sizeof( const char* );
	Sequence< ::rtl::OUString > aNames( nCount );
	::rtl::OUString* pNames = aNames.getArray();
	::rtl::OUString* pEnd	= pNames + aNames.getLength();
	int i = 0;
	for ( ; pNames != pEnd; ++pNames )
		*pNames = ::rtl::OUString::createFromAscii( aPropNames[i++] );

	return aNames;
}

// -----------------------------------------------------------------------

SfxHelpOptions_Impl::SfxHelpOptions_Impl()
    : ConfigItem( ::rtl::OUString::createFromAscii("Office.SFX/Help") )
    , m_pIds( NULL )
{
	Sequence< ::rtl::OUString > aNames = GetPropertyNames();
	Sequence< Any > aValues = GetProperties( aNames );
	EnableNotification( aNames );
	const Any* pValues = aValues.getConstArray();
	DBG_ASSERT( aValues.getLength() == aNames.getLength(), "GetProperties failed" );
	if ( aValues.getLength() == aNames.getLength() )
	{
		for ( int nProp = 0; nProp < aNames.getLength(); nProp++ )
		{
			DBG_ASSERT( pValues[nProp].hasValue(), "property value missing" );
			if ( pValues[nProp].hasValue() )
			{
				switch ( nProp )
				{
                    case STARTERLIST :
                    {
                        ::rtl::OUString aCodedList;
                        if ( pValues[nProp] >>= aCodedList )
                        {
                            String aTmp( aCodedList );
                            USHORT nCount = aTmp.GetTokenCount( ',' );
                            m_pIds = new SvULongsSort();
                            for ( USHORT n=0; n<nCount; n++ )
                                m_pIds->Insert( (ULONG) aTmp.GetToken( n, ',' ).ToInt64() );
                        }
                        else {
                            DBG_ERRORFILE( "Wrong property type!" );
                        }

                        break;
                    }

					default:
                        DBG_ERRORFILE( "Wrong property!" );
                        break;
				}
			}
		}
	}
}

SfxHelpOptions_Impl::~SfxHelpOptions_Impl()
{
    delete m_pIds;
}

// class SfxHelp_Impl ----------------------------------------------------

class SfxHelp_Impl
{
private:
	sal_Bool							m_bIsDebug;		// environment variable "help_debug=1"
    SfxHelpOptions_Impl*				m_pOpt;			// the options
	::std::vector< ::rtl::OUString >	m_aModulesList;	// list of all installed modules
	void					Load();

public:
    SfxHelp_Impl( sal_Bool bDebug );
    ~SfxHelp_Impl();

    SfxHelpOptions_Impl*	GetOptions();
	String					GetHelpText( ULONG nHelpId, const String& rModule );	// get "Active Help"
    String                  GetHelpText( const rtl::OUString& aCommandURL, const String& rModule );
	sal_Bool				HasModule( const ::rtl::OUString& rModule );			// module installed
	sal_Bool				IsHelpInstalled();										// module list not empty
};

SfxHelp_Impl::SfxHelp_Impl( sal_Bool bDebug ) :

	m_bIsDebug		( bDebug ),
    m_pOpt      	( NULL )

{
}

SfxHelp_Impl::~SfxHelp_Impl()
{
    delete m_pOpt;
}

void SfxHelp_Impl::Load()
{
	// fill modules list
	// create the help url (empty, without module and helpid)
	String sHelpURL( DEFINE_CONST_UNICODE("vnd.sun.star.help://") );
	AppendConfigToken_Impl( sHelpURL, sal_True );

	// open ucb content and get the list of the help modules
	// the list contains strings with three tokens "ui title \t type \t url"
	Sequence< ::rtl::OUString > aAllModulesList = SfxContentHelper::GetResultSet( sHelpURL );
	sal_Int32 nLen = aAllModulesList.getLength();
	m_aModulesList.reserve( nLen + 1 );
	const ::rtl::OUString* pBegin = aAllModulesList.getConstArray();
	const ::rtl::OUString* pEnd	= pBegin + nLen;
	for ( ; pBegin != pEnd; ++pBegin )
	{
		// get one module string
		String sModule( *pBegin );
		// extract the url
		String sURL = sModule.GetToken( 2, '\t' );
		// insert the module (the host part of the "vnd.sun.star.help" url)
		m_aModulesList.push_back( ::rtl::OUString( INetURLObject( sURL ).GetHost() ) );
	}
}

String SfxHelp_Impl::GetHelpText( ULONG nHelpId, const String& rModule )
{
	// create help url
    String aHelpURL = SfxHelp::CreateHelpURL( nHelpId, rModule );
	// added 'active' parameter
	aHelpURL.Insert( String( DEFINE_CONST_UNICODE("&Active=true") ), aHelpURL.SearchBackward( '#' ) );
	// load help string
	return SfxContentHelper::GetActiveHelpString( aHelpURL );
}

String SfxHelp_Impl::GetHelpText( const rtl::OUString& aCommandURL, const String& rModule )
{
	// create help url
    String aHelpURL = SfxHelp::CreateHelpURL( aCommandURL, rModule );
	// added 'active' parameter
	aHelpURL.Insert( String( DEFINE_CONST_UNICODE("&Active=true") ), aHelpURL.SearchBackward( '#' ) );
	// load help string
	return SfxContentHelper::GetActiveHelpString( aHelpURL );
}

SfxHelpOptions_Impl* SfxHelp_Impl::GetOptions()
{
	// create if not exists
    if ( !m_pOpt )
        m_pOpt = new SfxHelpOptions_Impl;
    return m_pOpt;
}

sal_Bool SfxHelp_Impl::HasModule( const ::rtl::OUString& rModule )
{
	if ( !m_aModulesList.size() )
		Load();
	return ( ::std::find( m_aModulesList.begin(), m_aModulesList.end(), rModule ) != m_aModulesList.end() );
}

sal_Bool SfxHelp_Impl::IsHelpInstalled()
{
	if ( !m_aModulesList.size() )
		Load();
	return ( m_aModulesList.begin() != m_aModulesList.end() );
}

// class SfxHelp ---------------------------------------------------------

SfxHelp::SfxHelp() :

	bIsDebug( sal_False ),
    pImp	( NULL )

{
	// read the environment variable "HELP_DEBUG"
	// if it's set, you will see debug output on active help
	{
		::rtl::OUString sHelpDebug;
		::rtl::OUString sEnvVarName( RTL_CONSTASCII_USTRINGPARAM( "HELP_DEBUG" ) );
		osl_getEnvironment( sEnvVarName.pData, &sHelpDebug.pData );
		bIsDebug = ( 0 != sHelpDebug.getLength() );
	}

	pImp = new SfxHelp_Impl( bIsDebug );

    ::rtl::OUString aLocaleStr = HelpLocaleString();

    sal_Int32 nSepPos = aLocaleStr.indexOf( '_' );
    if ( nSepPos != -1 )
    {
        aLanguageStr = aLocaleStr.copy( 0, nSepPos );
        aCountryStr = aLocaleStr.copy( nSepPos+1 );
    }
    else
    {
        nSepPos = aLocaleStr.indexOf( '-' );
        if ( nSepPos != -1 )
        {
            aLanguageStr = aLocaleStr.copy( 0, nSepPos );
            aCountryStr = aLocaleStr.copy( nSepPos+1 );
        }
        else
        {
            aLanguageStr = aLocaleStr;
        }
    }
}

SfxHelp::~SfxHelp()
{
    delete pImp;
}

::rtl::OUString getDefaultModule_Impl()
{
    rtl::OUString sDefaultModule;
    SvtModuleOptions aModOpt;
    if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SWRITER ) )
        sDefaultModule = DEFINE_CONST_UNICODE("swriter");
    else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SCALC ) )
        sDefaultModule = DEFINE_CONST_UNICODE("scalc");
    else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SIMPRESS ) )
        sDefaultModule = DEFINE_CONST_UNICODE("simpress");
    else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SDRAW ) )
        sDefaultModule = DEFINE_CONST_UNICODE("sdraw");
    else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SMATH ) )
        sDefaultModule = DEFINE_CONST_UNICODE("smath");
    else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SCHART ) )
        sDefaultModule = DEFINE_CONST_UNICODE("schart");
    else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SBASIC ) )
        sDefaultModule = DEFINE_CONST_UNICODE("sbasic");
    else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SDATABASE ) )
        sDefaultModule = DEFINE_CONST_UNICODE("sdatabase");
    else
    {
        DBG_ERRORFILE( "getDefaultModule_Impl(): no module installed" );
    }
    return sDefaultModule;
}

::rtl::OUString getCurrentModuleIdentifier_Impl()
{
    ::rtl::OUString sIdentifier;
    Reference < XFrame > xCurrentFrame;
    Reference < XModuleManager > xModuleManager( ::comphelper::getProcessServiceFactory()->createInstance(
        DEFINE_CONST_UNICODE("com.sun.star.frame.ModuleManager") ), UNO_QUERY );
    Reference < XDesktop > xDesktop( ::comphelper::getProcessServiceFactory()->createInstance(
        DEFINE_CONST_UNICODE("com.sun.star.frame.Desktop") ), UNO_QUERY );
    if ( xDesktop.is() )
        xCurrentFrame = xDesktop->getCurrentFrame();

    if ( xCurrentFrame.is() && xModuleManager.is() )
    {
        try
        {
            sIdentifier = xModuleManager->identify( xCurrentFrame );
        }
        catch ( ::com::sun::star::frame::UnknownModuleException& )
        {
            DBG_WARNING( "SfxHelp::getCurrentModuleIdentifier_Impl(): unknown module (help in help?)" );
        }
        catch ( Exception& )
        {
            DBG_ERRORFILE( "SfxHelp::getCurrentModuleIdentifier_Impl(): exception of XModuleManager::identify()" );
        }
    }

    return sIdentifier;
}

String SfxHelp::GetHelpModuleName_Impl()
{
    String sModuleName;
    rtl::OUString aFactoryShortName;
    rtl::OUString aModuleIdentifier = getCurrentModuleIdentifier_Impl();

    if ( aModuleIdentifier.getLength() > 0 )
    {
        try
        {
            Reference < XModuleManager > xModuleManager(
                ::comphelper::getProcessServiceFactory()->createInstance(
                    DEFINE_CONST_UNICODE("com.sun.star.frame.ModuleManager") ), UNO_QUERY );
            Sequence< PropertyValue > lProps;
            Reference< ::com::sun::star::container::XNameAccess > xCont( xModuleManager, UNO_QUERY);
            if ( xCont.is() )
                xCont->getByName( aModuleIdentifier ) >>= lProps;
            for ( sal_Int32 i = 0; i < lProps.getLength(); ++i )
            {
                if ( lProps[i].Name.equalsAscii("ooSetupFactoryShortName") )
                {
                    lProps[i].Value >>= aFactoryShortName;
                    break;
                }
            }
        }
        catch ( Exception& )
        {
            DBG_ERRORFILE( "SfxHelp::GetHelpModuleName_Impl(): exception of XNameAccess::getByName()" );
        }
    }

    rtl::OUString sDefaultModule = getDefaultModule_Impl();
    if ( aFactoryShortName.getLength() > 0 )
    {
        // Map some module identifiers to their "real" help module string.
        if ( aFactoryShortName.equalsAscii( "chart2" ) )
            aFactoryShortName = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "schart" ) );
        else if ( aFactoryShortName.equalsAscii( "BasicIDE" ) )
            aFactoryShortName = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "sbasic" ) );
        else if ( aFactoryShortName.equalsAscii( "sweb" )
                || aFactoryShortName.equalsAscii( "sglobal" )
                || aFactoryShortName.equalsAscii( "swxform" ) )
            aFactoryShortName = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "swriter" ) );
        else if ( aFactoryShortName.equalsAscii( "dbquery" )
                || aFactoryShortName.equalsAscii( "dbbrowser" )
                || aFactoryShortName.equalsAscii( "dbrelation" )
                || aFactoryShortName.equalsAscii( "dbtable" )
                || aFactoryShortName.equalsAscii( "dbapp" )
                || aFactoryShortName.equalsAscii( "dbreport" )
                || aFactoryShortName.equalsAscii( "swreport" )
                || aFactoryShortName.equalsAscii( "dbbrowser" )
                || aFactoryShortName.equalsAscii( "swform" ) )
            aFactoryShortName = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "sdatabase" ) );
        else if ( aFactoryShortName.equalsAscii( "sbibliography" )
                || aFactoryShortName.equalsAscii( "StartModule" ) )
            aFactoryShortName = sDefaultModule;
    }
    else
        aFactoryShortName = sDefaultModule;

    sModuleName = String( aFactoryShortName );
    return sModuleName;
}

String SfxHelp::CreateHelpURL_Impl( ULONG nHelpId, const String& rModuleName )
{
	String aModuleName( rModuleName );
	if ( aModuleName.Len() == 0 )
        aModuleName = getDefaultModule_Impl();

	// build up the help URL
    String aHelpURL;
    if ( aTicket.Len() )
    {
        // if there is a ticket, we are inside a plugin, so a special Help URL must be sent
        aHelpURL = DEFINE_CONST_UNICODE("vnd.sun.star.cmd:help?");
        aHelpURL += DEFINE_CONST_UNICODE("HELP_Request_Mode=contextIndex&HELP_Session_Mode=context&HELP_CallMode=portal&HELP_Device=html");

        if ( !nHelpId )
        {
			// no help id -> start page
            aHelpURL += DEFINE_CONST_UNICODE("&HELP_ContextID=start");
        }
        else
        {
            aHelpURL += DEFINE_CONST_UNICODE("&HELP_ContextID=");
            aHelpURL += String::CreateFromInt64( nHelpId );
        }

        aHelpURL += DEFINE_CONST_UNICODE("&HELP_ProgramID=");
        aHelpURL += aModuleName;
        aHelpURL += DEFINE_CONST_UNICODE("&HELP_User=");
        aHelpURL += aUser;
        aHelpURL += DEFINE_CONST_UNICODE("&HELP_Ticket=");
        aHelpURL += aTicket;
        aHelpURL += DEFINE_CONST_UNICODE("&HELP_Language=");
        aHelpURL += aLanguageStr;
        if ( aCountryStr.Len() )
        {
            aHelpURL += DEFINE_CONST_UNICODE("&HELP_Country=");
            aHelpURL += aCountryStr;
        }
    }
    else
    {
		sal_Bool bHasAnchor = sal_False;
		String aAnchor;
        aHelpURL = String::CreateFromAscii("vnd.sun.star.help://");
        aHelpURL += aModuleName;

        if ( !nHelpId )
            aHelpURL += String::CreateFromAscii("/start");
        else
        {
            aHelpURL += '/';
            aHelpURL += String::CreateFromInt64( nHelpId );

			String aTempURL = aHelpURL;
	        AppendConfigToken_Impl( aTempURL, sal_True );
			bHasAnchor = GetHelpAnchor_Impl( aTempURL, aAnchor );
		}

        AppendConfigToken_Impl( aHelpURL, sal_True );

		if ( bHasAnchor )
		{
			aHelpURL += '#';
			aHelpURL += aAnchor;
		}
    }

    return aHelpURL;
}

String	SfxHelp::CreateHelpURL_Impl( const String& aCommandURL, const String& rModuleName )
{
	// build up the help URL
    String aHelpURL;
	sal_Bool bHasAnchor = sal_False;
	String aAnchor;

	String aModuleName( rModuleName );
	if ( aModuleName.Len() == 0 )
	{
		// no active module (quicklaunch?) -> detect default module
		SvtModuleOptions aModOpt;
        if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SWRITER ) )
			aModuleName = DEFINE_CONST_UNICODE("swriter");
        else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SCALC ) )
			aModuleName = DEFINE_CONST_UNICODE("scalc");
        else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SIMPRESS ) )
			aModuleName = DEFINE_CONST_UNICODE("simpress");
        else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SDRAW ) )
			aModuleName = DEFINE_CONST_UNICODE("sdraw");
        else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SMATH ) )
			aModuleName = DEFINE_CONST_UNICODE("smath");
        else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SCHART ) )
			aModuleName = DEFINE_CONST_UNICODE("schart");
        else if ( aModOpt.IsModuleInstalled( SvtModuleOptions::E_SBASIC ) )
			aModuleName = DEFINE_CONST_UNICODE("sbasic");
		else
		{
			DBG_ERRORFILE( "no installed module found" );
		}
	}

    aHelpURL = String::CreateFromAscii("vnd.sun.star.help://");
    aHelpURL += aModuleName;

    if ( !aCommandURL.Len() )
        aHelpURL += String::CreateFromAscii("/start");
    else
    {
        aHelpURL += '/';
        aHelpURL += String( rtl::Uri::encode( aCommandURL,
                                              rtl_UriCharClassRelSegment,
                                              rtl_UriEncodeKeepEscapes,
                                              RTL_TEXTENCODING_ASCII_US ));

		String aTempURL = aHelpURL;
	    AppendConfigToken_Impl( aTempURL, sal_True );
		bHasAnchor = GetHelpAnchor_Impl( aTempURL, aAnchor );
	}

    AppendConfigToken_Impl( aHelpURL, sal_True );

	if ( bHasAnchor )
	{
		aHelpURL += '#';
		aHelpURL += aAnchor;
	}

    return aHelpURL;
}

static ::rtl::OUString OFFICE_HELP_TASK = ::rtl::OUString(DEFINE_CONST_UNICODE("OFFICE_HELP_TASK"));
static ::rtl::OUString OFFICE_HELP      = ::rtl::OUString(DEFINE_CONST_UNICODE("OFFICE_HELP"     ));

SfxHelpWindow_Impl* impl_createHelp(Reference< XFrame >& rHelpTask   ,
                                    Reference< XFrame >& rHelpContent)
{
    Reference < XFrame > xDesktop( ::comphelper::getProcessServiceFactory()->createInstance(
		DEFINE_CONST_UNICODE("com.sun.star.frame.Desktop") ), UNO_QUERY );

    // otherwhise - create new help task
    Reference< XFrame > xHelpTask = xDesktop->findFrame(
        OFFICE_HELP_TASK,
        FrameSearchFlag::TASKS | FrameSearchFlag::CREATE);
    if (!xHelpTask.is())
        return 0;

    // create all internal windows and sub frames ...
    Reference< ::com::sun::star::awt::XWindow > xParentWindow = xHelpTask->getContainerWindow();
    Window*                                     pParentWindow = VCLUnoHelper::GetWindow( xParentWindow );
    SfxHelpWindow_Impl*                         pHelpWindow   = new SfxHelpWindow_Impl( xHelpTask, pParentWindow, WB_DOCKBORDER );
    Reference< ::com::sun::star::awt::XWindow > xHelpWindow   = VCLUnoHelper::GetInterface( pHelpWindow );

    Reference< XFrame > xHelpContent;
    if (xHelpTask->setComponent( xHelpWindow, Reference< XController >() ))
    {
        // Customize UI ...
        xHelpTask->setName( OFFICE_HELP_TASK );

        Reference< XPropertySet > xProps(xHelpTask, UNO_QUERY);
        if (xProps.is())
            xProps->setPropertyValue(
                DEFINE_CONST_UNICODE("Title"),
                makeAny(::rtl::OUString(String(SfxResId(STR_HELP_WINDOW_TITLE)))));

        pHelpWindow->setContainerWindow( xParentWindow );
        xParentWindow->setVisible(sal_True);
        xHelpWindow->setVisible(sal_True);

        // This sub frame is created internaly (if we called new SfxHelpWindow_Impl() ...)
        // It should exist :-)
        xHelpContent = xHelpTask->findFrame(OFFICE_HELP, FrameSearchFlag::CHILDREN);
    }

    if (!xHelpContent.is())
        delete pHelpWindow;

    xHelpContent->setName(OFFICE_HELP);

    rHelpTask    = xHelpTask;
    rHelpContent = xHelpContent;
    return pHelpWindow;
}

BOOL SfxHelp::Start( const String& rURL, const Window* pWindow )
{
    // check if help is available
    String aHelpRootURL( DEFINE_CONST_OUSTRING("vnd.sun.star.help://") );
    AppendConfigToken_Impl( aHelpRootURL, sal_True );
    Sequence< ::rtl::OUString > aFactories = SfxContentHelper::GetResultSet( aHelpRootURL );
    if ( 0 == aFactories.getLength() )
    {
        // no factories -> no help -> error message and return
        NoHelpErrorBox aErrBox( const_cast< Window* >( pWindow ) );
        aErrBox.Execute();
        return FALSE;
    }

    // check if it's an URL or a jump mark!
	String          aHelpURL(rURL    );
	INetURLObject   aParser (aHelpURL);
	::rtl::OUString sKeyword;
    INetProtocol nProtocol = aParser.GetProtocol();
    if ( nProtocol != INET_PROT_VND_SUN_STAR_HELP )
	{
		// #i90162 Accept anything that is not invalid as help id, as both
		// uno: URLs used as commands/help ids in the Office and the scheme
		// used in extension help ids (e.g. com.foocorp.foo-ext:FooDialogButton)
		// are accepted as INET_PROT_UNO respectively INET_PROT_GENERIC
        bool bAcceptAsURL = ( nProtocol != INET_PROT_NOT_VALID );

		// #i94891 As in some extensions help ids like foo.bar.dummy without
		// any : have been used that worked before the fix of #i90162 (see
		// above) strings containing . will be also accepted to avoid brea-
		// king the help of existing extensions.
		if( !bAcceptAsURL )
		    bAcceptAsURL = ( rURL.Search( '.' ) != STRING_NOTFOUND );

        if ( bAcceptAsURL )
		{
            aHelpURL = CreateHelpURL_Impl( rURL, GetHelpModuleName_Impl( ) );
		}
        else
        {
            aHelpURL  = CreateHelpURL_Impl( 0, GetHelpModuleName_Impl( ) );

            // pb i91715: strings begin with ".HelpId:" are not words of the basic ide
            // they are helpid-strings used by the testtool -> so we ignore them
            static const String sHelpIdScheme( DEFINE_CONST_OUSTRING(".HelpId:") );
            if ( rURL.Search( sHelpIdScheme ) != 0 )
                sKeyword = ::rtl::OUString( rURL );
        }
	}

    Reference < XFrame > xDesktop( ::comphelper::getProcessServiceFactory()->createInstance(
		DEFINE_CONST_UNICODE("com.sun.star.frame.Desktop") ), UNO_QUERY );

    // check if help is still open
    // If not - create new one and return acces directly
    // to the internal sub frame, which shows the help content.

    // Note further: We search for this sub frame here directly instead of
    // the real top level help task ... It's needed to have the same
    // sub frame available - so we can use it for loading (which is done
    // in both cases)!

    Reference< XFrame > xHelp = xDesktop->findFrame(
        OFFICE_HELP_TASK,
        FrameSearchFlag::CHILDREN);
    Reference< XFrame > xHelpContent = xDesktop->findFrame(
        OFFICE_HELP,
        FrameSearchFlag::CHILDREN);

    SfxHelpWindow_Impl* pHelpWindow = 0;
    if (!xHelp.is())
        pHelpWindow = impl_createHelp(xHelp, xHelpContent);
    else
        pHelpWindow = (SfxHelpWindow_Impl*)VCLUnoHelper::GetWindow(xHelp->getComponentWindow());
    if (!xHelp.is() || !xHelpContent.is() || !pHelpWindow)
        return FALSE;

    pHelpWindow->SetHelpURL( aHelpURL );
    pHelpWindow->loadHelpContent(aHelpURL);
    if ( sKeyword.getLength() > 0 )
		pHelpWindow->OpenKeyword( sKeyword );

	Reference < ::com::sun::star::awt::XTopWindow > xTopWindow( xHelp->getContainerWindow(), UNO_QUERY );
    if ( xTopWindow.is() )
        xTopWindow->toFront();

    return TRUE;
}

BOOL SfxHelp::Start( ULONG nHelpId, const Window* pWindow )
{
    String aHelpModuleName( GetHelpModuleName_Impl() );
    String aHelpURL = CreateHelpURL( nHelpId, aHelpModuleName );
	if ( pWindow && SfxContentHelper::IsHelpErrorDocument( aHelpURL ) )
	{
		// no help found -> try with parent help id.
		Window* pParent = pWindow->GetParent();
		while ( pParent )
		{
            nHelpId = pParent->GetSmartUniqueOrHelpId().GetNum();
		    aHelpURL = CreateHelpURL( nHelpId, aHelpModuleName );

			if ( !SfxContentHelper::IsHelpErrorDocument( aHelpURL ) )
				break;
			else
			{
				pParent = pParent->GetParent();
				if ( !pParent )
					// create help url of start page ( helpid == 0 -> start page)
					aHelpURL = CreateHelpURL( 0, aHelpModuleName );
			}
		}
	}

	return Start( aHelpURL, pWindow );
}

XubString SfxHelp::GetHelpText( ULONG nHelpId, const Window* pWindow )
{
    String aModuleName = GetHelpModuleName_Impl();
    String aHelpText = pImp->GetHelpText( nHelpId, aModuleName );
	ULONG nNewHelpId = 0;

	if ( pWindow && aHelpText.Len() == 0 )
	{
		// no help text found -> try with parent help id.
		Window* pParent = pWindow->GetParent();
		while ( pParent )
		{
			nNewHelpId = pParent->GetHelpId();
			aHelpText = pImp->GetHelpText( nNewHelpId, aModuleName );

			if ( aHelpText.Len() > 0 )
				pParent = NULL;
			else
				pParent = pParent->GetParent();
		}

		if ( bIsDebug && aHelpText.Len() == 0 )
			nNewHelpId = 0;
	}

	if ( bIsDebug )
	{
		aHelpText += DEFINE_CONST_UNICODE("\n\n");
		aHelpText += aModuleName;
		aHelpText += DEFINE_CONST_UNICODE(" - ");
		aHelpText += String::CreateFromInt64( nHelpId );
		if ( nNewHelpId )
		{
			aHelpText += DEFINE_CONST_UNICODE(" - ");
			aHelpText += String::CreateFromInt64( nNewHelpId );
		}
	}

	return aHelpText;
}

XubString SfxHelp::GetHelpText( const String& aCommandURL, const Window* )
{
    String sModuleName = GetHelpModuleName_Impl();
    String sHelpText = pImp->GetHelpText( aCommandURL, sModuleName );

    // add some debug information?
    if ( bIsDebug )
    {
        sHelpText += DEFINE_CONST_UNICODE("\n-------------\n");
        sHelpText += String( sModuleName );
        sHelpText += DEFINE_CONST_UNICODE(": ");
        sHelpText += aCommandURL;
    }

    return sHelpText;
}

String SfxHelp::CreateHelpURL( ULONG nHelpId, const String& rModuleName )
{
	String aURL;
	SfxHelp* pHelp = SAL_STATIC_CAST( SfxHelp*, Application::GetHelp() );
	if ( pHelp )
		aURL = pHelp->CreateHelpURL_Impl( nHelpId, rModuleName );
	return aURL;
}

String SfxHelp::CreateHelpURL( const String& aCommandURL, const String& rModuleName )
{
	String aURL;
	SfxHelp* pHelp = SAL_STATIC_CAST( SfxHelp*, Application::GetHelp() );
	if ( pHelp )
		aURL = pHelp->CreateHelpURL_Impl( aCommandURL, rModuleName );
	return aURL;
}

void SfxHelp::OpenHelpAgent( SfxFrame *pFrame, ULONG nHelpId )
{
	if ( SvtHelpOptions().IsHelpAgentAutoStartMode() )
	{
		SfxHelp* pHelp = SAL_STATIC_CAST( SfxHelp*, Application::GetHelp() );
		if ( pHelp )
		{
			SfxHelpOptions_Impl *pOpt = pHelp->pImp->GetOptions();
    		if ( !pOpt->HasId( nHelpId ) )
        		return;

			try
			{
				URL aURL;
                aURL.Complete = pHelp->CreateHelpURL_Impl( nHelpId, pHelp->GetHelpModuleName_Impl() );
        		Reference < XURLTransformer > xTrans( ::comphelper::getProcessServiceFactory()->createInstance(
					::rtl::OUString::createFromAscii("com.sun.star.util.URLTransformer" ) ), UNO_QUERY );
        		xTrans->parseStrict(aURL);

        		Reference< XDispatchProvider > xDispProv( pFrame->GetTopFrame()->GetFrameInterface(), UNO_QUERY );
				Reference< XDispatch > xHelpDispatch;
				if ( xDispProv.is() )
					xHelpDispatch = xDispProv->queryDispatch(
						aURL, ::rtl::OUString::createFromAscii("_helpagent"),
						FrameSearchFlag::PARENT | FrameSearchFlag::SELF );

        		DBG_ASSERT( xHelpDispatch.is(), "OpenHelpAgent: could not get a dispatcher!" );
				if ( xHelpDispatch.is() )
					xHelpDispatch->dispatch( aURL, Sequence< PropertyValue >() );
			}
			catch( const Exception& )
			{
        		DBG_ERRORFILE( "OpenHelpAgent: caught an exception while executing the dispatch!" );
			}
		}
	}
}

String SfxHelp::GetDefaultHelpModule()
{
    return getDefaultModule_Impl();
}

::rtl::OUString SfxHelp::GetCurrentModuleIdentifier()
{
    return getCurrentModuleIdentifier_Impl();
}

