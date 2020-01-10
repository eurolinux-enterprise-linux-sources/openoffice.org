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

#include "dp_help.hrc"
#include "dp_backend.h"
#include "dp_ucb.h"
#include "rtl/uri.hxx"
#include "osl/file.hxx"
#include "ucbhelper/content.hxx"
#include "comphelper/servicedecl.hxx"
#include "svtools/inettype.hxx"
#include "svtools/pathoptions.hxx"

#include <transex3/compilehelp.hxx> 
#include <com/sun/star/ucb/XSimpleFileAccess.hpp>
#include <com/sun/star/util/XMacroExpander.hpp>
#include <com/sun/star/uri/XUriReferenceFactory.hpp>
#include <com/sun/star/uri/XVndSunStarExpandUrl.hpp>
#include <com/sun/star/script/XInvocation.hpp>

using namespace ::dp_misc;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::ucb;
using ::rtl::OUString;

namespace dp_registry {
namespace backend {
namespace help {
namespace {

//==============================================================================
class BackendImpl : public ::dp_registry::backend::PackageRegistryBackend
{
    class PackageImpl : public ::dp_registry::backend::Package
    {
        BackendImpl * getMyBackend() const;
        
        // Package
        virtual beans::Optional< beans::Ambiguous<sal_Bool> > isRegistered_(
            ::osl::ResettableMutexGuard & guard,
            ::rtl::Reference<AbortChannel> const & abortChannel,
            Reference<XCommandEnvironment> const & xCmdEnv );
        virtual void processPackage_(
            ::osl::ResettableMutexGuard & guard,
            bool registerPackage,
            ::rtl::Reference<AbortChannel> const & abortChannel,
            Reference<XCommandEnvironment> const & xCmdEnv );

    public:
        inline PackageImpl(
            ::rtl::Reference<PackageRegistryBackend> const & myBackend,
            OUString const & url, OUString const & name,
            Reference<deployment::XPackageTypeInfo> const & xPackageType )
				: Package( myBackend, url, name, name, xPackageType )
            {}
    };
    friend class PackageImpl;

    // PackageRegistryBackend
    virtual Reference<deployment::XPackage> bindPackage_(
        OUString const & url, OUString const & mediaType,
        Reference<XCommandEnvironment> const & xCmdEnv );

	void implProcessHelp( Reference< deployment::XPackage > xPackage, bool doRegisterPackage );
	void implCollectXhpFiles( const rtl::OUString& aDir,
		std::vector< rtl::OUString >& o_rXhpFileVector );
	rtl::OUString getFlagFileURL( Reference< deployment::XPackage > xPackage, const char* pFlagStr );
	rtl::OUString getRegisteredFlagFileURL( Reference< deployment::XPackage > xPackage );
	rtl::OUString getCompiledFlagFileURL( Reference< deployment::XPackage > xPackage );
	rtl::OUString expandURL( const rtl::OUString& aURL );
	Reference< ucb::XSimpleFileAccess > getFileAccess( void );
	Reference< ucb::XSimpleFileAccess > m_xSFA;
    
    const Reference<deployment::XPackageTypeInfo> m_xHelpTypeInfo;
    Sequence< Reference<deployment::XPackageTypeInfo> > m_typeInfos;
    
public:
    BackendImpl( Sequence<Any> const & args,
                 Reference<XComponentContext> const & xComponentContext );
    
    // XPackageRegistry
    virtual Sequence< Reference<deployment::XPackageTypeInfo> > SAL_CALL
	    getSupportedPackageTypes() throw (RuntimeException);
};

//______________________________________________________________________________
BackendImpl::BackendImpl(
    Sequence<Any> const & args,
    Reference<XComponentContext> const & xComponentContext )
    : PackageRegistryBackend( args, xComponentContext ),
      m_xHelpTypeInfo( new Package::TypeInfo(
                               OUSTR("application/vnd.sun.star.help"),
                               rtl::OUString(),
                               getResourceString(RID_STR_HELP),
                               RID_IMG_HELP, RID_IMG_HELP_HC ) ),
      m_typeInfos( 1 )
{
    m_typeInfos[ 0 ] = m_xHelpTypeInfo;
}

// XPackageRegistry
//______________________________________________________________________________
Sequence< Reference<deployment::XPackageTypeInfo> >
BackendImpl::getSupportedPackageTypes() throw (RuntimeException)
{
    return m_typeInfos;
}

// PackageRegistryBackend
//______________________________________________________________________________
Reference<deployment::XPackage> BackendImpl::bindPackage_(
    OUString const & url, OUString const & mediaType_,
    Reference<XCommandEnvironment> const & xCmdEnv )
{
	// we don't support auto detection:
    if (mediaType_.getLength() == 0)
        throw lang::IllegalArgumentException(
            StrCannotDetectMediaType::get() + url,
            static_cast<OWeakObject *>(this), static_cast<sal_Int16>(-1) );
    
    String type, subType;
    INetContentTypeParameterList params;
	if (INetContentTypes::parse( mediaType_, type, subType, &params ))
    {
        if (type.EqualsIgnoreCaseAscii("application"))
        {
            ::ucbhelper::Content ucbContent( url, xCmdEnv );
            if (subType.EqualsIgnoreCaseAscii(
                    "vnd.sun.star.help"))
			{
                return new PackageImpl( this, url, 
					ucbContent.getPropertyValue( StrTitle::get() ).get<OUString>(), m_xHelpTypeInfo );
            }
        }
    }
    throw lang::IllegalArgumentException(
        StrUnsupportedMediaType::get() + mediaType_,
        static_cast<OWeakObject *>(this),
        static_cast<sal_Int16>(-1) );
}


//##############################################################################

// Package
BackendImpl * BackendImpl::PackageImpl::getMyBackend() const
{
    BackendImpl * pBackend = static_cast<BackendImpl *>(m_myBackend.get());
    if (NULL == pBackend)
    {    
        //May throw a DisposedException
        check();
        //We should never get here...
        throw RuntimeException(
            OUSTR("Failed to get the BackendImpl"), 
            static_cast<OWeakObject*>(const_cast<PackageImpl *>(this)));
    }
    return pBackend;
}

//______________________________________________________________________________
beans::Optional< beans::Ambiguous<sal_Bool> >
BackendImpl::PackageImpl::isRegistered_(
    ::osl::ResettableMutexGuard &,
    ::rtl::Reference<AbortChannel> const &,
    Reference<XCommandEnvironment> const & )
{
    BackendImpl * that = getMyBackend();
	Reference< deployment::XPackage > xThisPackage( this );
	rtl::OUString aRegisteredFlagFile = that->getRegisteredFlagFileURL( xThisPackage );

	Reference< ucb::XSimpleFileAccess > xSFA = that->getFileAccess();
	bool bReg = xSFA->exists( aRegisteredFlagFile );

	return beans::Optional< beans::Ambiguous<sal_Bool> >( true, beans::Ambiguous<sal_Bool>( bReg, false ) );
}

//______________________________________________________________________________
void BackendImpl::PackageImpl::processPackage_(
    ::osl::ResettableMutexGuard &,
    bool doRegisterPackage,
    ::rtl::Reference<AbortChannel> const & abortChannel,
    Reference<XCommandEnvironment> const & xCmdEnv )
{
	(void)doRegisterPackage;
	(void)abortChannel;
	(void)xCmdEnv;

    BackendImpl* that = getMyBackend();
	Reference< deployment::XPackage > xThisPackage( this );
	that->implProcessHelp( xThisPackage, doRegisterPackage );
}

//##############################################################################

static rtl::OUString aSlash( rtl::OUString::createFromAscii( "/" ) );
static rtl::OUString aHelpStr( rtl::OUString::createFromAscii( "help" ) );

void BackendImpl::implProcessHelp
	( Reference< deployment::XPackage > xPackage, bool doRegisterPackage )
{
	if( !xPackage.is() )
		return;

	Reference< ucb::XSimpleFileAccess > xSFA = getFileAccess();

	rtl::OUString aRegisteredFlagFile = getRegisteredFlagFileURL( xPackage );
	if( !doRegisterPackage )
	{
		if( xSFA->exists( aRegisteredFlagFile ) )
			xSFA->kill( aRegisteredFlagFile );
		return;
	}

	bool bCompile = true;
	rtl::OUString aCompiledFlagFile = getCompiledFlagFileURL( xPackage );
	if( xSFA->exists( aCompiledFlagFile ) )
		bCompile = false;

	if( bCompile )
	{
		rtl::OUString aHelpURL = xPackage->getURL();
		rtl::OUString aExpandedHelpURL = expandURL( aHelpURL );
		rtl::OUString aName = xPackage->getName();
		if( !xSFA->isFolder( aExpandedHelpURL ) )
		{
			rtl::OUString aErrStr = getResourceString( RID_STR_HELPPROCESSING_GENERAL_ERROR );
			aErrStr += rtl::OUString::createFromAscii( "No help folder" );
			OWeakObject* oWeakThis = static_cast<OWeakObject *>(this);
			throw deployment::DeploymentException( rtl::OUString(), oWeakThis,
				makeAny( uno::Exception( aErrStr, oWeakThis ) ) );
		}

		Reference<XComponentContext> const & xContext = getComponentContext();
		Reference< script::XInvocation > xInvocation;
		if( xContext.is() )
		{
			try
			{
				xInvocation = Reference< script::XInvocation >( 
					xContext->getServiceManager()->createInstanceWithContext( rtl::OUString::createFromAscii(
					"com.sun.star.help.HelpIndexer" ), xContext ) , UNO_QUERY );
			}
	        catch (Exception &)
			{
				// i98680: Survive missing lucene
			}
		}

		// Scan languages
		Sequence< rtl::OUString > aLanguageFolderSeq = xSFA->getFolderContents( aExpandedHelpURL, true );
		sal_Int32 nLangCount = aLanguageFolderSeq.getLength();
		const rtl::OUString* pSeq = aLanguageFolderSeq.getConstArray();
		for( sal_Int32 iLang = 0 ; iLang < nLangCount ; ++iLang )
		{
			rtl::OUString aLangURL = pSeq[iLang];
			if( xSFA->isFolder( aLangURL ) )
			{
				std::vector< rtl::OUString > aXhpFileVector;

				// Delete (old) files in any case to allow compiler to be started every time
				rtl::OUString aLangWithPureNameURL( aLangURL );
				aLangWithPureNameURL += aSlash;
				aLangWithPureNameURL += aHelpStr;
				rtl::OUString aDbFile( aLangWithPureNameURL );
				aDbFile += rtl::OUString::createFromAscii( ".db" );
				if( xSFA->exists( aDbFile ) )
					xSFA->kill( aDbFile );
				rtl::OUString aHtFile( aLangWithPureNameURL );
				aHtFile += rtl::OUString::createFromAscii( ".ht" );
				if( xSFA->exists( aHtFile ) )
					xSFA->kill( aHtFile );
				rtl::OUString aKeyFile( aLangWithPureNameURL );
				aKeyFile += rtl::OUString::createFromAscii( ".key" );
				if( xSFA->exists( aKeyFile ) )
					xSFA->kill( aKeyFile );

				// calculate jar file URL
				rtl::OUString aJarFile( aLangURL );
				aJarFile += aSlash;
				aJarFile += aHelpStr;
				aJarFile += rtl::OUString::createFromAscii( ".jar" );
				// remove in any case to clean up
				if( xSFA->exists( aJarFile ) )
					xSFA->kill( aJarFile );

				rtl::OUString aEncodedJarFilePath = rtl::Uri::encode( aJarFile,
					rtl_UriCharClassPchar, rtl_UriEncodeIgnoreEscapes, RTL_TEXTENCODING_UTF8 );
				rtl::OUString aDestBasePath = rtl::OUString::createFromAscii( "vnd.sun.star.pkg://" );
				aDestBasePath += aEncodedJarFilePath;
				aDestBasePath += rtl::OUString::createFromAscii( "/" );

				sal_Int32 nLenLangFolderURL = aLangURL.getLength() + 1;

				Sequence< rtl::OUString > aSubLangSeq = xSFA->getFolderContents( aLangURL, true );
				sal_Int32 nSubLangCount = aSubLangSeq.getLength();
				const rtl::OUString* pSubLangSeq = aSubLangSeq.getConstArray();
				for( sal_Int32 iSubLang = 0 ; iSubLang < nSubLangCount ; ++iSubLang )
				{
					rtl::OUString aSubFolderURL = pSubLangSeq[iSubLang];
					if( !xSFA->isFolder( aSubFolderURL ) )
						continue;

					implCollectXhpFiles( aSubFolderURL, aXhpFileVector );

					// Copy to package (later: move?)
					rtl::OUString aDestPath = aDestBasePath;
					rtl::OUString aPureFolderName = aSubFolderURL.copy( nLenLangFolderURL );
					aDestPath += aPureFolderName;
					xSFA->copy( aSubFolderURL, aDestPath );
				}

				// Call compiler
				sal_Int32 nXhpFileCount = aXhpFileVector.size();
				rtl::OUString* pXhpFiles = new rtl::OUString[nXhpFileCount];
				for( sal_Int32 iXhp = 0 ; iXhp < nXhpFileCount ; ++iXhp )
				{
					rtl::OUString aXhpFile = aXhpFileVector[iXhp];
					rtl::OUString aXhpRelFile = aXhpFile.copy( nLenLangFolderURL );
					pXhpFiles[iXhp] = aXhpRelFile;
				}

				rtl::OUString aOfficeHelpPath( SvtPathOptions().GetHelpPath() );
				rtl::OUString aOfficeHelpPathFileURL;
				::osl::File::getFileURLFromSystemPath( aOfficeHelpPath, aOfficeHelpPathFileURL ); 

				HelpProcessingErrorInfo aErrorInfo;
				bool bSuccess = compileExtensionHelp( aOfficeHelpPathFileURL, aHelpStr, aLangURL,
					nXhpFileCount, pXhpFiles, aErrorInfo );

				if( bSuccess && xInvocation.is() )
				{
					Sequence<uno::Any> aParamsSeq( 6 );

					aParamsSeq[0] = uno::makeAny( rtl::OUString::createFromAscii( "-lang" ) );

					rtl::OUString aLang;
					sal_Int32 nLastSlash = aLangURL.lastIndexOf( '/' );
					if( nLastSlash != -1 )
						aLang = aLangURL.copy( nLastSlash + 1 );
					else
						aLang = rtl::OUString::createFromAscii( "en" );
					aParamsSeq[1] = uno::makeAny( aLang );

					aParamsSeq[2] = uno::makeAny( rtl::OUString::createFromAscii( "-mod" ) );
					aParamsSeq[3] = uno::makeAny( rtl::OUString::createFromAscii( "help" ) );

					aParamsSeq[4] = uno::makeAny( rtl::OUString::createFromAscii( "-zipdir" ) );
					rtl::OUString aSystemPath;
					osl::FileBase::getSystemPathFromFileURL( aLangURL, aSystemPath );
					aParamsSeq[5] = uno::makeAny( aSystemPath );

					Sequence< sal_Int16 > aOutParamIndex;
					Sequence< uno::Any > aOutParam;
					uno::Any aRet = xInvocation->invoke( rtl::OUString::createFromAscii( "createIndex" ),
						aParamsSeq, aOutParamIndex, aOutParam );
				}

				if( !bSuccess )
				{
					USHORT nErrStrId = 0;
					switch( aErrorInfo.m_eErrorClass )
					{
						case HELPPROCESSING_GENERAL_ERROR:
						case HELPPROCESSING_INTERNAL_ERROR:		nErrStrId = RID_STR_HELPPROCESSING_GENERAL_ERROR; break;
						case HELPPROCESSING_XMLPARSING_ERROR:	nErrStrId = RID_STR_HELPPROCESSING_XMLPARSING_ERROR; break;
						default: ;
					};

					rtl::OUString aErrStr;
					if( nErrStrId != 0 )
					{
						aErrStr = getResourceString( nErrStrId );

						// Remoce CR/LF
						rtl::OUString aErrMsg( aErrorInfo.m_aErrorMsg );
						sal_Unicode nCR = 13, nLF = 10;
						sal_Int32 nSearchCR = aErrMsg.indexOf( nCR );
						sal_Int32 nSearchLF = aErrMsg.indexOf( nLF );
						sal_Int32 nCopy;
						if( nSearchCR != -1 || nSearchLF != -1 )
						{
							if( nSearchCR == -1 )
								nCopy = nSearchLF;
							else if( nSearchLF == -1 )
								nCopy = nSearchCR;
							else
								nCopy = ( nSearchCR < nSearchLF ) ? nSearchCR : nSearchLF;

							aErrMsg = aErrMsg.copy( 0, nCopy );
						}
						aErrStr += aErrMsg;
						if( nErrStrId == RID_STR_HELPPROCESSING_XMLPARSING_ERROR && aErrorInfo.m_aXMLParsingFile.getLength() )
						{
							aErrStr += rtl::OUString::createFromAscii( " in " );

							rtl::OUString aDecodedFile = rtl::Uri::decode( aErrorInfo.m_aXMLParsingFile,
								rtl_UriDecodeWithCharset, RTL_TEXTENCODING_UTF8 );
							aErrStr += aDecodedFile;
							if( aErrorInfo.m_nXMLParsingLine != -1 )
							{
								aErrStr += rtl::OUString::createFromAscii( ", line " );
								aErrStr += ::rtl::OUString::valueOf( aErrorInfo.m_nXMLParsingLine );
							}
						}
					}

					OWeakObject* oWeakThis = static_cast<OWeakObject *>(this);
					throw deployment::DeploymentException( rtl::OUString(), oWeakThis,
						makeAny( uno::Exception( aErrStr, oWeakThis ) ) );
				}
			}
		}

		// Write compiled flag file (this code is only reached in case of success)
		Reference< io::XOutputStream > xOutputStream = xSFA->openFileWrite( aCompiledFlagFile );
		if( xOutputStream.is() )
			xOutputStream->closeOutput();

	}	// if( bCompile )

	// Write registered flag file (this code is only reached in case of success)
	if( !xSFA->exists( aRegisteredFlagFile ) )
	{
		Reference< io::XOutputStream > xOutputStream = xSFA->openFileWrite( aRegisteredFlagFile );
		if( xOutputStream.is() )
			xOutputStream->closeOutput();
	}
}

rtl::OUString BackendImpl::getFlagFileURL( Reference< deployment::XPackage > xPackage, const char* pFlagStr )
{
	rtl::OUString aRetURL;
	if( !xPackage.is() )
		return aRetURL;
	rtl::OUString aHelpURL = xPackage->getURL();
	aRetURL = expandURL( aHelpURL );
	aRetURL += rtl::OUString::createFromAscii( pFlagStr );
	return aRetURL;
}

rtl::OUString BackendImpl::getRegisteredFlagFileURL( Reference< deployment::XPackage > xPackage )
{
	return getFlagFileURL( xPackage, "/RegisteredFlag" );
}

rtl::OUString BackendImpl::getCompiledFlagFileURL( Reference< deployment::XPackage > xPackage )
{
	return getFlagFileURL( xPackage, "/CompiledFlag" );
}

rtl::OUString BackendImpl::expandURL( const rtl::OUString& aURL )
{
	static Reference< util::XMacroExpander > xMacroExpander;
	static Reference< uri::XUriReferenceFactory > xFac;

	if( !xMacroExpander.is() || !xFac.is() )
	{
		Reference<XComponentContext> const & xContext = getComponentContext();
		if( xContext.is() )
		{
			xFac = Reference< uri::XUriReferenceFactory >(
				xContext->getServiceManager()->createInstanceWithContext( rtl::OUString::createFromAscii(
				"com.sun.star.uri.UriReferenceFactory"), xContext ) , UNO_QUERY );
		}
		if( !xFac.is() )
		{
			throw RuntimeException(
				::rtl::OUString::createFromAscii( 
				"dp_registry::backend::help::BackendImpl::expandURL(), "
				"could not instatiate UriReferenceFactory." ),
				Reference< XInterface >() );
		}

		xMacroExpander = Reference< util::XMacroExpander >(
			xContext->getValueByName(
			::rtl::OUString::createFromAscii( "/singletons/com.sun.star.util.theMacroExpander" ) ),
			UNO_QUERY_THROW );
 	}

	rtl::OUString aRetURL = aURL;
	if( xMacroExpander.is() )
	{
		Reference< uri::XUriReference > uriRef;
		for (;;)
		{
			uriRef = Reference< uri::XUriReference >( xFac->parse( aRetURL ), UNO_QUERY );
			if ( uriRef.is() )
			{
				Reference < uri::XVndSunStarExpandUrl > sxUri( uriRef, UNO_QUERY );
				if( !sxUri.is() )
					break;

				aRetURL = sxUri->expand( xMacroExpander );
			}
		}
 	}
	return aRetURL;
}

void BackendImpl::implCollectXhpFiles( const rtl::OUString& aDir,
	std::vector< rtl::OUString >& o_rXhpFileVector )
{
	Reference< ucb::XSimpleFileAccess > xSFA = getFileAccess();

	// Scan xhp files recursively
    Sequence< rtl::OUString > aSeq = xSFA->getFolderContents( aDir, true );
    sal_Int32 nCount = aSeq.getLength();
	const rtl::OUString* pSeq = aSeq.getConstArray();
	for( sal_Int32 i = 0 ; i < nCount ; ++i )
	{
		rtl::OUString aURL = pSeq[i];
		if( xSFA->isFolder( aURL ) )
		{
			implCollectXhpFiles( aURL, o_rXhpFileVector );
		}
		else
		{
			sal_Int32 nLastDot = aURL.lastIndexOf( '.' );
			if( nLastDot != -1 )
			{
				rtl::OUString aExt = aURL.copy( nLastDot + 1 );
				if( aExt.equalsIgnoreAsciiCase( rtl::OUString::createFromAscii( "xhp" ) ) )
					o_rXhpFileVector.push_back( aURL );
			}
		}
	}
}

Reference< ucb::XSimpleFileAccess > BackendImpl::getFileAccess( void )
{
	if( !m_xSFA.is() )
	{
		Reference<XComponentContext> const & xContext = getComponentContext();
		if( xContext.is() )
		{
			m_xSFA = Reference< ucb::XSimpleFileAccess >( 
				xContext->getServiceManager()->createInstanceWithContext(
					rtl::OUString::createFromAscii( "com.sun.star.ucb.SimpleFileAccess" ),
					xContext ), UNO_QUERY );
		}
		if( !m_xSFA.is() )
		{
			throw RuntimeException(
				::rtl::OUString::createFromAscii( 
				"dp_registry::backend::help::BackendImpl::getFileAccess(), "
				"could not instatiate SimpleFileAccess." ),
				Reference< XInterface >() );
		}
	}
	return m_xSFA;
}

} // anon namespace

namespace sdecl = comphelper::service_decl;
sdecl::class_<BackendImpl, sdecl::with_args<true> > serviceBI;
extern sdecl::ServiceDecl const serviceDecl(
    serviceBI,
    "com.sun.star.comp.deployment.help.PackageRegistryBackend",
    BACKEND_SERVICE_NAME );

} // namespace help
} // namespace backend
} // namespace dp_registry

