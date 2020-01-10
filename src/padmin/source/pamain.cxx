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

#include <stdio.h>
#include <unistd.h>

#include "tools/testtoolloader.hxx"

#include "vcl/svapp.hxx"
#include "vcl/wrkwin.hxx"
#include "vcl/unowrap.hxx"

#include "padialog.hxx"
#include "helper.hxx"
#include "desktopcontext.hxx"

#include "cppuhelper/bootstrap.hxx"
#include "comphelper/processfactory.hxx"
#include "ucbhelper/contentbroker.hxx"
#include "ucbhelper/configurationkeys.hxx"
#include "unotools/configmgr.hxx"

#include "com/sun/star/lang/XMultiServiceFactory.hpp"

using namespace padmin;
using namespace rtl;
using namespace cppu;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace comphelper;

// -----------------------------------------------------------------------

class MyApp : public Application
{
public:
    void			Main();
	virtual USHORT	Exception( USHORT nError );
    
    static void ReadStringHook( String& );
};

MyApp aMyApp;

void MyApp::ReadStringHook( String& rStr )
{
    static String maProduct;
    if( ! maProduct.Len() )
    {
        Any aRet = utl::ConfigManager::GetDirectConfigProperty( utl::ConfigManager::PRODUCTNAME );
        OUString aProd;
        aRet >>= aProd;
        maProduct = String( aProd );
    }
    rStr.SearchAndReplaceAllAscii( "%PRODUCTNAME", maProduct );
};


// -----------------------------------------------------------------------

USHORT MyApp::Exception( USHORT nError )
{
	switch( nError & EXC_MAJORTYPE )
	{
		case EXC_RSCNOTLOADED:
			Abort( String::CreateFromAscii( "Error: could not load language resources.\nPlease check your installation.\n" ) );
			break;
	}
	return 0;
}

void MyApp::Main()
{
	PADialog* pPADialog;

    EnableAutoHelpId();

	//-------------------------------------------------
	// create the global service-manager
	//-------------------------------------------------
    Reference< XMultiServiceFactory > xFactory;
    try
    {
        Reference< XComponentContext > xCtx = defaultBootstrap_InitialComponentContext();
        xFactory = Reference< XMultiServiceFactory >(  xCtx->getServiceManager(), UNO_QUERY );
        if( xFactory.is() )
            setProcessServiceFactory( xFactory );
    }
    catch( com::sun::star::uno::Exception& rExc)
    {
    }
    
    if( ! xFactory.is() )
    {
        fprintf( stderr, "Could not bootstrap UNO, installation must be in disorder. Exiting.\n" );
        exit( 1 );
    }

	// Detect desktop environment - need to do this as early as possible
    com::sun::star::uno::setCurrentContext(
        new DesktopContext( com::sun::star::uno::getCurrentContext() ) );

    /*
     *	Create UCB.
     */
	Sequence< Any > aArgs( 2 );
	aArgs[ 0 ] <<= OUString::createFromAscii( UCB_CONFIGURATION_KEY1_LOCAL );
	aArgs[ 1 ] <<= OUString::createFromAscii( UCB_CONFIGURATION_KEY2_OFFICE );
#if OSL_DEBUG_LEVEL > 1
	sal_Bool bSuccess =
#endif
        ::ucbhelper::ContentBroker::initialize( xFactory, aArgs );

#if OSL_DEBUG_LEVEL > 1
	if ( !bSuccess )
    {
		fprintf( stderr, "Error creating UCB, installation must be in disorder. Exiting.\n" );
        exit( 1 );
    }
#endif
    
    /*
     * Initialize the Java UNO AccessBridge if accessibility is turned on 
     */
     
    if( Application::GetSettings().GetMiscSettings().GetEnableATToolSupport() )
    {
        BOOL bQuitApp;
        if( !InitAccessBridge( true, bQuitApp ) )
            if( bQuitApp )
                return;
    }

    // initialize test-tool library (if available)
    tools::InitTestToolLib();
    
    ResMgr::SetReadStringHook( MyApp::ReadStringHook );

	pPADialog = PADialog::Create( NULL , FALSE );
	Application::SetDisplayName( pPADialog->GetText() );
	pPADialog->SetIcon(501);
	pPADialog->Execute();
	delete pPADialog;

    tools::DeInitTestToolLib();

    /*
     *  clean up UCB
     */
	::ucbhelper::ContentBroker::deinitialize();

}
