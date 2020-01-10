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
#include <rtl/unload.h>
#include <osl/time.h>
#include "ole2uno.hxx"
#include "servprov.hxx"
#include <rtl/ustring.hxx>
#include <cppuhelper/factory.hxx>
using namespace rtl;
using namespace ole_adapter;
using namespace cppu;

namespace ole_adapter
{
rtl_StandardModuleCount globalModuleCount= MODULE_COUNT_INIT;



Reference<XInterface> SAL_CALL ConverterProvider_CreateInstance2(	const Reference<XMultiServiceFactory> & xSMgr)
							throw(Exception)
{
	Reference<XInterface> xService = *new OleConverter_Impl2( xSMgr);
	return xService;
}

Reference<XInterface> SAL_CALL ConverterProvider_CreateInstanceVar1(	const Reference<XMultiServiceFactory> & xSMgr)
							throw(Exception)
{
	Reference<XInterface> xService = *new OleConverter_Impl2( xSMgr, UNO_OBJECT_WRAPPER_REMOTE_OPT, IUNKNOWN_WRAPPER_IMPL);
	return xService;
}

Reference<XInterface> SAL_CALL OleClient_CreateInstance( const Reference<XMultiServiceFactory> & xSMgr)
							throw(Exception)
{
	Reference<XInterface> xService = *new OleClient_Impl( xSMgr);
	return xService;
}

Reference<XInterface> SAL_CALL OleServer_CreateInstance( const Reference<XMultiServiceFactory> & xSMgr)
							throw (Exception)
{
	Reference<XInterface > xService = *new OleServer_Impl(xSMgr);
	return xService;
}
} // end namespace

extern "C" void * SAL_CALL component_getFactory(
	const sal_Char * pImplName, void * pServiceManager, void * /*pRegistryKey*/ )
{
	void * pRet = 0;
	
	OUString aImplName( OUString::createFromAscii( pImplName ) );
	Reference< XSingleServiceFactory > xFactory;
	Sequence<OUString> seqServiceNames;
	if (pServiceManager && aImplName.equals(  reinterpret_cast<const sal_Unicode*>(L"com.sun.star.comp.ole.OleConverter2")  ))
	{
		xFactory=  createSingleFactory( reinterpret_cast< XMultiServiceFactory*>(pServiceManager),
										 OUString( RTL_CONSTASCII_USTRINGPARAM("com.sun.star.comp.ole.OleConverter2")), 
										 ConverterProvider_CreateInstance2, seqServiceNames,
										 &globalModuleCount.modCnt );
	}
	else if (pServiceManager && aImplName.equals(  reinterpret_cast<const sal_Unicode*>(L"com.sun.star.comp.ole.OleConverterVar1")  ))
	{
		xFactory= createSingleFactory( reinterpret_cast<XMultiServiceFactory*>(pServiceManager),
									   OUString( RTL_CONSTASCII_USTRINGPARAM("com.sun.star.comp.ole.OleConverterVar1")), 
									   ConverterProvider_CreateInstanceVar1, seqServiceNames,
									   &globalModuleCount.modCnt );
	}
	else if(pServiceManager && aImplName.equals(reinterpret_cast<const sal_Unicode*>(L"com.sun.star.comp.ole.OleClient")))
	{
		xFactory= createSingleFactory( reinterpret_cast< XMultiServiceFactory*>(pServiceManager),
									   OUString( RTL_CONSTASCII_USTRINGPARAM("com.sun.star.comp.ole.OleClient")), 
									   OleClient_CreateInstance, seqServiceNames,
									   &globalModuleCount.modCnt);
	}
	else if(pServiceManager && aImplName.equals(reinterpret_cast<const sal_Unicode*>(L"com.sun.star.comp.ole.OleServer")))
	{
		xFactory= createOneInstanceFactory( reinterpret_cast< XMultiServiceFactory*>(pServiceManager),
											OUString( RTL_CONSTASCII_USTRINGPARAM("com.sun.star.comp.ole.OleServer")), 
											OleServer_CreateInstance, seqServiceNames,
											&globalModuleCount.modCnt);
	}
		
	if (xFactory.is())
	{
		xFactory->acquire();
		pRet = xFactory.get();
	}
	
	return pRet;
}


extern "C" sal_Bool SAL_CALL component_writeInfo(	void * /*pServiceManager*/, void * pRegistryKey )
{
	if (pRegistryKey)
	{
		try
		{
            //deprecated
			Reference<XRegistryKey> xNewKey =
				reinterpret_cast<XRegistryKey*>( pRegistryKey)->createKey(reinterpret_cast<const sal_Unicode*>(L"/com.sun.star.comp.ole.OleConverter2/UNO/SERVICES"));
			xNewKey->createKey(reinterpret_cast<const sal_Unicode*>(L"com.sun.star.bridge.OleBridgeSupplier2"));

			xNewKey->createKey(reinterpret_cast<const sal_Unicode*>(L"com.sun.star.bridge.oleautomation.BridgeSupplier"));

            //deprecated
			xNewKey =
				reinterpret_cast<XRegistryKey*>( pRegistryKey)->createKey(reinterpret_cast<const sal_Unicode*>(L"/com.sun.star.comp.ole.OleConverterVar1/UNO/SERVICES"));
			xNewKey->createKey(reinterpret_cast<const sal_Unicode*>(L"com.sun.star.bridge.OleBridgeSupplierVar1"));

            //deprecated
			xNewKey =
				reinterpret_cast<XRegistryKey*>( pRegistryKey)->createKey(reinterpret_cast<const sal_Unicode*>(L"/com.sun.star.comp.ole.OleClient/UNO/SERVICES"));
			xNewKey->createKey(reinterpret_cast<const sal_Unicode*>(L"com.sun.star.bridge.OleObjectFactory"));

			xNewKey->createKey(reinterpret_cast<const sal_Unicode*>(L"com.sun.star.bridge.oleautomation.Factory"));
            //deprecated
			xNewKey =
				reinterpret_cast<XRegistryKey*>( pRegistryKey)->createKey(reinterpret_cast<const sal_Unicode*>(L"/com.sun.star.comp.ole.OleServer/UNO/SERVICES"));
			xNewKey->createKey(reinterpret_cast<const sal_Unicode*>(L"com.sun.star.bridge.OleApplicationRegistration"));

			xNewKey->createKey(reinterpret_cast<const sal_Unicode*>(L"com.sun.star.bridge.oleautomation.ApplicationRegistration"));
				
			return sal_True;
		}
		catch (InvalidRegistryException &)
		{
			OSL_ENSURE( sal_False, "### InvalidRegistryException!" );
		}
	}
	return sal_False;
}

extern "C" void SAL_CALL component_getImplementationEnvironment(
	const sal_Char ** ppEnvTypeName, uno_Environment ** /*ppEnv*/ )
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}


extern "C"	sal_Bool component_canUnload( TimeValue* libUnused)
{
	return globalModuleCount.canUnload( &globalModuleCount, libUnused);
}
