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

#include "quicktimecommon.hxx"
#include "manager.hxx"

using namespace ::com::sun::star;

// -------------------
// - factory methods -
// -------------------

static uno::Reference< uno::XInterface > SAL_CALL create_MediaPlayer( const uno::Reference< lang::XMultiServiceFactory >& rxFact )
{
	return uno::Reference< uno::XInterface >( *new ::avmedia::quicktime::Manager( rxFact ) );
}

// ------------------------------------------
// - component_getImplementationEnvironment -
// ------------------------------------------

extern "C" void SAL_CALL component_getImplementationEnvironment( const sal_Char ** ppEnvTypeName, uno_Environment ** /* ppEnv */ )
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

// -----------------------
// - component_writeInfo -
// -----------------------

extern "C" sal_Bool SAL_CALL component_writeInfo( void* /* pServiceManager */, void* pRegistryKey )
{
	sal_Bool bRet = sal_False;

	if( pRegistryKey )
	{
		try
		{
			uno::Reference< registry::XRegistryKey > xNewKey1(
				static_cast< registry::XRegistryKey* >( pRegistryKey )->createKey(                                
                ::rtl::OUString::createFromAscii(
                    "/" AVMEDIA_QUICKTIME_MANAGER_IMPLEMENTATIONNAME "/UNO/SERVICES/"
                    AVMEDIA_QUICKTIME_MANAGER_SERVICENAME ) ) );
            
			bRet = sal_True;
		}
		catch( registry::InvalidRegistryException& )
		{
			OSL_ENSURE( sal_False, "### InvalidRegistryException!" );
		}
	}

	return bRet;
}

// ------------------------
// - component_getFactory -
// ------------------------

extern "C" void* SAL_CALL component_getFactory( const sal_Char* pImplName, void* pServiceManager, void* /* pRegistryKey */ )
{
	uno::Reference< lang::XSingleServiceFactory > xFactory;
	void*									pRet = 0;

	if( rtl_str_compare( pImplName, AVMEDIA_QUICKTIME_MANAGER_IMPLEMENTATIONNAME ) == 0 )
	{
		const ::rtl::OUString aServiceName( ::rtl::OUString::createFromAscii( AVMEDIA_QUICKTIME_MANAGER_SERVICENAME ) );

		xFactory = uno::Reference< lang::XSingleServiceFactory >( ::cppu::createSingleFactory(
						reinterpret_cast< lang::XMultiServiceFactory* >( pServiceManager ),
						::rtl::OUString::createFromAscii( AVMEDIA_QUICKTIME_MANAGER_IMPLEMENTATIONNAME ),
						create_MediaPlayer, uno::Sequence< ::rtl::OUString >( &aServiceName, 1 ) ) );
	}

	if( xFactory.is() )
	{
		xFactory->acquire();
		pRet = xFactory.get();
	}

	return pRet;
}
