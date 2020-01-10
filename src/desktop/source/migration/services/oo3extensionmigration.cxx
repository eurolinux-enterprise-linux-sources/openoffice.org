/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * $RCSfile: extensionmigration.cxx,v $
 * $Revision: 1.2 $
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

#include "oo3extensionmigration.hxx"
#include <rtl/instance.hxx>
#include <osl/file.hxx>
#include <osl/thread.h>
#include <tools/urlobj.hxx>
#include <unotools/bootstrap.hxx>
#include <unotools/ucbstreamhelper.hxx>
#include <unotools/textsearch.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/processfactory.hxx>
#include <ucbhelper/content.hxx>

#include <com/sun/star/deployment/thePackageManagerFactory.hpp>
#include <com/sun/star/deployment/XPackageManagerFactory.hpp>
#include <com/sun/star/task/XInteractionApprove.hpp>
#include <com/sun/star/task/XInteractionAbort.hpp>
#include <com/sun/star/ucb/XCommandInfo.hpp>
#include <com/sun/star/ucb/TransferInfo.hpp>
#include <com/sun/star/ucb/NameClash.hpp>
#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <com/sun/star/xml/xpath/XXPathAPI.hpp>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

namespace migration
{

static ::rtl::OUString sExtensionSubDir = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "/user/uno_packages/" ) );
static ::rtl::OUString sSubDirName = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "cache" ) );
static ::rtl::OUString sConfigDir = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "/user/registry/data" ) );
static ::rtl::OUString sOrgDir = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "/user/registry/data/org" ) );
static ::rtl::OUString sExcludeDir1 = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "/user/registry/data/org" ) );
static ::rtl::OUString sExcludeDir2 = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "/user/registry/data/org/openoffice" ) );
static ::rtl::OUString sDescriptionXmlFile = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "/description.xml" ) );
static ::rtl::OUString sExtensionRootSubDirName = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "/uno_packages" ) );

static ::rtl::OUString sConfigurationDataType = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("application/vnd.sun.star.configuration-data"));
static ::rtl::OUString sConfigurationSchemaType = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("application/vnd.sun.star.configuration-schema"));

// =============================================================================
// component operations
// =============================================================================

::rtl::OUString OO3ExtensionMigration_getImplementationName()
{
    static ::rtl::OUString* pImplName = 0;
    if ( !pImplName )
    {
        ::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
        if ( !pImplName )
	    {
            static ::rtl::OUString aImplName( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.comp.desktop.migration.OOo3Extensions" ) );
		    pImplName = &aImplName;
	    }
    }
    return *pImplName;
}

// -----------------------------------------------------------------------------

Sequence< ::rtl::OUString > OO3ExtensionMigration_getSupportedServiceNames()
{
    static Sequence< ::rtl::OUString >* pNames = 0;
    if ( !pNames )
    {
        ::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
	    if ( !pNames )
	    {
            static Sequence< ::rtl::OUString > aNames(1);
            aNames.getArray()[0] = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.migration.Extensions" ) );
            pNames = &aNames;
	    }
    }
    return *pNames;
}

// =============================================================================
// ExtensionMigration
// =============================================================================

OO3ExtensionMigration::OO3ExtensionMigration(Reference< XComponentContext > const & ctx) :
m_ctx(ctx)
{
}

// -----------------------------------------------------------------------------

OO3ExtensionMigration::~OO3ExtensionMigration()
{
}

::osl::FileBase::RC OO3ExtensionMigration::checkAndCreateDirectory( INetURLObject& rDirURL )
{
    ::osl::FileBase::RC aResult = ::osl::Directory::create( rDirURL.GetMainURL( INetURLObject::DECODE_TO_IURI ) );
    if ( aResult == ::osl::FileBase::E_NOENT )
    {
        INetURLObject aBaseURL( rDirURL );
        aBaseURL.removeSegment();
        checkAndCreateDirectory( aBaseURL );
        return ::osl::Directory::create( rDirURL.GetMainURL( INetURLObject::DECODE_TO_IURI ) );
    }
    else
    {
        return aResult;
    }
}

void OO3ExtensionMigration::registerConfigurationPackage( const uno::Reference< deployment::XPackage > & xPkg)
{
    const ::rtl::OUString sMediaType = xPkg->getPackageType()->getMediaType();
    if ( (sMediaType.equals(sConfigurationDataType) || sMediaType.equals(sConfigurationSchemaType) ) )
    {
        xPkg->revokePackage(uno::Reference< task::XAbortChannel >(), uno::Reference< ucb::XCommandEnvironment> ());
        xPkg->registerPackage(uno::Reference< task::XAbortChannel >(), uno::Reference< ucb::XCommandEnvironment> ());
    }
}

 void OO3ExtensionMigration::scanUserExtensions( const ::rtl::OUString& sSourceDir, TStringVector& aMigrateExtensions )
{
    osl::Directory    aScanRootDir( sSourceDir );
    osl::FileStatus   fs(FileStatusMask_Type | FileStatusMask_FileURL);
    osl::FileBase::RC nRetCode = aScanRootDir.open();
    if ( nRetCode == osl::Directory::E_None )
    {
        sal_uInt32    nHint( 0 );
        osl::DirectoryItem aItem;
        while ( aScanRootDir.getNextItem( aItem, nHint ) == osl::Directory::E_None )
        {
            if (( aItem.getFileStatus(fs) == osl::FileBase::E_None ) &&
                ( fs.getFileType() == osl::FileStatus::Directory   ))
            {
                //Check next folder as the "real" extension folder is below a temp folder!
                ::rtl::OUString sExtensionFolderURL = fs.getFileURL();

                osl::DirectoryItem aExtDirItem;
                osl::Directory     aExtensionRootDir( sExtensionFolderURL );
                
                nRetCode = aExtensionRootDir.open();
                if (( nRetCode == osl::Directory::E_None ) &&
                    ( aExtensionRootDir.getNextItem( aExtDirItem, nHint ) == osl::Directory::E_None ))
                {
                    bool bFileStatus = aExtDirItem.getFileStatus(fs) == osl::FileBase::E_None;
                    bool bIsDir      = fs.getFileType() == osl::FileStatus::Directory;
                    
                    if ( bFileStatus && bIsDir )
                    {
                        sExtensionFolderURL = fs.getFileURL();
                        ScanResult eResult = scanExtensionFolder( sExtensionFolderURL );
                        if ( eResult == SCANRESULT_MIGRATE_EXTENSION )
                            aMigrateExtensions.push_back( sExtensionFolderURL );
                    }
                }
            }
        }
    }
}

OO3ExtensionMigration::ScanResult OO3ExtensionMigration::scanExtensionFolder( const ::rtl::OUString& sExtFolder )
{
    ScanResult     aResult = SCANRESULT_NOTFOUND;
    osl::Directory aDir(sExtFolder);
    
    // get sub dirs
    if (aDir.open() == osl::FileBase::E_None)
    {
        // work through directory contents...
        osl::DirectoryItem item;
        osl::FileStatus fs(FileStatusMask_Type | FileStatusMask_FileURL);
        TStringVector aDirectories;
        while ((aDir.getNextItem(item) == osl::FileBase::E_None ) &&
               ( aResult == SCANRESULT_NOTFOUND ))
        {
            if (item.getFileStatus(fs) == osl::FileBase::E_None)
            {
                ::rtl::OUString aDirEntryURL;
                if (fs.getFileType() == osl::FileStatus::Directory)
                    aDirectories.push_back( fs.getFileURL() );
                else
                {
                    aDirEntryURL = fs.getFileURL();
                    if ( aDirEntryURL.indexOf( sDescriptionXmlFile ) > 0 )
                        aResult = scanDescriptionXml( aDirEntryURL ) ? SCANRESULT_MIGRATE_EXTENSION : SCANRESULT_DONTMIGRATE_EXTENSION;
                }
            }
        }

        TStringVector::const_iterator pIter = aDirectories.begin();
        while ( pIter != aDirectories.end() && aResult == SCANRESULT_NOTFOUND )
        {
            aResult = scanExtensionFolder( *pIter );
            ++pIter;
        }
    }
    return aResult;
}

bool OO3ExtensionMigration::scanDescriptionXml( const ::rtl::OUString& sDescriptionXmlURL )
{
    if ( !m_xDocBuilder.is() )
    {
        m_xDocBuilder = uno::Reference< xml::dom::XDocumentBuilder >(
            m_ctx->getServiceManager()->createInstanceWithContext(
                ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("com.sun.star.xml.dom.DocumentBuilder")),
                m_ctx ), uno::UNO_QUERY );
    }

    if ( !m_xSimpleFileAccess.is() )
    {
        m_xSimpleFileAccess = uno::Reference< ucb::XSimpleFileAccess >(
            m_ctx->getServiceManager()->createInstanceWithContext(
                ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("com.sun.star.ucb.SimpleFileAccess")),
                m_ctx ), uno::UNO_QUERY );
    }
    
    ::rtl::OUString aExtIdentifier;
    if ( m_xDocBuilder.is() && m_xSimpleFileAccess.is() )
    {
        try
        {
            uno::Reference< io::XInputStream > xIn = 
                m_xSimpleFileAccess->openFileRead( sDescriptionXmlURL );

            if ( xIn.is() )
            {
                uno::Reference< xml::dom::XDocument > xDoc = m_xDocBuilder->parse( xIn );
                if ( xDoc.is() )
                {
                    uno::Reference< xml::dom::XElement > xRoot = xDoc->getDocumentElement();
                    if ( xRoot.is() && 
                         xRoot->getTagName().equals(::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("description"))) )
                    {
                        uno::Reference< xml::xpath::XXPathAPI > xPath(
                            m_ctx->getServiceManager()->createInstanceWithContext(
                                ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.xml.xpath.XPathAPI")),
                                m_ctx),
                            uno::UNO_QUERY);
                        
                        xPath->registerNS(
                            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("desc")),
                            xRoot->getNamespaceURI());
                        xPath->registerNS(
                            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("xlink")),
                            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("http://www.w3.org/1999/xlink")));
                        
                        try 
                        {
                            uno::Reference< xml::dom::XNode > xRootNode( xRoot, uno::UNO_QUERY );
                            uno::Reference< xml::dom::XNode > xNode( 
                                xPath->selectSingleNode(
                                    xRootNode, 
                                    ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("desc:identifier/@value")) ));
                            if ( xNode.is() )
                                aExtIdentifier = xNode->getNodeValue();
                        } 
                        catch ( xml::xpath::XPathException& )
                        {
                        }
                        catch ( xml::dom::DOMException& )
                        {
                        }
                    }
                }
            }
            
            if ( aExtIdentifier.getLength() > 0 )
            {
                // scan extension identifier and try to match with our black list entries
                for ( sal_uInt32 i = 0; i < m_aBlackList.size(); i++ )
                {
                    utl::SearchParam param(m_aBlackList[i], utl::SearchParam::SRCH_REGEXP);
                    utl::TextSearch  ts(param, LANGUAGE_DONTKNOW);

                    xub_StrLen start = 0;
                    xub_StrLen end   = static_cast<USHORT>(aExtIdentifier.getLength());
                    if (ts.SearchFrwrd(aExtIdentifier, &start, &end))
                        return false;
                }
            }
        }
        catch ( ucb::CommandAbortedException& )
        {
        }
        catch ( uno::RuntimeException& )
        {
        }

        if ( aExtIdentifier.getLength() == 0 )
        {
            // Fallback: 
            // Try to use the folder name to match our black list
            // as some extensions don't provide an identifier in the 
            // description.xml!
            for ( sal_uInt32 i = 0; i < m_aBlackList.size(); i++ )
            {
                utl::SearchParam param(m_aBlackList[i], utl::SearchParam::SRCH_REGEXP);
                utl::TextSearch  ts(param, LANGUAGE_DONTKNOW);

                xub_StrLen start = 0;
                xub_StrLen end   = static_cast<USHORT>(sDescriptionXmlURL.getLength());
                if (ts.SearchFrwrd(sDescriptionXmlURL, &start, &end))
                    return false;
            }
        }
    }

    return true;
}

bool OO3ExtensionMigration::migrateExtension( const ::rtl::OUString& sSourceDir )
{
    if ( !m_xPackageManager.is() )
    {
        try 
        {
            m_xPackageManager = deployment::thePackageManagerFactory::get( m_ctx )->getPackageManager( 
                ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM( "user" )) );
        }
        catch ( ucb::CommandFailedException & ){}
        catch ( uno::RuntimeException & ) {}
    }
    
    if ( m_xPackageManager.is() )
    {
        try
        {
            TmpRepositoryCommandEnv* pCmdEnv = new TmpRepositoryCommandEnv();
            
            uno::Reference< ucb::XCommandEnvironment > xCmdEnv( 
                static_cast< cppu::OWeakObject* >( pCmdEnv ), uno::UNO_QUERY );
            uno::Reference< task::XAbortChannel > xAbortChannel;
            uno::Reference< deployment::XPackage > xPackage = 
                m_xPackageManager->addPackage( sSourceDir, ::rtl::OUString(), xAbortChannel, xCmdEnv );

            if ( xPackage.is() )
                return true;
        }
        catch ( ucb::CommandFailedException& )
        {
        }
        catch ( ucb::CommandAbortedException& )
        {
        }
        catch ( lang::IllegalArgumentException& )
        {
        }
    }

    return false;
}

bool OO3ExtensionMigration::copy( const ::rtl::OUString& sSourceDir, const ::rtl::OUString& sTargetDir )
{
	bool bRet = false;

	INetURLObject aSourceObj( sSourceDir );
	INetURLObject aDestObj( sTargetDir );
	String aName = aDestObj.getName();
	aDestObj.removeSegment();
	aDestObj.setFinalSlash();

	try
	{
		::ucbhelper::Content aDestPath( aDestObj.GetMainURL( INetURLObject::NO_DECODE ), uno::Reference< ucb::XCommandEnvironment > () );
		uno::Reference< ucb::XCommandInfo > xInfo = aDestPath.getCommands();
		::rtl::OUString aTransferName = ::rtl::OUString::createFromAscii( "transfer" );
		if ( xInfo->hasCommandByName( aTransferName ) )
		{
			aDestPath.executeCommand( aTransferName, uno::makeAny(
				ucb::TransferInfo( sal_False, aSourceObj.GetMainURL( INetURLObject::NO_DECODE ), aName, ucb::NameClash::OVERWRITE ) ) );
			bRet = true;
		}
	}
	catch( uno::Exception& )
	{
	}

    return bRet;
}


// -----------------------------------------------------------------------------
// XServiceInfo
// -----------------------------------------------------------------------------

::rtl::OUString OO3ExtensionMigration::getImplementationName() throw (RuntimeException)
{
    return OO3ExtensionMigration_getImplementationName();
}

// -----------------------------------------------------------------------------

sal_Bool OO3ExtensionMigration::supportsService( const ::rtl::OUString& rServiceName ) throw (RuntimeException)
{
    Sequence< ::rtl::OUString > aNames( getSupportedServiceNames() );
    const ::rtl::OUString* pNames = aNames.getConstArray();
    const ::rtl::OUString* pEnd = pNames + aNames.getLength();
    for ( ; pNames != pEnd && !pNames->equals( rServiceName ); ++pNames )
	    ;

    return pNames != pEnd;
}

// -----------------------------------------------------------------------------

Sequence< ::rtl::OUString > OO3ExtensionMigration::getSupportedServiceNames() throw (RuntimeException)
{
    return OO3ExtensionMigration_getSupportedServiceNames();
}

// -----------------------------------------------------------------------------
// XInitialization
// -----------------------------------------------------------------------------

void OO3ExtensionMigration::initialize( const Sequence< Any >& aArguments ) throw (Exception, RuntimeException)
{
    ::osl::MutexGuard aGuard( m_aMutex );

    const Any* pIter = aArguments.getConstArray();
    const Any* pEnd = pIter + aArguments.getLength();
    for ( ; pIter != pEnd ; ++pIter )
    {
        beans::NamedValue aValue;
        *pIter >>= aValue;
        if ( aValue.Name.equalsAscii( "UserData" ) )
        {
            if ( !(aValue.Value >>= m_sSourceDir) )
            {
                OSL_ENSURE( false, "ExtensionMigration::initialize: argument UserData has wrong type!" );
            }
        }
        else if ( aValue.Name.equalsAscii( "ExtensionBlackList" ) )
        {
            Sequence< ::rtl::OUString > aBlackList;
            if ( (aValue.Value >>= aBlackList ) && ( aBlackList.getLength() > 0 ))
            {
                m_aBlackList.resize( aBlackList.getLength() );
                ::comphelper::sequenceToArray< ::rtl::OUString >( &m_aBlackList[0], aBlackList );
            }
        }
    }
}

// -----------------------------------------------------------------------------

TStringVectorPtr getContent( const ::rtl::OUString& rBaseURL )
{
    TStringVectorPtr aResult( new TStringVector );
    ::osl::Directory aDir( rBaseURL);
    if ( aDir.open() == ::osl::FileBase::E_None )
    {
        // iterate over directory content
        TStringVector aSubDirs;
        ::osl::DirectoryItem aItem;
        while ( aDir.getNextItem( aItem ) == ::osl::FileBase::E_None )
        {
            ::osl::FileStatus aFileStatus( FileStatusMask_Type | FileStatusMask_FileURL );
            if ( aItem.getFileStatus( aFileStatus ) == ::osl::FileBase::E_None )
                aResult->push_back( aFileStatus.getFileURL() );
        }
    }

    return aResult;
}

// -----------------------------------------------------------------------------
// XJob
// -----------------------------------------------------------------------------

void OO3ExtensionMigration::copyConfig( const ::rtl::OUString& sSourceDir, const ::rtl::OUString& sTargetDir )
{
	::rtl::OUString sEx1( m_sSourceDir );
	sEx1 += sExcludeDir1;
	::rtl::OUString sEx2( m_sSourceDir );
	sEx2 += sExcludeDir2;

	TStringVectorPtr aList = getContent( sSourceDir );
    TStringVector::const_iterator aI = aList->begin();
    while ( aI != aList->end() )
    {
		::rtl::OUString sSourceLocalName = aI->copy( sSourceDir.getLength() );
		::rtl::OUString aTemp = aI->copy( m_sSourceDir.getLength() );
		if ( aTemp != sExcludeDir1 && aTemp != sExcludeDir2 )
		{
			::rtl::OUString sTargetName = sTargetDir + sSourceLocalName;
			copy( (*aI), sTargetName );
		}
		++aI;
	}
}

Any OO3ExtensionMigration::execute( const Sequence< beans::NamedValue >& )
    throw (lang::IllegalArgumentException, Exception, RuntimeException)
{
    ::osl::MutexGuard aGuard( m_aMutex );

	::utl::Bootstrap::PathStatus aStatus = ::utl::Bootstrap::locateUserInstallation( m_sTargetDir );
	if ( aStatus == ::utl::Bootstrap::PATH_EXISTS )
	{
		// copy all extensions
		::rtl::OUString sSourceDir( m_sSourceDir );
        sSourceDir += sExtensionSubDir;
		sSourceDir += sSubDirName;
        sSourceDir += sExtensionRootSubDirName;
        TStringVector aExtensionToMigrate;
        scanUserExtensions( sSourceDir, aExtensionToMigrate );
        if ( aExtensionToMigrate.size() > 0 )
        {
            TStringVector::iterator pIter = aExtensionToMigrate.begin();
            while ( pIter != aExtensionToMigrate.end() )
            {
                migrateExtension( *pIter );
                ++pIter;
            }
        }
	}

    return Any();
}

// -----------------------------------------------------------------------------
// TmpRepositoryCommandEnv
// -----------------------------------------------------------------------------

TmpRepositoryCommandEnv::TmpRepositoryCommandEnv()
{
}

TmpRepositoryCommandEnv::TmpRepositoryCommandEnv(
    uno::Reference< task::XInteractionHandler> const & handler)
    : m_forwardHandler(handler)
{
}

TmpRepositoryCommandEnv::~TmpRepositoryCommandEnv()
{
}
// XCommandEnvironment
//______________________________________________________________________________
uno::Reference< task::XInteractionHandler > TmpRepositoryCommandEnv::getInteractionHandler()
throw ( uno::RuntimeException )
{
    return this;
}

//______________________________________________________________________________
uno::Reference< ucb::XProgressHandler > TmpRepositoryCommandEnv::getProgressHandler()
throw ( uno::RuntimeException )
{
    return this;
}

// XInteractionHandler
void TmpRepositoryCommandEnv::handle(
    uno::Reference< task::XInteractionRequest> const & xRequest )
    throw ( uno::RuntimeException )
{
    uno::Any request( xRequest->getRequest() );
    OSL_ASSERT( request.getValueTypeClass() == uno::TypeClass_EXCEPTION );
    
    bool approve = true;
    bool abort   = false;

    // select:
    uno::Sequence< Reference< task::XInteractionContinuation > > conts(
        xRequest->getContinuations() );
    Reference< task::XInteractionContinuation > const * pConts =
        conts.getConstArray();
    sal_Int32 len = conts.getLength();
    for ( sal_Int32 pos = 0; pos < len; ++pos )
    {
        if (approve) {
            uno::Reference< task::XInteractionApprove > xInteractionApprove(
                pConts[ pos ], uno::UNO_QUERY );
            if (xInteractionApprove.is()) {
                xInteractionApprove->select();
                // don't query again for ongoing continuations:
                approve = false;
            }
        }
        else if (abort) {
            uno::Reference< task::XInteractionAbort > xInteractionAbort(
                pConts[ pos ], uno::UNO_QUERY );
            if (xInteractionAbort.is()) {           
                xInteractionAbort->select();
                // don't query again for ongoing continuations:
                abort = false;
            }
        }
	}
}

// XProgressHandler
void TmpRepositoryCommandEnv::push( uno::Any const & /*Status*/ )
throw (uno::RuntimeException)
{
}


void TmpRepositoryCommandEnv::update( uno::Any const & /*Status */)
throw (uno::RuntimeException)
{
}

void TmpRepositoryCommandEnv::pop() throw (uno::RuntimeException)
{
}

// =============================================================================
// component operations
// =============================================================================

Reference< XInterface > SAL_CALL OO3ExtensionMigration_create(
    Reference< XComponentContext > const & ctx )
    SAL_THROW( () )
{
    return static_cast< lang::XTypeProvider * >( new OO3ExtensionMigration(
        ctx) );
}

// -----------------------------------------------------------------------------

}	// namespace migration
