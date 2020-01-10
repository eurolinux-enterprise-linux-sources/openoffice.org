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
#include "precompiled_fpicker.hxx"

//-----------------------------------------------------------------------
//	includes of other projects
//-----------------------------------------------------------------------
#include <cppuhelper/factory.hxx>
#include <com/sun/star/container/XSet.hpp>
#include <osl/diagnose.h>

#ifndef _FILEPICKER_HXX_
#include "folderpicker.hxx"
#endif

#ifndef _FPSERVICEINFO_HXX_
#include "FOPServiceInfo.hxx"
#endif
#include "WinFOPImpl.hxx"

//-----------------------------------------------------------------------
// namespace directives
//-----------------------------------------------------------------------

using namespace ::rtl						;
using namespace ::com::sun::star::uno		;
using namespace ::com::sun::star::container	;
using namespace ::com::sun::star::lang		;
using namespace ::com::sun::star::registry	;
using namespace ::cppu					    ;
using com::sun::star::ui::dialogs::XFolderPicker;

 
namespace
{

	//-----------------------------------------------------------------------
    // 
    //-----------------------------------------------------------------------

	Reference< XInterface > SAL_CALL createInstance( const Reference< XMultiServiceFactory >& rServiceManager )
	{		
		return Reference< XInterface >( static_cast< XFolderPicker* >( new CFolderPicker( rServiceManager ) ) );
	}
}

//-----------------------------------------------------------------------
// the 3 important functions which will be exported 
//-----------------------------------------------------------------------

extern "C" 
{

//----------------------------------------------------------------------
// component_getImplementationEnvironment
//----------------------------------------------------------------------

void SAL_CALL component_getImplementationEnvironment(
	const sal_Char ** ppEnvTypeName, uno_Environment ** )
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------

sal_Bool SAL_CALL component_writeInfo( void*, void* pRegistryKey )
{
	sal_Bool bRetVal = sal_True;

	if ( pRegistryKey )
	{
		try
		{
			Reference< XRegistryKey > pXNewKey( static_cast< XRegistryKey* >( pRegistryKey ) );							
			pXNewKey->createKey( OUString( RTL_CONSTASCII_USTRINGPARAM( FOLDER_PICKER_REGKEY_NAME ) ) );
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

void* SAL_CALL component_getFactory( const sal_Char* pImplName, uno_Interface* pSrvManager, uno_Interface* )
{
	void* pRet = 0;

	if ( pSrvManager && ( 0 == rtl_str_compare( pImplName, FOLDER_PICKER_IMPL_NAME ) ) )
	{
		Sequence< OUString > aSNS( 1 );
		aSNS.getArray( )[0] = OUString( RTL_CONSTASCII_USTRINGPARAM( FOLDER_PICKER_SERVICE_NAME ) );		
				
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
