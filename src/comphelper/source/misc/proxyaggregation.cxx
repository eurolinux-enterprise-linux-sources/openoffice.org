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
#include "precompiled_comphelper.hxx"
#include <comphelper/proxyaggregation.hxx>
#include <com/sun/star/reflection/XProxyFactory.hpp>

//.............................................................................
namespace comphelper
{
//.............................................................................

	using namespace ::com::sun::star::uno;
	using namespace ::com::sun::star::lang;
	using namespace ::com::sun::star::reflection;

	//=========================================================================
	//= OProxyAggregation
	//=========================================================================
	//-------------------------------------------------------------------------
	OProxyAggregation::OProxyAggregation( const Reference< XMultiServiceFactory >& _rxORB )
		:m_xORB( _rxORB )
	{
	}

	//-------------------------------------------------------------------------
	void OProxyAggregation::baseAggregateProxyFor( const Reference< XInterface >& _rxComponent, oslInterlockedCount& _rRefCount,
			::cppu::OWeakObject& _rDelegator )
	{
		// first a factory for the proxy
		Reference< XProxyFactory > xFactory(
			m_xORB->createInstance( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.reflection.ProxyFactory" ) ) ),
			UNO_QUERY
		);
		OSL_ENSURE( xFactory.is(), "OProxyAggregation::baseAggregateProxyFor: could not create a proxy factory!" );

		// then the proxy itself
		if ( xFactory.is() )
		{
			{ // i36686 OJ: achieve the desctruction of the tempoary -> otherwise it leads to _rRefCount -= 2
				m_xProxyAggregate = xFactory->createProxy( _rxComponent );
			}
            if ( m_xProxyAggregate.is() )
                m_xProxyAggregate->queryAggregation( ::getCppuType( &m_xProxyTypeAccess ) ) >>= m_xProxyTypeAccess;

			// aggregate the proxy
			osl_incrementInterlockedCount( &_rRefCount );
			if ( m_xProxyAggregate.is() )
			{
				// At this point in time, the proxy has a ref count of exactly two - in m_xControlContextProxy,
                // and in m_xProxyTypeAccess.
				// Remember to _not_ reset these members unless the delegator of the proxy has been reset, too!
				m_xProxyAggregate->setDelegator( _rDelegator );
			}
			osl_decrementInterlockedCount( &_rRefCount );
		}
	}

	//-------------------------------------------------------------------------
	Any SAL_CALL OProxyAggregation::queryAggregation( const Type& _rType ) throw (RuntimeException)
	{
		return m_xProxyAggregate.is() ? m_xProxyAggregate->queryAggregation( _rType ) : Any();
	}

	//-------------------------------------------------------------------------
	Sequence< Type > SAL_CALL OProxyAggregation::getTypes(  ) throw (RuntimeException)
	{
		Sequence< Type > aTypes;
		if ( m_xProxyAggregate.is() )
		{
			if ( m_xProxyTypeAccess.is() )
				aTypes = m_xProxyTypeAccess->getTypes();
		}
		return aTypes;
	}

	//-------------------------------------------------------------------------
	OProxyAggregation::~OProxyAggregation()
	{
		if ( m_xProxyAggregate.is() )
			m_xProxyAggregate->setDelegator( NULL );
		m_xProxyAggregate.clear();
        m_xProxyTypeAccess.clear();
			// this should remove the _two_only_ "real" references (means not delegated to
			// ourself) to this proxy, and thus delete it
	}

	//=========================================================================
	//= OComponentProxyAggregationHelper
	//=========================================================================
	//-------------------------------------------------------------------------
	OComponentProxyAggregationHelper::OComponentProxyAggregationHelper( const Reference< XMultiServiceFactory >& _rxORB,
		::cppu::OBroadcastHelper& _rBHelper )
		:OProxyAggregation( _rxORB )
		,m_rBHelper( _rBHelper )
	{
		OSL_ENSURE( _rxORB.is(), "OComponentProxyAggregationHelper::OComponentProxyAggregationHelper: invalid arguments!" );
	}

	//-------------------------------------------------------------------------
	void OComponentProxyAggregationHelper::componentAggregateProxyFor(
		const Reference< XComponent >& _rxComponent, oslInterlockedCount& _rRefCount,
		::cppu::OWeakObject& _rDelegator )
	{
		OSL_ENSURE( _rxComponent.is(), "OComponentProxyAggregationHelper::componentAggregateProxyFor: invalid inner component!" );
		m_xInner = _rxComponent;

		// aggregate a proxy for the object
		baseAggregateProxyFor( m_xInner.get(), _rRefCount, _rDelegator );

		// add as event listener to the inner context, because we want to be notified of disposals
		osl_incrementInterlockedCount( &_rRefCount );
		{
			if ( m_xInner.is() )
				m_xInner->addEventListener( this );
		}
		osl_decrementInterlockedCount( &_rRefCount );
	}

	//-------------------------------------------------------------------------
	Any SAL_CALL OComponentProxyAggregationHelper::queryInterface( const Type& _rType ) throw (RuntimeException)
	{
		Any aReturn( BASE::queryInterface( _rType ) );
		if ( !aReturn.hasValue() )
			aReturn = OProxyAggregation::queryAggregation( _rType );
		return aReturn;
	}

	//-------------------------------------------------------------------------
	IMPLEMENT_FORWARD_XTYPEPROVIDER2( OComponentProxyAggregationHelper, BASE, OProxyAggregation )

	//-------------------------------------------------------------------------
	OComponentProxyAggregationHelper::~OComponentProxyAggregationHelper( )
	{
		OSL_ENSURE( m_rBHelper.bDisposed, "OComponentProxyAggregationHelper::~OComponentProxyAggregationHelper: you should dispose your derived class in the dtor, if necessary!" );
			// if this asserts, add the following to your derived class dtor:
			//
			// if ( !m_rBHelper.bDisposed )
			// {
			//   acquire();	// to prevent duplicate dtor calls
			//   dispose();
			// }
			
		m_xInner.clear();
	}

	//-------------------------------------------------------------------------
	void SAL_CALL OComponentProxyAggregationHelper::disposing( const EventObject& _rSource ) throw (RuntimeException)
	{
		if ( _rSource.Source == m_xInner )
		{	// it's our inner context which is dying -> dispose ourself
			if ( !m_rBHelper.bDisposed && !m_rBHelper.bInDispose )
			{	// (if necessary only, of course)
				dispose();
			}
		}
	}

	//-------------------------------------------------------------------------
	void SAL_CALL OComponentProxyAggregationHelper::dispose() throw( RuntimeException )
	{
		::osl::MutexGuard aGuard( m_rBHelper.rMutex );

		// dispose our inner context
		// before we do this, remove ourself as listener - else in disposing( EventObject ), we
		// would dispose ourself a second time
		Reference< XComponent > xComp( m_xInner, UNO_QUERY );
		if ( xComp.is() )
		{
			xComp->removeEventListener( this );
			xComp->dispose();
			xComp.clear();
		}
	}

	//=========================================================================
	//= OComponentProxyAggregation
	//=========================================================================
	//-------------------------------------------------------------------------
	OComponentProxyAggregation::OComponentProxyAggregation( const Reference< XMultiServiceFactory >& _rxORB,
			const Reference< XComponent >& _rxComponent )
		:OComponentProxyAggregation_CBase( m_aMutex )
		,OComponentProxyAggregationHelper( _rxORB, rBHelper )
	{
		OSL_ENSURE( _rxComponent.is(), "OComponentProxyAggregation::OComponentProxyAggregation: accessible is no XComponent!" );
        if ( _rxComponent.is() )
		    componentAggregateProxyFor( _rxComponent, m_refCount, *this );
	}

	//-------------------------------------------------------------------------
	OComponentProxyAggregation::~OComponentProxyAggregation()
	{
		implEnsureDisposeInDtor( );
	}

	//-------------------------------------------------------------------------
	IMPLEMENT_FORWARD_XINTERFACE2( OComponentProxyAggregation, OComponentProxyAggregation_CBase, OComponentProxyAggregationHelper )

	//-------------------------------------------------------------------------
	IMPLEMENT_GET_IMPLEMENTATION_ID( OComponentProxyAggregation )

	//-------------------------------------------------------------------------
	Sequence< Type > SAL_CALL OComponentProxyAggregation::getTypes(  ) throw (RuntimeException)
	{
		Sequence< Type > aTypes( OComponentProxyAggregationHelper::getTypes() );

		// append XComponent, coming from OComponentProxyAggregation_CBase
		sal_Int32 nLen = aTypes.getLength();
		aTypes.realloc( nLen + 1 );
		aTypes[ nLen ] = ::getCppuType( static_cast< Reference< XComponent >* >( NULL ) );

		return aTypes;
	}

	//-------------------------------------------------------------------------
	void OComponentProxyAggregation::implEnsureDisposeInDtor( )
	{
		if ( !rBHelper.bDisposed )
		{
			acquire();	// to prevent duplicate dtor calls
			dispose();
		}
	}

	//--------------------------------------------------------------------
	void SAL_CALL OComponentProxyAggregation::disposing( const EventObject& _rSource ) throw (RuntimeException)
	{
		// simly disambiguate - this is necessary for MSVC to distinguish
		// "disposing( EventObject )" from "disposing()"
		OComponentProxyAggregationHelper::disposing( _rSource );
	}

	//--------------------------------------------------------------------
	void SAL_CALL OComponentProxyAggregation::disposing()  throw (RuntimeException)
	{
		// call the dispose-functionality of the base, which will dispose our aggregated component
		OComponentProxyAggregationHelper::dispose();
	}

	//--------------------------------------------------------------------
	void SAL_CALL OComponentProxyAggregation::dispose() throw( RuntimeException )
	{
		// simply disambiguate
		OComponentProxyAggregation_CBase::dispose();
	}


//.............................................................................
}	// namespace comphelper
//.............................................................................

