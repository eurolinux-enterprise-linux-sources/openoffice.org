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
#include "precompiled_extensions.hxx"
#include <osl/diagnose.h>
#include <cppuhelper/factory.hxx>
#include <uno/mapping.hxx>
#include "scanner.hxx"

#include <com/sun/star/registry/XRegistryKey.hpp>

using namespace com::sun::star::registry;

// ------------------------------------------
// - component_getImplementationEnvironment -
// ------------------------------------------

extern "C" void SAL_CALL component_getImplementationEnvironment( const sal_Char** ppEnvTypeName, uno_Environment** /*ppEnv*/ )
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

// -----------------------
// - component_writeInfo -
// -----------------------

extern "C" sal_Bool SAL_CALL component_writeInfo( void* /*pServiceManager*/, void* pRegistryKey )
{
	sal_Bool bRet = sal_False;

	if( pRegistryKey )
	{
		try
		{
			::rtl::OUString aImplName( '/' );
			
			aImplName += ScannerManager::getImplementationName_Static();
			aImplName += ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "/UNO/SERVICES/" ) );
			aImplName += ScannerManager::getImplementationName_Static();

			REF( XRegistryKey ) xNewKey1( static_cast< XRegistryKey* >( pRegistryKey )->createKey( aImplName ) );
			
			bRet = sal_True;
		}
		catch( InvalidRegistryException& )
		{
			OSL_ENSURE( sal_False, "### InvalidRegistryException!" );
		}
	}

	return bRet;
}

// ------------------------
// - component_getFactory -
// ------------------------

extern "C" void* SAL_CALL component_getFactory( const sal_Char* pImplName, void* pServiceManager, void* /*pRegistryKey*/ )
{
	REF( ::com::sun::star::lang::XSingleServiceFactory ) xFactory;
	void*												 pRet = 0;
	
	if( ::rtl::OUString::createFromAscii( pImplName ) == ScannerManager::getImplementationName_Static() )
	{
		xFactory = REF( ::com::sun::star::lang::XSingleServiceFactory )( ::cppu::createSingleFactory(
						static_cast< ::com::sun::star::lang::XMultiServiceFactory* >( pServiceManager ),
						ScannerManager::getImplementationName_Static(),
						ScannerManager_CreateInstance, 
						ScannerManager::getSupportedServiceNames_Static() ) );
	}

	if( xFactory.is() )
	{
		xFactory->acquire();
		pRet = xFactory.get();
	}
	
	return pRet;
}
