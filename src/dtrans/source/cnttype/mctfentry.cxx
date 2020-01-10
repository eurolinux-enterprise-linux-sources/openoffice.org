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
#include "precompiled_dtrans.hxx"

//-----------------------------------------------------------------
//	includes of other projects
//-----------------------------------------------------------------
#include <cppuhelper/factory.hxx>
#include <com/sun/star/container/XSet.hpp>
#include <osl/diagnose.h>
#include <com/sun/star/datatransfer/XMimeContentTypeFactory.hpp>
#include "mcnttfactory.hxx"

//-----------------------------------------------------------------
// some defines
//-----------------------------------------------------------------

// the service names
#define MIMECONTENTTYPEFACTORY_SERVICE_NAME  "com.sun.star.datatransfer.MimeContentTypeFactory"

// the implementation names		
#define MIMECONTENTTYPEFACTORY_IMPL_NAME  "com.sun.star.datatransfer.MimeCntTypeFactory"

// the registry key names
// a key under which this service will be registered, Format: -> "/ImplName/UNO/SERVICES/ServiceName"
//                        <     Implementation-Name    ></UNO/SERVICES/><    Service-Name           > 
#define MIMECONTENTTYPEFACTORY_REGKEY_NAME  "/com.sun.star.datatransfer.MimeCntTypeFactory/UNO/SERVICES/com.sun.star.datatransfer.MimeContentTypeFactory"

//-----------------------------------------------------------------------------------------------------------
// namespace directives
//-----------------------------------------------------------------------------------------------------------

using namespace ::rtl						;
using namespace ::cppu					    ;
using namespace ::com::sun::star::uno		;
using namespace ::com::sun::star::registry	;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::datatransfer;

//-----------------------------------------------------------------
// create a static object to initialize the shell9x library
//-----------------------------------------------------------------
 
namespace
{

	//-----------------------------------------------------------------------------------------------------------
    // functions to create a new Clipboad instance; is needed by factory helper implementation
    // @param rServiceManager - service manager, useful if the component needs other uno services
    // so we should give it to every UNO-Implementation component	
    //-----------------------------------------------------------------------------------------------------------

	Reference< XInterface > SAL_CALL createInstance( const Reference< XMultiServiceFactory >& rServiceManager )
	{
		return Reference< XInterface >( static_cast< XMimeContentTypeFactory* >( new CMimeContentTypeFactory( rServiceManager ) ) );
	}
}

//-----------------------------------------------------------------------------------------------------------
// the 3 important functions which will be exported 
//-----------------------------------------------------------------------------------------------------------

extern "C" 
{

//----------------------------------------------------------------------
// component_getImplementationEnvironment
//----------------------------------------------------------------------

void SAL_CALL component_getImplementationEnvironment(
	const sal_Char ** ppEnvTypeName, uno_Environment ** /*ppEnv*/ )
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

//-------------------------------------------------------------------------
// component_writeInfo - to register a UNO-Service
// to register a UNO-Service use: regcomp -register -r *.rdb -c *.dll
// to view the registry use: regview *.rdb /SERVICES/ServiceName
// (you must use the full services name e.g. com.sun.star.frame.FilePicker
//-------------------------------------------------------------------------

sal_Bool SAL_CALL component_writeInfo( void* /*pServiceManager*/, void* pRegistryKey )
{
	sal_Bool bRetVal = sal_False;

	if ( pRegistryKey )
	{
		try
		{
			Reference< XRegistryKey > pXNewKey( static_cast< XRegistryKey* >( pRegistryKey ) );							
			pXNewKey->createKey( OUString( RTL_CONSTASCII_USTRINGPARAM( MIMECONTENTTYPEFACTORY_REGKEY_NAME ) ) );
			bRetVal = sal_True;
		}
		catch( InvalidRegistryException& )
		{			
			OSL_ENSURE(sal_False, "InvalidRegistryException caught");			
			bRetVal = sal_False;
		}
	}

	return bRetVal;
}

//----------------------------------------------------------------------
// component_getFactory 
// returns a factory to create XFilePicker-Services
//----------------------------------------------------------------------

void* SAL_CALL component_getFactory( const sal_Char* pImplName, uno_Interface* pSrvManager, uno_Interface* /*pRegistryKey*/ )
{
	void* pRet = 0;

	if ( pSrvManager && ( 0 == rtl_str_compare( pImplName, MIMECONTENTTYPEFACTORY_IMPL_NAME ) ) )
	{
		Sequence< OUString > aSNS( 1 );
		aSNS.getArray( )[0] = OUString( RTL_CONSTASCII_USTRINGPARAM( MIMECONTENTTYPEFACTORY_SERVICE_NAME ) );		
		
		Reference< XSingleServiceFactory > xFactory ( createSingleFactory(
			reinterpret_cast< XMultiServiceFactory* > ( pSrvManager ),
			OUString::createFromAscii( pImplName ),
			createInstance,
			aSNS ) );
		if ( xFactory.is() )
		{
			xFactory->acquire();
			pRet = xFactory.get();
		}			
	}

	return pRet;
}

} // extern "C"
