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
#include "precompiled_ucbhelper.hxx"

/**************************************************************************
								TODO
 **************************************************************************

 *************************************************************************/

#include <vector>

#include "ucbhelper/contentidentifier.hxx"
#include "ucbhelper/providerhelper.hxx"

#include "myucp_datasupplier.hxx"
#include "myucp_content.hxx"

using namespace com::sun::star;

// @@@ Adjust namespace name.
namespace myucp
{

//=========================================================================
//
// struct ResultListEntry.
//
//=========================================================================

struct ResultListEntry
{
	rtl::OUString						      aId;
	uno::Reference< ucb::XContentIdentifier > xId;
	uno::Reference< ucb::XContent > 		  xContent;
	uno::Reference< sdbc::XRow > 			  xRow;
	const ContentProperties& 		          rData;

	ResultListEntry( const ContentProperties& rEntry ) : rData( rEntry ) {}
};

//=========================================================================
//
// ResultList.
//
//=========================================================================

typedef std::vector< ResultListEntry* > ResultList;

//=========================================================================
//
// struct DataSupplier_Impl.
//
//=========================================================================

struct DataSupplier_Impl
{
	osl::Mutex					                 m_aMutex;
	ResultList					                 m_aResults;
	rtl::Reference< Content >     	             m_xContent;
	uno::Reference< lang::XMultiServiceFactory > m_xSMgr;
// @@@ The data source and an iterator for it
//	Entry 		 	      	  		             m_aFolder;
//	Entry::iterator 		  		             m_aIterator;
  	sal_Int32					                 m_nOpenMode;
  	sal_Bool					                 m_bCountFinal;

	DataSupplier_Impl( const uno::Reference< lang::XMultiServiceFactory >& rxSMgr,
	                   const rtl::Reference< Content >& rContent,
					   sal_Int32 nOpenMode )
	: m_xContent( rContent ), m_xSMgr( rxSMgr ),
//	  m_aFolder( rxSMgr, rContent->getIdentifier()->getContentIdentifier() ),
	  m_nOpenMode( nOpenMode ), m_bCountFinal( sal_False ) {}
	~DataSupplier_Impl();
};

//=========================================================================
DataSupplier_Impl::~DataSupplier_Impl()
{
	ResultList::const_iterator it  = m_aResults.begin();
	ResultList::const_iterator end = m_aResults.end();

	while ( it != end )
	{
		delete (*it);
		it++;
	}
}

//=========================================================================
//=========================================================================
//
// DataSupplier Implementation.
//
//=========================================================================
//=========================================================================

DataSupplier::DataSupplier( const uno::Reference< lang::XMultiServiceFactory >& rxSMgr,
							const rtl::Reference< Content >& rContent,
							sal_Int32 nOpenMode )
: m_pImpl( new DataSupplier_Impl( rxSMgr, rContent, nOpenMode ) )
{
}

//=========================================================================
// virtual
DataSupplier::~DataSupplier()
{
	delete m_pImpl;
}

//=========================================================================
// virtual
rtl::OUString DataSupplier::queryContentIdentifierString( sal_uInt32 nIndex )
{
	osl::Guard< osl::Mutex > aGuard( m_pImpl->m_aMutex );

	if ( nIndex < m_pImpl->m_aResults.size() )
	{
		rtl::OUString aId = m_pImpl->m_aResults[ nIndex ]->aId;
		if ( aId.getLength() )
		{
			// Already cached.
			return aId;
		}
	}

	if ( getResult( nIndex ) )
	{
		rtl::OUString aId
			= m_pImpl->m_xContent->getIdentifier()->getContentIdentifier();

		aId += m_pImpl->m_aResults[ nIndex ]->rData.aTitle;

		m_pImpl->m_aResults[ nIndex ]->aId = aId;
		return aId;
	}
	return rtl::OUString();
}

//=========================================================================
// virtual
uno::Reference< ucb::XContentIdentifier > 
DataSupplier::queryContentIdentifier( sal_uInt32 nIndex )
{
	osl::Guard< osl::Mutex > aGuard( m_pImpl->m_aMutex );

	if ( nIndex < m_pImpl->m_aResults.size() )
	{
		uno::Reference< ucb::XContentIdentifier > xId
								= m_pImpl->m_aResults[ nIndex ]->xId;
		if ( xId.is() )
		{
			// Already cached.
			return xId;
		}
	}

	rtl::OUString aId = queryContentIdentifierString( nIndex );
	if ( aId.getLength() )
	{
		uno::Reference< ucb::XContentIdentifier > xId 
            = new ::ucbhelper::ContentIdentifier( aId );
		m_pImpl->m_aResults[ nIndex ]->xId = xId;
		return xId;
	}
	return uno::Reference< ucb::XContentIdentifier >();
}

//=========================================================================
// virtual
uno::Reference< ucb::XContent > 
DataSupplier::queryContent( sal_uInt32 nIndex )
{
	osl::Guard< osl::Mutex > aGuard( m_pImpl->m_aMutex );

	if ( nIndex < m_pImpl->m_aResults.size() )
	{
		uno::Reference< ucb::XContent > xContent
								= m_pImpl->m_aResults[ nIndex ]->xContent;
		if ( xContent.is() )
		{
			// Already cached.
			return xContent;
		}
	}

	uno::Reference< ucb::XContentIdentifier > xId 
        = queryContentIdentifier( nIndex );
	if ( xId.is() )
	{
		try
		{
			uno::Reference< ucb::XContent > xContent
				= m_pImpl->m_xContent->getProvider()->queryContent( xId );
			m_pImpl->m_aResults[ nIndex ]->xContent = xContent;
			return xContent;

		}
		catch ( ucb::IllegalIdentifierException& )
		{
		}
	}
	return uno::Reference< ucb::XContent >();
}

//=========================================================================
// virtual
sal_Bool DataSupplier::getResult( sal_uInt32 nIndex )
{
	osl::ClearableGuard< osl::Mutex > aGuard( m_pImpl->m_aMutex );

	if ( m_pImpl->m_aResults.size() > nIndex )
	{
		// Result already present.
		return sal_True;
	}

	// Result not (yet) present.

	if ( m_pImpl->m_bCountFinal )
		return sal_False;

	// Try to obtain result...

	sal_uInt32 nOldCount = m_pImpl->m_aResults.size();
	sal_Bool bFound = sal_False;

	// @@@ Obtain data and put it into result list...
/*
	sal_uInt32 nPos = nOldCount;
	while ( m_pImpl->m_aFolder.next( m_pImpl->m_aIterator ) )
	{
		m_pImpl->m_aResults.push_back(
						new ResultListEntry( *m_pImpl->m_aIterator ) );

		if ( nPos == nIndex )
		{
			// Result obtained.
			bFound = sal_True;
			break;
		}

		nPos++;
	}
*/

	if ( !bFound )
		m_pImpl->m_bCountFinal = sal_True;

	rtl::Reference< ::ucbhelper::ResultSet > xResultSet = getResultSet().get();
	if ( xResultSet.is() )
	{
		// Callbacks follow!
		aGuard.clear();

		if ( nOldCount < m_pImpl->m_aResults.size() )
			xResultSet->rowCountChanged(
									nOldCount, m_pImpl->m_aResults.size() );

		if ( m_pImpl->m_bCountFinal )
			xResultSet->rowCountFinal();
	}

	return bFound;
}

//=========================================================================
// virtual
sal_uInt32 DataSupplier::totalCount()
{
	osl::ClearableGuard< osl::Mutex > aGuard( m_pImpl->m_aMutex );

	if ( m_pImpl->m_bCountFinal )
		return m_pImpl->m_aResults.size();

	sal_uInt32 nOldCount = m_pImpl->m_aResults.size();

	// @@@ Obtain data and put it into result list...
/*
	while ( m_pImpl->m_aFolder.next( m_pImpl->m_aIterator ) )
		m_pImpl->m_aResults.push_back(
						new ResultListEntry( *m_pImpl->m_aIterator ) );
*/
	m_pImpl->m_bCountFinal = sal_True;

	rtl::Reference< ::ucbhelper::ResultSet > xResultSet = getResultSet().get();
	if ( xResultSet.is() )
	{
		// Callbacks follow!
		aGuard.clear();

		if ( nOldCount < m_pImpl->m_aResults.size() )
			xResultSet->rowCountChanged(
									nOldCount, m_pImpl->m_aResults.size() );

		xResultSet->rowCountFinal();
	}

	return m_pImpl->m_aResults.size();
}

//=========================================================================
// virtual
sal_uInt32 DataSupplier::currentCount()
{
	return m_pImpl->m_aResults.size();
}

//=========================================================================
// virtual
sal_Bool DataSupplier::isCountFinal()
{
	return m_pImpl->m_bCountFinal;
}

//=========================================================================
// virtual
uno::Reference< sdbc::XRow > 
DataSupplier::queryPropertyValues( sal_uInt32 nIndex  )
{
	osl::Guard< osl::Mutex > aGuard( m_pImpl->m_aMutex );

	if ( nIndex < m_pImpl->m_aResults.size() )
	{
		uno::Reference< sdbc::XRow > xRow = m_pImpl->m_aResults[ nIndex ]->xRow;
		if ( xRow.is() )
		{
			// Already cached.
			return xRow;
		}
	}

	if ( getResult( nIndex ) )
	{
		uno::Reference< sdbc::XRow > xRow = Content::getPropertyValues(
									m_pImpl->m_xSMgr,
									getResultSet()->getProperties(),
									m_pImpl->m_aResults[ nIndex ]->rData,
									m_pImpl->m_xContent->getProvider().get(),
									queryContentIdentifierString( nIndex ) );
		m_pImpl->m_aResults[ nIndex ]->xRow = xRow;
		return xRow;
	}

	return uno::Reference< sdbc::XRow >();
}

//=========================================================================
// virtual
void DataSupplier::releasePropertyValues( sal_uInt32 nIndex )
{
	osl::Guard< osl::Mutex > aGuard( m_pImpl->m_aMutex );

	if ( nIndex < m_pImpl->m_aResults.size() )
		m_pImpl->m_aResults[ nIndex ]->xRow = uno::Reference< sdbc::XRow >();
}

//=========================================================================
// virtual
void DataSupplier::close()
{
}

//=========================================================================
// virtual
void DataSupplier::validate()
	throw( ucb::ResultSetException )
{
}

} // namespace
