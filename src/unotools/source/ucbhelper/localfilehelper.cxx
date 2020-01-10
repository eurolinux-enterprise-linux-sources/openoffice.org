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
#include "precompiled_unotools.hxx"
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <com/sun/star/ucb/XContentAccess.hpp>
#include <com/sun/star/ucb/CommandAbortedException.hpp>

#include <unotools/localfilehelper.hxx>
#include <ucbhelper/fileidentifierconverter.hxx>
#include <ucbhelper/contentbroker.hxx>
#include <rtl/ustring.hxx>
#include <osl/file.hxx>
#include <tools/debug.hxx>
#include <tools/list.hxx>
#include <tools/urlobj.hxx>
#include <ucbhelper/content.hxx>

using namespace ::osl;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::ucb;

namespace utl
{

sal_Bool LocalFileHelper::ConvertSystemPathToURL( const String& rName, const String& rBaseURL, String& rReturn )
{
	rReturn = ::rtl::OUString();

    ::ucbhelper::ContentBroker* pBroker = ::ucbhelper::ContentBroker::get();
    if ( !pBroker )
    {
    	rtl::OUString aRet;
        if ( FileBase::getFileURLFromSystemPath( rName, aRet ) == FileBase::E_None )
			rReturn = aRet;
    }
    else
    {
        ::com::sun::star::uno::Reference< ::com::sun::star::ucb::XContentProviderManager > xManager =
                pBroker->getContentProviderManagerInterface();
        try
        {
            rReturn = ::ucbhelper::getFileURLFromSystemPath( xManager, rBaseURL, rName );
        }
        catch ( ::com::sun::star::uno::RuntimeException& )
        {
            return sal_False;
        }
    }

    return ( rReturn.Len() != 0 );
}

sal_Bool LocalFileHelper::ConvertURLToSystemPath( const String& rName, String& rReturn )
{
	rReturn = ::rtl::OUString();
    ::ucbhelper::ContentBroker* pBroker = ::ucbhelper::ContentBroker::get();
    if ( !pBroker )
    {
    	rtl::OUString aRet;
        if( FileBase::getSystemPathFromFileURL( rName, aRet ) == FileBase::E_None )
			rReturn = aRet;
    }
    else
    {
        ::com::sun::star::uno::Reference< ::com::sun::star::ucb::XContentProviderManager > xManager =
                pBroker->getContentProviderManagerInterface();
        try
        {
            rReturn = ::ucbhelper::getSystemPathFromFileURL( xManager, rName );
        }
        catch ( ::com::sun::star::uno::RuntimeException& )
        {
        }
    }

    return ( rReturn.Len() != 0 );
}

sal_Bool LocalFileHelper::ConvertPhysicalNameToURL( const String& rName, String& rReturn )
{
	rReturn = ::rtl::OUString();
    ::ucbhelper::ContentBroker* pBroker = ::ucbhelper::ContentBroker::get();
    if ( !pBroker )
    {
    	rtl::OUString aRet;
        if ( FileBase::getFileURLFromSystemPath( rName, aRet ) == FileBase::E_None )
			rReturn = aRet;
    }
    else
    {
        ::com::sun::star::uno::Reference< ::com::sun::star::ucb::XContentProviderManager > xManager =
                pBroker->getContentProviderManagerInterface();

		try
		{
            rtl::OUString aBase( ::ucbhelper::getLocalFileURL( xManager ) );
            rReturn = ::ucbhelper::getFileURLFromSystemPath( xManager, aBase, rName );
        }
        catch ( ::com::sun::star::uno::RuntimeException& )
        {
        }
    }

    return ( rReturn.Len() != 0 );
}

sal_Bool LocalFileHelper::ConvertURLToPhysicalName( const String& rName, String& rReturn )
{
	rReturn = ::rtl::OUString();
    ::ucbhelper::ContentBroker* pBroker = ::ucbhelper::ContentBroker::get();
    if ( !pBroker )
    {
		::rtl::OUString aRet;
        if ( FileBase::getSystemPathFromFileURL( rName, aRet ) == FileBase::E_None )
			rReturn = aRet;
    }
    else
    {
        ::com::sun::star::uno::Reference< ::com::sun::star::ucb::XContentProviderManager > xManager =
				pBroker->getContentProviderManagerInterface();
        try
        {
			INetURLObject aObj( rName );
			INetURLObject aLocal( ::ucbhelper::getLocalFileURL( xManager ) );
			if ( aObj.GetProtocol() == aLocal.GetProtocol() )
            	rReturn = ::ucbhelper::getSystemPathFromFileURL( xManager, rName );
        }
        catch ( ::com::sun::star::uno::RuntimeException& )
        {
        }
    }

    return ( rReturn.Len() != 0 );
}

sal_Bool LocalFileHelper::IsLocalFile( const String& rName )
{
    String aTmp;
    return ConvertURLToPhysicalName( rName, aTmp );
}

sal_Bool LocalFileHelper::IsFileContent( const String& rName )
{
    String aTmp;
    return ConvertURLToSystemPath( rName, aTmp );
}

DECLARE_LIST( StringList_Impl, ::rtl::OUString* )

::com::sun::star::uno::Sequence < ::rtl::OUString > LocalFileHelper::GetFolderContents( const ::rtl::OUString& rFolder, sal_Bool bFolder )
{
	StringList_Impl* pFiles = NULL;
	try
	{
        ::ucbhelper::Content aCnt( rFolder, Reference< XCommandEnvironment > () );
        Reference< ::com::sun::star::sdbc::XResultSet > xResultSet;
        ::com::sun::star::uno::Sequence< ::rtl::OUString > aProps(1);
        ::rtl::OUString* pProps = aProps.getArray();
        pProps[0] = ::rtl::OUString::createFromAscii( "Url" );

		try
		{
            ::ucbhelper::ResultSetInclude eInclude = bFolder ? ::ucbhelper::INCLUDE_FOLDERS_AND_DOCUMENTS : ::ucbhelper::INCLUDE_DOCUMENTS_ONLY;
            xResultSet = aCnt.createCursor( aProps, eInclude );
		}
		catch( ::com::sun::star::ucb::CommandAbortedException& )
		{
		}
        catch( Exception& )
		{
		}

		if ( xResultSet.is() )
		{
			pFiles = new StringList_Impl;
            Reference< XContentAccess > xContentAccess( xResultSet, UNO_QUERY );
			try
			{
				while ( xResultSet->next() )
				{
                    ::rtl::OUString aId = xContentAccess->queryContentIdentifierString();
                    ::rtl::OUString* pFile = new ::rtl::OUString( aId );
					pFiles->Insert( pFile, LIST_APPEND );
				}
			}
			catch( ::com::sun::star::ucb::CommandAbortedException& )
			{
			}
            catch( Exception& )
			{
			}
		}
	}
    catch( Exception& )
	{
	}

	if ( pFiles )
	{
        ULONG nCount = pFiles->Count();
        Sequence < ::rtl::OUString > aRet( nCount );
        ::rtl::OUString* pRet = aRet.getArray();
		for ( USHORT i = 0; i < nCount; ++i )
		{
            ::rtl::OUString* pFile = pFiles->GetObject(i);
			pRet[i] = *( pFile );
			delete pFile;
		}
		delete pFiles;
		return aRet;
	}
	else
        return Sequence < ::rtl::OUString > ();
}

}
