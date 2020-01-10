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
#include "precompiled_ucb.hxx"
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/registry/XRegistryKey.hpp>
#include "odma_contentprops.hxx"
#include "odma_provider.hxx"
#include "odma_lib.hxx"

using namespace rtl;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::registry;

//=========================================================================
static sal_Bool writeInfo( void * pRegistryKey,
						   const OUString & rImplementationName,
   						   Sequence< OUString > const & rServiceNames )
{
	OUString aKeyName( OUString::createFromAscii( "/" ) );
	aKeyName += rImplementationName;
	aKeyName += OUString::createFromAscii( "/UNO/SERVICES" );

	Reference< XRegistryKey > xKey;
	try
	{
		xKey = static_cast< XRegistryKey * >(
									pRegistryKey )->createKey( aKeyName );
	}
	catch ( InvalidRegistryException const & )
	{
	}

	if ( !xKey.is() )
		return sal_False;

	sal_Bool bSuccess = sal_True;

	for ( sal_Int32 n = 0; n < rServiceNames.getLength(); ++n )
	{
		try
		{
			xKey->createKey( rServiceNames[ n ] );
		}
		catch ( InvalidRegistryException const & )
		{
			bSuccess = sal_False;
			break;
		}
	}
	return bSuccess;
}

//=========================================================================
extern "C" void SAL_CALL component_getImplementationEnvironment(
	const sal_Char ** ppEnvTypeName, uno_Environment ** /*ppEnv*/ )
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

//=========================================================================
extern "C" sal_Bool SAL_CALL component_writeInfo(
	void * /*pServiceManager*/, void * pRegistryKey )
{
	return pRegistryKey &&

	//////////////////////////////////////////////////////////////////////
	// Write info into registry.
	//////////////////////////////////////////////////////////////////////

	// @@@ Adjust namespace names.
	writeInfo( pRegistryKey,
			   ::odma::ContentProvider::getImplementationName_Static(),
			   ::odma::ContentProvider::getSupportedServiceNames_Static() );
}

//=========================================================================
extern "C" void * SAL_CALL component_getFactory(
	const sal_Char * pImplName, void * pServiceManager, void * /*pRegistryKey*/ )
{
	void * pRet = 0;

	Reference< XMultiServiceFactory > xSMgr(
			reinterpret_cast< XMultiServiceFactory * >( pServiceManager ) );
	Reference< XSingleServiceFactory > xFactory;

	//////////////////////////////////////////////////////////////////////
	// Create factory, if implementation name matches.
	//////////////////////////////////////////////////////////////////////

	// @@@ Adjust namespace names.
	if ( ::odma::ContentProvider::getImplementationName_Static().
				compareToAscii( pImplName ) == 0 )
	{
		if(::odma::LoadLibrary())
			xFactory = ::odma::ContentProvider::createServiceFactory( xSMgr );
		else
			OSL_ASSERT(!"Could not load library!");
	}

	//////////////////////////////////////////////////////////////////////

	if ( xFactory.is() )
	{
		xFactory->acquire();
		pRet = xFactory.get();
	}

	return pRet;
}

