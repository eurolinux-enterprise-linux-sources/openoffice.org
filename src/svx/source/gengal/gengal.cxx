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
#include "precompiled_svx.hxx"

#include <stdio.h>
#include <unistd.h>
#include <memory>
#include <list>

#include <unotools/streamwrap.hxx>
#include <unotools/ucbstreamhelper.hxx>

#include <comphelper/processfactory.hxx>
#include <comphelper/regpathhelper.hxx>
#include <cppuhelper/servicefactory.hxx>
#include <cppuhelper/bootstrap.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/registry/XSimpleRegistry.hpp>

#include <ucbhelper/contentbroker.hxx>
#include <ucbhelper/configurationkeys.hxx>

#include <tools/urlobj.hxx>
#include <tools/fsys.hxx>
#include <svtools/filedlg.hxx>

#include <vcl/window.hxx>
#include <vcl/svapp.hxx>
#include <vcl/font.hxx>
#include <vcl/sound.hxx>
#include <vcl/print.hxx>
#include <vcl/toolbox.hxx>
#include <vcl/help.hxx>
#include <vcl/scrbar.hxx>
#include <vcl/wrkwin.hxx>
#include <vcl/msgbox.hxx>

#include <osl/file.hxx>
#include <osl/process.h>
#include <rtl/bootstrap.hxx>

#include <galtheme.hxx>
#include <gallery1.hxx>

using namespace ::vos;
using namespace ::rtl;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::registry;
using namespace ::com::sun::star::lang;

typedef ::std::list<rtl::OUString> FileNameList;

class GalApp : public Application
{
public:
	virtual void Main();

protected:
    Reference<XMultiServiceFactory> xMSF;
    void Init();
    void InitUCB();
};

Gallery* createGallery( const rtl::OUString& aGalleryURL )
{
	return new Gallery( aGalleryURL );
}

void disposeGallery( Gallery* pGallery )
{
	delete pGallery;
}

static void createTheme( rtl::OUString aThemeName,
						 rtl::OUString aGalleryURL,
						 rtl::OUString aDestDir,
						 UINT32 nNumFrom,
						 FileNameList &rFiles )
{
	Gallery * pGallery( createGallery( aGalleryURL ) );

	if (!pGallery ) {
			fprintf( stderr, "Could't acquire '%s'\n",
					 (const sal_Char *) rtl::OUStringToOString( aGalleryURL,
																RTL_TEXTENCODING_UTF8 ) );
			exit( 1 );
	}
	fprintf( stderr, "Work on gallery '%s'\n",
					 (const sal_Char *) rtl::OUStringToOString( aGalleryURL, RTL_TEXTENCODING_UTF8 ) );

	fprintf( stderr, "Existing themes: %lu\n",
			 sal::static_int_cast< unsigned long >(
                 pGallery->GetThemeCount() ) );

	if( !pGallery->HasTheme( aThemeName) ) {
			if( !pGallery->CreateTheme( aThemeName, nNumFrom ) ) {
					fprintf( stderr, "Failed to create theme\n" );
					disposeGallery( pGallery );
					exit( 1 );
			}
	}

	fprintf( stderr, "Existing themes: %lu\n",
			 sal::static_int_cast< unsigned long >(
                 pGallery->GetThemeCount() ) );

	SfxListener aListener;

	GalleryTheme *pGalTheme = pGallery->AcquireTheme( aThemeName, aListener );
	if ( pGalTheme == NULL  ) {
			fprintf( stderr, "Failed to acquire theme\n" );
			disposeGallery( pGallery );
			exit( 1 );
	}

	fprintf( stderr, "Using DestDir: %s\n",
			 (const sal_Char *) rtl::OUStringToOString( aDestDir, RTL_TEXTENCODING_UTF8 ) );
	pGalTheme->SetDestDir(String(aDestDir));

	FileNameList::const_iterator aIter;
	
	for( aIter = rFiles.begin(); aIter != rFiles.end(); aIter++ )
	{
//  Should/could use:
//	if ( ! pGalTheme->InsertFileOrDirURL( aURL ) ) {
//	Requires a load more components ...

		Graphic aGraphic;
		String aFormat;

#if 1
		if ( ! pGalTheme->InsertURL( *aIter ) )
			fprintf( stderr, "Failed to import '%s'\n",
					 (const sal_Char *) rtl::OUStringToOString( *aIter, RTL_TEXTENCODING_UTF8 ) );
		else
			fprintf( stderr, "Imported file '%s' (%lu)\n",
					 (const sal_Char *) rtl::OUStringToOString( *aIter, RTL_TEXTENCODING_UTF8 ),
					 sal::static_int_cast< unsigned long >(
                         pGalTheme->GetObjectCount() ) );

#else // only loads BMPs
		SvStream *pStream = ::utl::UcbStreamHelper::CreateStream( *aIter, STREAM_READ );
		if (!pStream) {
			fprintf( stderr, "Can't find image to import\n" );
			disposeGallery( pGallery );
			exit (1);
		}
		*pStream >> aGraphic;
		delete pStream;
		if( aGraphic.GetType() == GRAPHIC_NONE )
		{
			fprintf( stderr, "Failed to load '%s'\n",
					 (const sal_Char *) rtl::OUStringToOString( *aIter, RTL_TEXTENCODING_UTF8 ) );
			continue;
		}

		SgaObjectBmp aObject( aGraphic, *aIter, aFormat );
		if ( ! aObject.IsValid() ) {
			fprintf( stderr, "Failed to create thumbnail for image\n" );
			continue;
		}
		
		if ( ! pGalTheme->InsertObject( aObject ) ) {
			fprintf( stderr, "Failed to insert file or URL\n" );
			continue;
		} 
#endif
	}

	pGallery->ReleaseTheme( pGalTheme, aListener );
	disposeGallery( pGallery );
}

static void PrintHelp()
{
	fprintf( stdout, "Utility to generate OO.o gallery files\n\n" );
	
	fprintf( stdout, "using: gengal --name <name> --path <dir> [ --destdir <path> ]\n");
	fprintf( stdout, "              [ --number-from <num> ] [ files ... ]\n\n" );
	
	fprintf( stdout, "options:\n");
	fprintf( stdout, " --name <theme>\t\tdefines a name of the created or updated theme.\n");
	fprintf( stdout, " --path <dir>\t\tdefines directory where the gallery files are created\n");
	fprintf( stdout, "\t\t\tor updated.\n");
	fprintf( stdout, " --destdir <dir>\tdefines a path prefix to be removed from the paths\n");
	fprintf( stdout, "\t\t\tstored in the gallery files. It is useful to create\n");
	fprintf( stdout, "\t\t\tRPM packages using the BuildRoot feature.\n");
	fprintf( stdout, " --number-from <num>\tdefines minimal number for the newly created gallery\n");
	fprintf( stdout, "\t\t\ttheme files.\n");
	fprintf( stdout, " files\t\t\tlists files to be added to the gallery. Absolute paths\n");
	fprintf( stdout, "\t\t\tare required.\n");
}

static OUString Smartify( const OUString &rPath )
{
	INetURLObject aURL;
	aURL.SetSmartURL( rPath );
	return aURL.GetMainURL( INetURLObject::NO_DECODE );
}

#define OUSTRING_CSTR( str ) \
    rtl::OUStringToOString( str, RTL_TEXTENCODING_ASCII_US ).getStr()

void GalApp::Init()
{
    if( getenv( "OOO_INSTALL_PREFIX" ) == NULL ) {
		OUString fileName = GetAppFileName();
		int lastSlash = fileName.lastIndexOf( '/' );
#ifdef WNT
		// Don't know which directory separators GetAppFileName() returns on Windows.
		// Be safe and take into consideration they might be backslashes.
		if( fileName.lastIndexOf( '\\' ) > lastSlash )
		    lastSlash = fileName.lastIndexOf( '\\' );
#endif
		OUString baseBinDir = fileName.copy( 0, lastSlash );
		OUString installPrefix = baseBinDir + OUString::createFromAscii( "/../.." );
		OUString assignment = OUString::createFromAscii( "OOO_INSTALL_PREFIX=" ) + installPrefix;
		putenv( strdup( OUSTRING_CSTR( assignment )));
    }
    OSL_TRACE( "OOO_INSTALL_PREFIX=%s", getenv( "OOO_INSTALL_PREFIX" ) );

	Reference<XComponentContext> xComponentContext
        = ::cppu::defaultBootstrap_InitialComponentContext();
	xMSF = Reference<XMultiServiceFactory>
        ( xComponentContext->getServiceManager(), UNO_QUERY );
	if( !xMSF.is() )
		fprintf( stderr, "Failed to bootstrap\n" );
	::comphelper::setProcessServiceFactory( xMSF );

    InitUCB();
}

void GalApp::InitUCB()
{
	OUString aEmpty;
	Sequence< Any > aArgs(6);
	aArgs[0]
		<<= rtl::OUString::createFromAscii(UCB_CONFIGURATION_KEY1_LOCAL);
	aArgs[1]
		<<= rtl::OUString::createFromAscii(UCB_CONFIGURATION_KEY2_OFFICE);
	aArgs[2] <<= rtl::OUString::createFromAscii("PIPE");
	aArgs[3] <<= aEmpty;
	aArgs[4] <<= rtl::OUString::createFromAscii("PORTAL");
	aArgs[5] <<= aEmpty;

	if (! ::ucbhelper::ContentBroker::initialize( xMSF, aArgs ) )
		fprintf( stderr, "Failed to init content broker\n" );
}

void GalApp::Main()
{
	bool bHelp = false;
	rtl::OUString aPath, aDestDir;
	rtl::OUString aName = rtl::OUString::createFromAscii( "Default name" );
	UINT32 nNumFrom = 0;
	FileNameList aFiles;

	for( USHORT i = 0; i < GetCommandLineParamCount(); i++ )
	{
		OUString aParam = GetCommandLineParam( i );

		if( aParam.equalsAscii( "--help" ) ||
			aParam.equalsAscii( "-h" ) )
				bHelp = true;

		else if ( aParam.equalsAscii( "--name" ) )
			aName = GetCommandLineParam( ++i );

		else if ( aParam.equalsAscii( "--path" ) )
			aPath = Smartify( GetCommandLineParam( ++i ) );

		else if ( aParam.equalsAscii( "--destdir" ) )
			aDestDir = GetCommandLineParam( ++i );

		else if ( aParam.equalsAscii( "--number-from" ) )
			 nNumFrom = GetCommandLineParam( ++i ).ToInt32();

		else
			aFiles.push_back( Smartify( aParam ) );
	}

	if( bHelp )
	{
		PrintHelp();
		return;
	}

	createTheme( aName, aPath, aDestDir, nNumFrom, aFiles );
}

GalApp aGalApp;
