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
#include <ManifestReader.hxx>
#include <ManifestWriter.hxx>
#include <cppuhelper/factory.hxx>
#ifndef _COM_SUN_STAR_REGISTRY_XREGISTRYKEY_HPP
#include <com/sun/star/registry/XRegistryKey.hpp>
#endif
#include <vos/diagnose.hxx>
#include <ZipPackage.hxx>

#include <zipfileaccess.hxx>

using namespace ::rtl;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::registry;
using namespace ::com::sun::star::packages;
using namespace ::com::sun::star::packages::manifest;

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
// C functions to implement this as a component

extern "C" void SAL_CALL component_getImplementationEnvironment(
    const sal_Char ** ppEnvTypeName, uno_Environment ** /*ppEnv*/ )
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}
 
/**
 * This function creates an implementation section in the registry and another subkey
 * for each supported service.
 * @param pServiceManager generic uno interface providing a service manager
 * @param pRegistryKey generic uno interface providing registry key to write
 */
extern "C" sal_Bool SAL_CALL component_writeInfo( void* /*pServiceManager*/, void* pRegistryKey )
{
    return pRegistryKey &&
	writeInfo (pRegistryKey, 
			   ManifestReader::static_getImplementationName(),
			   ManifestReader::static_getSupportedServiceNames() ) &&
	writeInfo (pRegistryKey, 
			   ManifestWriter::static_getImplementationName(),
			   ManifestWriter::static_getSupportedServiceNames() ) &&
	writeInfo (pRegistryKey, 
			   ZipPackage::static_getImplementationName(),
			   ZipPackage::static_getSupportedServiceNames() ) &&

	writeInfo (pRegistryKey, 
			   OZipFileAccess::impl_staticGetImplementationName(),
			   OZipFileAccess::impl_staticGetSupportedServiceNames() ); 

} 


/**
 * This function is called to get service factories for an implementation.
 * @param pImplName name of implementation
 * @param pServiceManager generic uno interface providing a service manager to instantiate components
 * @param pRegistryKey registry data key to read and write component persistent data
 * @return a component factory (generic uno interface)
 */
extern "C" void * SAL_CALL component_getFactory(
    const sal_Char * pImplName, void * pServiceManager, void * /*pRegistryKey*/ )
{
    void * pRet = 0;
	Reference< XMultiServiceFactory > xSMgr(
			reinterpret_cast< XMultiServiceFactory * >( pServiceManager ) );
	Reference< XSingleServiceFactory > xFactory;

	if (ManifestReader::static_getImplementationName().compareToAscii( pImplName ) == 0)
		xFactory = ManifestReader::createServiceFactory ( xSMgr );
	else if (ManifestWriter::static_getImplementationName().compareToAscii( pImplName ) == 0)
		xFactory = ManifestWriter::createServiceFactory ( xSMgr );
	else if (ZipPackage::static_getImplementationName().compareToAscii( pImplName ) == 0)
		xFactory = ZipPackage::createServiceFactory ( xSMgr );
	else if ( OZipFileAccess::impl_staticGetImplementationName().compareToAscii( pImplName ) == 0 )
		xFactory = ::cppu::createSingleFactory( xSMgr,
										   OZipFileAccess::impl_staticGetImplementationName(),
										   OZipFileAccess::impl_staticCreateSelfInstance,
										   OZipFileAccess::impl_staticGetSupportedServiceNames() );

	if ( xFactory.is() )
	{
		xFactory->acquire();
		pRet = xFactory.get();
	}
    return pRet;
}

