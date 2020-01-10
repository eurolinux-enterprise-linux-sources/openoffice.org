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
#include "precompiled_package.hxx"


#include <com/sun/star/registry/XRegistryKey.hpp>
#include <com/sun/star/registry/InvalidRegistryException.hpp>
#include <cppuhelper/factory.hxx>

#include "xfactory.hxx"

using namespace ::com::sun::star;


extern "C" {

void SAL_CALL component_getImplementationEnvironment( const sal_Char ** ppEnvTypeName, uno_Environment ** /*ppEnv*/ )
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

void * SAL_CALL component_getFactory( const sal_Char * pImplName, void * pServiceManager, void * /*pRegistryKey*/ )
{
	void * pRet = 0;
	
	::rtl::OUString aImplName( ::rtl::OUString::createFromAscii( pImplName ) );
	uno::Reference< lang::XSingleServiceFactory > xFactory;

	if ( pServiceManager && aImplName.equals( OStorageFactory::impl_staticGetImplementationName() ) )
	{
		xFactory= ::cppu::createOneInstanceFactory( reinterpret_cast< lang::XMultiServiceFactory*>( pServiceManager ),
											OStorageFactory::impl_staticGetImplementationName(),
											OStorageFactory::impl_staticCreateSelfInstance,
											OStorageFactory::impl_staticGetSupportedServiceNames() );
	}
		
	if ( xFactory.is() )
	{
		xFactory->acquire();
		pRet = xFactory.get();
	}
	
	return pRet;
}

sal_Bool SAL_CALL component_writeInfo( void * /*pServiceManager*/, void * pRegistryKey )
{
	if (pRegistryKey)
	{
		try
		{
    		uno::Reference< registry::XRegistryKey > xKey( reinterpret_cast< registry::XRegistryKey* >( pRegistryKey ) );

    		uno::Reference< registry::XRegistryKey >  xNewKey;

			xNewKey = xKey->createKey( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("/") ) + 
										OStorageFactory::impl_staticGetImplementationName() +
										::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM( "/UNO/SERVICES") )  );

			const uno::Sequence< ::rtl::OUString > aServices = OStorageFactory::impl_staticGetSupportedServiceNames();
			for( sal_Int32 ind = 0; ind < aServices.getLength(); ind++ )
				xNewKey->createKey( aServices.getConstArray()[ind] );

			return sal_True;
		}
		catch (registry::InvalidRegistryException &)
		{
			OSL_ENSURE( sal_False, "### InvalidRegistryException!" );
		}
	}
	return sal_False;
}

} // extern "C"

